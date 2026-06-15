#include "device/vkEngineLogicalDevice.h"

vkEngineLogicalDevice::vkEngineLogicalDevice(std::shared_ptr<vkEnginePhysicalDevice> physicalDevice)
    : _physicalDevice(physicalDevice)
{
    const auto& indices = physicalDevice->getQueueFamilies();
    const auto& req = physicalDevice->getPhysicalDeviceReq();

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        indices.graphicsFamily.value() };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceVulkan12Features vulkan12Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
    vulkan12Features.scalarBlockLayout = VK_TRUE;
    vulkan12Features.bufferDeviceAddress = VK_TRUE;

    VkPhysicalDeviceRayQueryFeaturesKHR rqFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR };
    rqFeatures.pNext = &vulkan12Features;
    rqFeatures.rayQuery = VK_TRUE;

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR };
    rtFeatures.pNext = &rqFeatures;
    rtFeatures.rayTracingPipeline = VK_TRUE;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR asFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR };
    asFeatures.pNext = &rtFeatures;
    asFeatures.accelerationStructure = VK_TRUE;

    VkPhysicalDeviceFeatures2 features2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
    features2.pNext = &asFeatures;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = nullptr;
    createInfo.pNext = &features2;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(req.extensions.size());
    createInfo.ppEnabledExtensionNames = req.extensions.data();

    if (vkCreateDevice(_physicalDevice->getVkPhysicalDevice(), &createInfo, nullptr, &_device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(_device, indices.graphicsFamily.value(), 0, &_graphicsQueue);

    volkLoadDevice(_device);
}

vkEngineLogicalDevice::~vkEngineLogicalDevice()
{
    if (_device != VK_NULL_HANDLE) {
        vkDestroyDevice(_device, nullptr);
        _device = VK_NULL_HANDLE;
    }

    _graphicsQueue = VK_NULL_HANDLE;
}

std::shared_ptr<vkEnginePhysicalDevice> vkEngineLogicalDevice::getPhysicalDevice()
{
    return _physicalDevice;
}

VkDevice& vkEngineLogicalDevice::getVkDevice()
{
    return _device;
}

VkQueue& vkEngineLogicalDevice::getGraphicsQueue()
{
    return _graphicsQueue;
}
