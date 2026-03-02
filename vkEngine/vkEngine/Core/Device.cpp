#include "Device.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

Device::Device(
    const PhysicalDevice& physicalDevice,
    std::vector<const char*> enabledExtensions,
    const VkPhysicalDeviceFeatures* enabledFeatures)
    : physicalDevice_(&physicalDevice)
{
    if (!physicalDevice.IsValid())
    {
        throw std::runtime_error("Cannot create logical device from an invalid physical device.");
    }

    if (!physicalDevice.SupportsExtensions(enabledExtensions))
    {
        throw std::runtime_error("Physical device does not support all requested device extensions.");
    }

    graphicsQueueFamilyIndex_ = physicalDevice.FindQueueFamily(VK_QUEUE_GRAPHICS_BIT);
    computeQueueFamilyIndex_ = physicalDevice.FindQueueFamily(VK_QUEUE_COMPUTE_BIT);
    transferQueueFamilyIndex_ = physicalDevice.FindQueueFamily(VK_QUEUE_TRANSFER_BIT);

    if (!graphicsQueueFamilyIndex_)
    {
        throw std::runtime_error("No graphics queue family found on selected physical device.");
    }

    CreateDevice(physicalDevice, enabledExtensions, enabledFeatures);
    CacheQueues();
}

Device::~Device()
{
    Destroy();
}

Device::Device(Device&& other) noexcept
    : physicalDevice_(other.physicalDevice_),
      device_(other.device_),
      graphicsQueueFamilyIndex_(other.graphicsQueueFamilyIndex_),
      computeQueueFamilyIndex_(other.computeQueueFamilyIndex_),
      transferQueueFamilyIndex_(other.transferQueueFamilyIndex_),
      graphicsQueue_(other.graphicsQueue_),
      computeQueue_(other.computeQueue_),
      transferQueue_(other.transferQueue_)
{
    other.physicalDevice_ = nullptr;
    other.device_ = VK_NULL_HANDLE;
    other.graphicsQueueFamilyIndex_.reset();
    other.computeQueueFamilyIndex_.reset();
    other.transferQueueFamilyIndex_.reset();
    other.graphicsQueue_ = VK_NULL_HANDLE;
    other.computeQueue_ = VK_NULL_HANDLE;
    other.transferQueue_ = VK_NULL_HANDLE;
}

Device& Device::operator=(Device&& other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    Destroy();

    physicalDevice_ = other.physicalDevice_;
    device_ = other.device_;
    graphicsQueueFamilyIndex_ = other.graphicsQueueFamilyIndex_;
    computeQueueFamilyIndex_ = other.computeQueueFamilyIndex_;
    transferQueueFamilyIndex_ = other.transferQueueFamilyIndex_;
    graphicsQueue_ = other.graphicsQueue_;
    computeQueue_ = other.computeQueue_;
    transferQueue_ = other.transferQueue_;

    other.physicalDevice_ = nullptr;
    other.device_ = VK_NULL_HANDLE;
    other.graphicsQueueFamilyIndex_.reset();
    other.computeQueueFamilyIndex_.reset();
    other.transferQueueFamilyIndex_.reset();
    other.graphicsQueue_ = VK_NULL_HANDLE;
    other.computeQueue_ = VK_NULL_HANDLE;
    other.transferQueue_ = VK_NULL_HANDLE;

    return *this;
}

VkDevice Device::Get() const noexcept
{
    return device_;
}

bool Device::IsValid() const noexcept
{
    return device_ != VK_NULL_HANDLE;
}

const PhysicalDevice& Device::GetPhysicalDevice() const noexcept
{
    return *physicalDevice_;
}

std::optional<uint32_t> Device::GetGraphicsQueueFamilyIndex() const noexcept
{
    return graphicsQueueFamilyIndex_;
}

std::optional<uint32_t> Device::GetComputeQueueFamilyIndex() const noexcept
{
    return computeQueueFamilyIndex_;
}

std::optional<uint32_t> Device::GetTransferQueueFamilyIndex() const noexcept
{
    return transferQueueFamilyIndex_;
}

VkQueue Device::GetGraphicsQueue() const noexcept
{
    return graphicsQueue_;
}

VkQueue Device::GetComputeQueue() const noexcept
{
    return computeQueue_;
}

VkQueue Device::GetTransferQueue() const noexcept
{
    return transferQueue_;
}

VkQueue Device::GetQueue(uint32_t familyIndex) const
{
    if (device_ == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Cannot get queue from an invalid logical device.");
    }

    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(device_, familyIndex, 0, &queue);
    return queue;
}

void Device::CreateDevice(
    const PhysicalDevice& physicalDevice,
    const std::vector<const char*>& enabledExtensions,
    const VkPhysicalDeviceFeatures* enabledFeatures)
{
    std::vector<uint32_t> families = BuildQueueFamiliesToCreate();
    std::vector<float> queuePriorities(families.size(), 1.0f);
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.reserve(families.size());

    for (size_t i = 0; i < families.size(); ++i)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = families[i];
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriorities[i];
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures defaultFeatures{};
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    createInfo.ppEnabledExtensionNames = enabledExtensions.empty() ? nullptr : enabledExtensions.data();
    createInfo.pEnabledFeatures = enabledFeatures != nullptr ? enabledFeatures : &defaultFeatures;

    if (vkCreateDevice(physicalDevice.Get(), &createInfo, nullptr, &device_) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan logical device.");
    }
}

void Device::CacheQueues()
{
    if (device_ == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Cannot cache queues from an invalid logical device.");
    }

    if (graphicsQueueFamilyIndex_)
    {
        vkGetDeviceQueue(device_, *graphicsQueueFamilyIndex_, 0, &graphicsQueue_);
    }

    if (computeQueueFamilyIndex_)
    {
        vkGetDeviceQueue(device_, *computeQueueFamilyIndex_, 0, &computeQueue_);
    }

    if (transferQueueFamilyIndex_)
    {
        vkGetDeviceQueue(device_, *transferQueueFamilyIndex_, 0, &transferQueue_);
    }
}

void Device::Destroy() noexcept
{
    if (device_ != VK_NULL_HANDLE)
    {
        vkDestroyDevice(device_, nullptr);
        device_ = VK_NULL_HANDLE;
    }

    physicalDevice_ = nullptr;
    graphicsQueueFamilyIndex_.reset();
    computeQueueFamilyIndex_.reset();
    transferQueueFamilyIndex_.reset();
    graphicsQueue_ = VK_NULL_HANDLE;
    computeQueue_ = VK_NULL_HANDLE;
    transferQueue_ = VK_NULL_HANDLE;
}

std::vector<uint32_t> Device::BuildQueueFamiliesToCreate() const
{
    std::vector<uint32_t> families;
    families.reserve(3);

    if (graphicsQueueFamilyIndex_)
    {
        families.push_back(*graphicsQueueFamilyIndex_);
    }

    if (computeQueueFamilyIndex_)
    {
        families.push_back(*computeQueueFamilyIndex_);
    }

    if (transferQueueFamilyIndex_)
    {
        families.push_back(*transferQueueFamilyIndex_);
    }

    std::sort(families.begin(), families.end());
    families.erase(std::unique(families.begin(), families.end()), families.end());
    return families;
}
