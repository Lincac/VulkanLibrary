#include "Swapchain.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "Context.h"

Swapchain::Swapchain(Context& context, GLFWwindow* window, bool enableVSync)
    : context_(&context),
      window_(window),
      enableVSync_(enableVSync)
{
    if (window_ == nullptr)
    {
        throw std::runtime_error("Cannot create swapchain with null GLFW window.");
    }

    CreateSurface();
    CreateSwapchain();
    CreateImageViews();
}

Swapchain::~Swapchain()
{
    Destroy();
}

Swapchain::Swapchain(Swapchain&& other) noexcept
    : context_(other.context_),
      window_(other.window_),
      enableVSync_(other.enableVSync_),
      surface_(other.surface_),
      swapchain_(other.swapchain_),
      imageFormat_(other.imageFormat_),
      extent_(other.extent_),
      presentMode_(other.presentMode_),
      presentQueueFamilyIndex_(other.presentQueueFamilyIndex_),
      presentQueue_(other.presentQueue_),
      images_(std::move(other.images_)),
      imageViews_(std::move(other.imageViews_))
{
    other.context_ = nullptr;
    other.window_ = nullptr;
    other.surface_ = VK_NULL_HANDLE;
    other.swapchain_ = VK_NULL_HANDLE;
    other.imageFormat_ = VK_FORMAT_UNDEFINED;
    other.extent_ = {};
    other.presentMode_ = VK_PRESENT_MODE_FIFO_KHR;
    other.presentQueueFamilyIndex_ = 0;
    other.presentQueue_ = VK_NULL_HANDLE;
}

Swapchain& Swapchain::operator=(Swapchain&& other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    Destroy();

    context_ = other.context_;
    window_ = other.window_;
    enableVSync_ = other.enableVSync_;
    surface_ = other.surface_;
    swapchain_ = other.swapchain_;
    imageFormat_ = other.imageFormat_;
    extent_ = other.extent_;
    presentMode_ = other.presentMode_;
    presentQueueFamilyIndex_ = other.presentQueueFamilyIndex_;
    presentQueue_ = other.presentQueue_;
    images_ = std::move(other.images_);
    imageViews_ = std::move(other.imageViews_);

    other.context_ = nullptr;
    other.window_ = nullptr;
    other.surface_ = VK_NULL_HANDLE;
    other.swapchain_ = VK_NULL_HANDLE;
    other.imageFormat_ = VK_FORMAT_UNDEFINED;
    other.extent_ = {};
    other.presentMode_ = VK_PRESENT_MODE_FIFO_KHR;
    other.presentQueueFamilyIndex_ = 0;
    other.presentQueue_ = VK_NULL_HANDLE;

    return *this;
}

void Swapchain::Recreate()
{
    if (context_ == nullptr || window_ == nullptr)
    {
        throw std::runtime_error("Cannot recreate swapchain from invalid state.");
    }

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window_, &width, &height);

    while (width == 0 || height == 0)
    {
        glfwWaitEvents();
        glfwGetFramebufferSize(window_, &width, &height);
    }

    vkDeviceWaitIdle(context_->GetVkDevice());

    CleanupSwapchain();
    CreateSwapchain();
    CreateImageViews();
}

uint32_t Swapchain::AcquireNextImage(VkSemaphore imageAvailableSemaphore, VkFence inFlightFence) const
{
    if (swapchain_ == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Cannot acquire image from an invalid swapchain.");
    }

    uint32_t imageIndex = 0;
    const VkResult result = vkAcquireNextImageKHR(
        context_->GetVkDevice(),
        swapchain_,
        std::numeric_limits<uint64_t>::max(),
        imageAvailableSemaphore,
        inFlightFence,
        &imageIndex);

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("Failed to acquire swapchain image.");
    }

    return imageIndex;
}

VkResult Swapchain::Present(uint32_t imageIndex, const std::vector<VkSemaphore>& waitSemaphores) const
{
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
    presentInfo.pWaitSemaphores = waitSemaphores.empty() ? nullptr : waitSemaphores.data();
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain_;
    presentInfo.pImageIndices = &imageIndex;

    return vkQueuePresentKHR(presentQueue_, &presentInfo);
}

VkSwapchainKHR Swapchain::Get() const noexcept
{
    return swapchain_;
}

VkSurfaceKHR Swapchain::GetSurface() const noexcept
{
    return surface_;
}

VkFormat Swapchain::GetImageFormat() const noexcept
{
    return imageFormat_;
}

VkExtent2D Swapchain::GetExtent() const noexcept
{
    return extent_;
}

VkPresentModeKHR Swapchain::GetPresentMode() const noexcept
{
    return presentMode_;
}

uint32_t Swapchain::GetImageCount() const noexcept
{
    return static_cast<uint32_t>(images_.size());
}

const std::vector<VkImage>& Swapchain::GetImages() const noexcept
{
    return images_;
}

const std::vector<VkImageView>& Swapchain::GetImageViews() const noexcept
{
    return imageViews_.GetAll();
}

void Swapchain::CreateSurface()
{
    if (glfwCreateWindowSurface(context_->GetVkInstance(), window_, nullptr, &surface_) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan surface from GLFW window.");
    }
}

