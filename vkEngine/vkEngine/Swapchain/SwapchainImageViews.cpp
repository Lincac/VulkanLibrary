#include "SwapchainImageViews.h"

#include <stdexcept>
#include <utility>

SwapchainImageViews::SwapchainImageViews(
    VkDevice device,
    const std::vector<VkImage>& swapchainImages,
    VkFormat imageFormat,
    VkImageAspectFlags aspectMask)
    : device_(device),
      aspectMask_(aspectMask)
{
    if (device_ == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Cannot create swapchain image views with a null device.");
    }

    CreateImageViews(swapchainImages, imageFormat);
}

SwapchainImageViews::~SwapchainImageViews()
{
    Destroy();
}

SwapchainImageViews::SwapchainImageViews(SwapchainImageViews&& other) noexcept
    : device_(other.device_),
      aspectMask_(other.aspectMask_),
      imageViews_(std::move(other.imageViews_))
{
    other.device_ = VK_NULL_HANDLE;
    other.aspectMask_ = VK_IMAGE_ASPECT_COLOR_BIT;
}

SwapchainImageViews& SwapchainImageViews::operator=(SwapchainImageViews&& other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    Destroy();

    device_ = other.device_;
    aspectMask_ = other.aspectMask_;
    imageViews_ = std::move(other.imageViews_);

    other.device_ = VK_NULL_HANDLE;
    other.aspectMask_ = VK_IMAGE_ASPECT_COLOR_BIT;
    return *this;
}

void SwapchainImageViews::Recreate(const std::vector<VkImage>& swapchainImages, VkFormat imageFormat)
{
    if (device_ == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Cannot recreate image views with a null device.");
    }

    Destroy();
    CreateImageViews(swapchainImages, imageFormat);
}

void SwapchainImageViews::Destroy() noexcept
{
    if (device_ == VK_NULL_HANDLE)
    {
        imageViews_.clear();
        return;
    }

    for (VkImageView imageView : imageViews_)
    {
        if (imageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(device_, imageView, nullptr);
        }
    }

    imageViews_.clear();
}

VkImageView SwapchainImageViews::Get(uint32_t index) const
{
    if (index >= imageViews_.size())
    {
        throw std::out_of_range("Swapchain image view index is out of range.");
    }

    return imageViews_[index];
}

const std::vector<VkImageView>& SwapchainImageViews::GetAll() const noexcept
{
    return imageViews_;
}

uint32_t SwapchainImageViews::GetCount() const noexcept
{
    return static_cast<uint32_t>(imageViews_.size());
}

bool SwapchainImageViews::IsValid() const noexcept
{
    return device_ != VK_NULL_HANDLE;
}

void SwapchainImageViews::CreateImageViews(const std::vector<VkImage>& swapchainImages, VkFormat imageFormat)
{
    imageViews_.clear();
    imageViews_.reserve(swapchainImages.size());

    for (VkImage image : swapchainImages)
    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = image;
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = imageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = aspectMask_;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VkImageView imageView = VK_NULL_HANDLE;
        if (vkCreateImageView(device_, &createInfo, nullptr, &imageView) != VK_SUCCESS)
        {
            for (VkImageView createdView : imageViews_)
            {
                vkDestroyImageView(device_, createdView, nullptr);
            }
            imageViews_.clear();
            throw std::runtime_error("Failed to create swapchain image view.");
        }

        imageViews_.push_back(imageView);
    }
}
