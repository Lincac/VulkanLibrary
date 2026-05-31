#include "vkEngineLogicalDevice.h"

#include <set>
#include <stdexcept>

vkEngineLogicalDevice::vkEngineLogicalDevice(vkEnginePhysicalDevice& physicalDevice)
    : _physicalDevice(physicalDevice)
{
    auto indices = physicalDevice.findQueueFamilies();

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        indices.graphicsFamily.value(),
        indices.presentFamily.value() };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(_physicalDevice._deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = _physicalDevice._deviceExtensions.data();

    if (vkCreateDevice(_physicalDevice._physicalDevice, &createInfo, nullptr, &_device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(_device, indices.graphicsFamily.value(), 0, &_graphicsQueue);
    vkGetDeviceQueue(_device, indices.presentFamily.value(), 0, &_presentQueue);
}

vkEngineLogicalDevice::~vkEngineLogicalDevice()
{
}

vkEnginePhysicalDevice& vkEngineLogicalDevice::getVkPhysicalDevice()
{
    return _physicalDevice;
}

VkDevice& vkEngineLogicalDevice::getVkDevice()
{
    return _device;
}