#include "Device.h"

#include <set>
#include <stdexcept>

Device::Device()
{
    _physicalDevice = nullptr;

    _device = VK_NULL_HANDLE;

    _graphicsQueue = VK_NULL_HANDLE;
    _presentQueue = VK_NULL_HANDLE;
}

void Device::setDependice(PhysicalDevice* physicalDevice)
{
    if (physicalDevice == nullptr)
    {
        return;
    }

    _physicalDevice = physicalDevice;
}

int Device::create()
{
    if (_physicalDevice == nullptr)
    {
        return -1;
    }

    auto indices = _physicalDevice->getQueueFamilyIndices();

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

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;

    auto deviceExtensions = _physicalDevice->getDeviceExtensions();

    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (vkCreateDevice(_physicalDevice->getPhysicalDevice(), &createInfo, nullptr, &_device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(_device, indices.graphicsFamily.value(), 0, &_graphicsQueue);
    vkGetDeviceQueue(_device, indices.presentFamily.value(), 0, &_presentQueue);
}

VkDevice Device::getDevice() const
{
    return _device;
}

PhysicalDevice* Device::getPhysicalDevice() const
{
    return _physicalDevice;
}

VkQueue Device::getGraphicsQueue() const
{
    return _graphicsQueue;
}

VkQueue Device::getPresentQueue() const
{
    return _presentQueue;
}
