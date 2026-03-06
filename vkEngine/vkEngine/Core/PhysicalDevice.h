#pragma once

#include <vector>
#include <optional>

#include <volk/volk.h>

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class Device;
class Swapchain;

class PhysicalDevice
{
public:

	PhysicalDevice(VkInstance instance, VkSurfaceKHR surface);

    PhysicalDevice(const PhysicalDevice&) = delete;
    PhysicalDevice& operator=(const PhysicalDevice&) = delete;
    PhysicalDevice(PhysicalDevice&& other) noexcept = default;
    PhysicalDevice& operator=(PhysicalDevice&& other) noexcept = default;
    ~PhysicalDevice() = default;

private:

    friend class Device;
    friend class Swapchain;

    bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

    bool checkDeviceExtensionSupport(VkPhysicalDevice device);

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

private:

    const std::vector<const char*> _deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkPhysicalDevice _physicalDevice;

    QueueFamilyIndices _indices;

};

