#include "PhysicalDevice.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>

PhysicalDevice::PhysicalDevice(
    VkInstance instance,
    std::vector<const char*> requiredExtensions,
    VkPhysicalDeviceType preferredType)
    : instance_(instance),
      requiredExtensions_(std::move(requiredExtensions))
{
    if (instance_ == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Cannot select physical device from a null Vulkan instance.");
    }

    SelectPhysicalDevice(instance_, preferredType);
    CacheCapabilities();
}

VkPhysicalDevice PhysicalDevice::Get() const noexcept
{
    return device_;
}

bool PhysicalDevice::IsValid() const noexcept
{
    return device_ != VK_NULL_HANDLE;
}

const VkPhysicalDeviceProperties& PhysicalDevice::GetProperties() const noexcept
{
    return properties_;
}

const VkPhysicalDeviceFeatures& PhysicalDevice::GetFeatures() const noexcept
{
    return features_;
}

const VkPhysicalDeviceMemoryProperties& PhysicalDevice::GetMemoryProperties() const noexcept
{
    return memoryProperties_;
}

const std::vector<VkQueueFamilyProperties>& PhysicalDevice::GetQueueFamilyProperties() const noexcept
{
    return queueFamilyProperties_;
}

const std::vector<VkExtensionProperties>& PhysicalDevice::GetSupportedExtensions() const noexcept
{
    return supportedExtensions_;
}

bool PhysicalDevice::SupportsExtensions(const std::vector<const char*>& extensions) const
{
    for (const char* extensionName : extensions)
    {
        const bool found = std::any_of(
            supportedExtensions_.begin(),
            supportedExtensions_.end(),
            [extensionName](const VkExtensionProperties& extensionProperties)
            {
                return std::strcmp(extensionProperties.extensionName, extensionName) == 0;
            });

        if (!found)
        {
            return false;
        }
    }

    return true;
}

std::optional<uint32_t> PhysicalDevice::FindQueueFamily(VkQueueFlags requiredFlags) const
{
    for (uint32_t familyIndex = 0; familyIndex < static_cast<uint32_t>(queueFamilyProperties_.size()); ++familyIndex)
    {
        const VkQueueFamilyProperties& queueFamily = queueFamilyProperties_[familyIndex];
        if (queueFamily.queueCount > 0 && (queueFamily.queueFlags & requiredFlags) == requiredFlags)
        {
            return familyIndex;
        }
    }

    return std::nullopt;
}

std::vector<VkPhysicalDevice> PhysicalDevice::EnumeratePhysicalDevices(VkInstance instance)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    std::vector<VkPhysicalDevice> devices(deviceCount);
    if (deviceCount > 0)
    {
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    }

    return devices;
}

void PhysicalDevice::SelectPhysicalDevice(VkInstance instance, VkPhysicalDeviceType preferredType)
{
    const auto devices = EnumeratePhysicalDevices(instance);
    if (devices.empty())
    {
        throw std::runtime_error("No Vulkan physical devices found.");
    }

    int bestScore = std::numeric_limits<int>::min();
    VkPhysicalDevice bestDevice = VK_NULL_HANDLE;

    for (VkPhysicalDevice candidate : devices)
    {
        if (!IsDeviceSuitable(candidate))
        {
            continue;
        }

        const int score = ScorePhysicalDevice(candidate, preferredType);
        if (score > bestScore)
        {
            bestScore = score;
            bestDevice = candidate;
        }
    }

    if (bestDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("No suitable Vulkan physical device satisfies requirements.");
    }

    device_ = bestDevice;
}

void PhysicalDevice::CacheCapabilities()
{
    if (device_ == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Cannot query capabilities from an invalid physical device.");
    }

    vkGetPhysicalDeviceProperties(device_, &properties_);
    vkGetPhysicalDeviceFeatures(device_, &features_);
    vkGetPhysicalDeviceMemoryProperties(device_, &memoryProperties_);

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device_, &queueFamilyCount, nullptr);
    queueFamilyProperties_.resize(queueFamilyCount);
    if (queueFamilyCount > 0)
    {
        vkGetPhysicalDeviceQueueFamilyProperties(device_, &queueFamilyCount, queueFamilyProperties_.data());
    }

    supportedExtensions_ = EnumerateDeviceExtensions(device_);
}

bool PhysicalDevice::IsDeviceSuitable(VkPhysicalDevice device) const
{
    for (const char* extensionName : requiredExtensions_)
    {
        if (!IsExtensionSupported(device, extensionName))
        {
            return false;
        }
    }

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    if (queueFamilyCount == 0)
    {
        return false;
    }

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    const bool hasGraphicsQueue = std::any_of(
        queueFamilies.begin(),
        queueFamilies.end(),
        [](const VkQueueFamilyProperties& queueFamily)
        {
            return queueFamily.queueCount > 0 && (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
        });

    return hasGraphicsQueue;
}

bool PhysicalDevice::IsExtensionSupported(VkPhysicalDevice device, const char* extensionName) const
{
    const auto extensions = EnumerateDeviceExtensions(device);
    return std::any_of(
        extensions.begin(),
        extensions.end(),
        [extensionName](const VkExtensionProperties& extensionProperties)
        {
            return std::strcmp(extensionProperties.extensionName, extensionName) == 0;
        });
}

int PhysicalDevice::ScorePhysicalDevice(VkPhysicalDevice device, VkPhysicalDeviceType preferredType) const
{
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(device, &properties);

    int score = 0;

    if (properties.deviceType == preferredType)
    {
        score += 1000;
    }

    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        score += 500;
    }
    else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
    {
        score += 250;
    }

    score += static_cast<int>(properties.limits.maxImageDimension2D);

    return score;
}

std::vector<VkExtensionProperties> PhysicalDevice::EnumerateDeviceExtensions(VkPhysicalDevice device) const
{
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> extensions(extensionCount);
    if (extensionCount > 0)
    {
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());
    }

    return extensions;
}
