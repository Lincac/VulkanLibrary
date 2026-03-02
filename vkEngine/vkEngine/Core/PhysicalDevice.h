#pragma once

#include <optional>
#include <vector>

#include <volk/volk.h>

class PhysicalDevice
{
public:
    PhysicalDevice() = default;
    PhysicalDevice(
        VkInstance instance,
        std::vector<const char*> requiredExtensions = {},
        VkPhysicalDeviceType preferredType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);

    VkPhysicalDevice Get() const noexcept;
    bool IsValid() const noexcept;

    const VkPhysicalDeviceProperties& GetProperties() const noexcept;
    const VkPhysicalDeviceFeatures& GetFeatures() const noexcept;
    const VkPhysicalDeviceMemoryProperties& GetMemoryProperties() const noexcept;
    const std::vector<VkQueueFamilyProperties>& GetQueueFamilyProperties() const noexcept;
    const std::vector<VkExtensionProperties>& GetSupportedExtensions() const noexcept;

    bool SupportsExtensions(const std::vector<const char*>& extensions) const;
    std::optional<uint32_t> FindQueueFamily(VkQueueFlags requiredFlags) const;

    static std::vector<VkPhysicalDevice> EnumeratePhysicalDevices(VkInstance instance);

private:
    void SelectPhysicalDevice(VkInstance instance, VkPhysicalDeviceType preferredType);
    void CacheCapabilities();
    bool IsDeviceSuitable(VkPhysicalDevice device) const;
    bool IsExtensionSupported(VkPhysicalDevice device, const char* extensionName) const;
    int ScorePhysicalDevice(VkPhysicalDevice device, VkPhysicalDeviceType preferredType) const;
    std::vector<VkExtensionProperties> EnumerateDeviceExtensions(VkPhysicalDevice device) const;

private:
    VkInstance instance_{VK_NULL_HANDLE};
    VkPhysicalDevice device_{VK_NULL_HANDLE};
    std::vector<const char*> requiredExtensions_{};

    VkPhysicalDeviceProperties properties_{};
    VkPhysicalDeviceFeatures features_{};
    VkPhysicalDeviceMemoryProperties memoryProperties_{};
    std::vector<VkQueueFamilyProperties> queueFamilyProperties_{};
    std::vector<VkExtensionProperties> supportedExtensions_{};
};