void Swapchain::CreateSwapchain()
{
    const VkPhysicalDevice physicalDevice = context_->GetVkPhysicalDevice();
    const SwapchainSupportDetails support = QuerySwapchainSupport(physicalDevice);

    if (support.formats.empty() || support.presentModes.empty())
    {
        throw std::runtime_error("Swapchain is not supported by selected surface/device.");
    }

    const auto graphicsQueueFamilyIndex = context_->GetGraphicsQueueFamilyIndex();
    if (!graphicsQueueFamilyIndex.has_value())
    {
        throw std::runtime_error("Graphics queue family is required for swapchain.");
    }

    presentQueueFamilyIndex_ = FindPresentQueueFamily(physicalDevice);
    const auto computeQueueFamilyIndex = context_->GetComputeQueueFamilyIndex();
    const auto transferQueueFamilyIndex = context_->GetTransferQueueFamilyIndex();

    const bool presentQueueFamilyCreatedInDevice =
        presentQueueFamilyIndex_ == *graphicsQueueFamilyIndex ||
        (computeQueueFamilyIndex.has_value() && presentQueueFamilyIndex_ == *computeQueueFamilyIndex) ||
        (transferQueueFamilyIndex.has_value() && presentQueueFamilyIndex_ == *transferQueueFamilyIndex);

    if (!presentQueueFamilyCreatedInDevice)
    {
        throw std::runtime_error(
            "Present queue family was not created by Device. Ensure present queue family is included in logical device queues.");
    }

    VkSurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat(support.formats);
    presentMode_ = ChoosePresentMode(support.presentModes);
    extent_ = ChooseExtent(support.capabilities);

    uint32_t imageCount = support.capabilities.minImageCount + 1;
    if (support.capabilities.maxImageCount > 0 && imageCount > support.capabilities.maxImageCount)
    {
        imageCount = support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface_;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent_;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.preTransform = support.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode_;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    const uint32_t queueFamilyIndices[] = {*graphicsQueueFamilyIndex, presentQueueFamilyIndex_};
    if (*graphicsQueueFamilyIndex != presentQueueFamilyIndex_)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    if (vkCreateSwapchainKHR(context_->GetVkDevice(), &createInfo, nullptr, &swapchain_) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan swapchain.");
    }

    imageFormat_ = surfaceFormat.format;

    uint32_t createdImageCount = 0;
    vkGetSwapchainImagesKHR(context_->GetVkDevice(), swapchain_, &createdImageCount, nullptr);
    images_.resize(createdImageCount);
    vkGetSwapchainImagesKHR(context_->GetVkDevice(), swapchain_, &createdImageCount, images_.data());

    presentQueue_ = context_->GetDevice().GetQueue(presentQueueFamilyIndex_);
}

void Swapchain::CreateImageViews()
{
    if (imageViews_.IsValid())
    {
        imageViews_.Recreate(images_, imageFormat_);
    }
    else
    {
        imageViews_ = SwapchainImageViews(context_->GetVkDevice(), images_, imageFormat_);
    }
}

void Swapchain::CleanupSwapchain() noexcept
{
    if (context_ == nullptr)
    {
        return;
    }

    imageViews_.Destroy();
    images_.clear();

    if (swapchain_ != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(context_->GetVkDevice(), swapchain_, nullptr);
        swapchain_ = VK_NULL_HANDLE;
    }
}

void Swapchain::CleanupSurface() noexcept
{
    if (context_ != nullptr && surface_ != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(context_->GetVkInstance(), surface_, nullptr);
        surface_ = VK_NULL_HANDLE;
    }
}

void Swapchain::Destroy() noexcept
{
    CleanupSwapchain();
    CleanupSurface();

    context_ = nullptr;
    window_ = nullptr;
    imageFormat_ = VK_FORMAT_UNDEFINED;
    extent_ = {};
    presentMode_ = VK_PRESENT_MODE_FIFO_KHR;
    presentQueueFamilyIndex_ = 0;
    presentQueue_ = VK_NULL_HANDLE;
}

Swapchain::SwapchainSupportDetails Swapchain::QuerySwapchainSupport(VkPhysicalDevice physicalDevice) const
{
    SwapchainSupportDetails details{};

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface_, &details.capabilities);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface_, &formatCount, nullptr);
    if (formatCount > 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface_, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface_, &presentModeCount, nullptr);
    if (presentModeCount > 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice,
            surface_,
            &presentModeCount,
            details.presentModes.data());
    }

    return details;
}

uint32_t Swapchain::FindPresentQueueFamily(VkPhysicalDevice physicalDevice) const
{
    const auto& queueFamilies = context_->GetPhysicalDevice().GetQueueFamilyProperties();
    for (uint32_t familyIndex = 0; familyIndex < static_cast<uint32_t>(queueFamilies.size()); ++familyIndex)
    {
        VkBool32 supportsPresent = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, familyIndex, surface_, &supportsPresent);
        if (supportsPresent == VK_TRUE)
        {
            return familyIndex;
        }
    }

    throw std::runtime_error("No present-capable queue family found for this surface.");
}

VkSurfaceFormatKHR Swapchain::ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const
{
    for (const auto& format : formats)
    {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return format;
        }
    }

    return formats.front();
}

VkPresentModeKHR Swapchain::ChoosePresentMode(const std::vector<VkPresentModeKHR>& presentModes) const
{
    if (!enableVSync_)
    {
        const auto mailboxIt = std::find(presentModes.begin(), presentModes.end(), VK_PRESENT_MODE_MAILBOX_KHR);
        if (mailboxIt != presentModes.end())
        {
            return *mailboxIt;
        }

        const auto immediateIt = std::find(presentModes.begin(), presentModes.end(), VK_PRESENT_MODE_IMMEDIATE_KHR);
        if (immediateIt != presentModes.end())
        {
            return *immediateIt;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Swapchain::ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities) const
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window_, &width, &height);

    VkExtent2D actualExtent{};
    actualExtent.width = static_cast<uint32_t>(width);
    actualExtent.height = static_cast<uint32_t>(height);

    actualExtent.width = std::clamp(
        actualExtent.width,
        capabilities.minImageExtent.width,
        capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(
        actualExtent.height,
        capabilities.minImageExtent.height,
        capabilities.maxImageExtent.height);

    return actualExtent;
}
