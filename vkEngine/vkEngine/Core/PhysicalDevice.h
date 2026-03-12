#pragma once

#include <optional>

#include "Instance.h"

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

class PhysicalDevice
{
public:

	PhysicalDevice();

public:

    void setDependice(Instance* instance, VkSurfaceKHR surface);

    int create();

    QueueFamilyIndices getQueueFamilyIndices() const;

    VkPhysicalDevice getPhysicalDevice() const;

    VkSurfaceKHR getSurface() const;

    const std::vector<const char*> getDeviceExtensions() const;

private:

    bool isDeviceSuitable(VkPhysicalDevice device);

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    bool checkDeviceExtensionSupport(VkPhysicalDevice device);

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

private:

    Instance* _instance;
    VkSurfaceKHR _surface;

private:

    const std::vector<const char*> _deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkPhysicalDevice _physicalDevice;

    QueueFamilyIndices _indices;

};

