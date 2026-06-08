#include "vkEnginePhysicalDevice.h"

vkEnginePhysicalDevice::vkEnginePhysicalDevice(std::shared_ptr<vkEngine> engine)
    : _engine(engine)
{
    if(engine->getInstance() == VK_NULL_HANDLE){
        std::cerr << "vk instance is not create!" << std::endl;
    }

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(engine->getInstance(), &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(engine->getInstance(), &deviceCount, devices.data());

    for (const auto& device : devices) {
        if (isDeviceSuitable(device)) {
            _physicalDevice = device;
            break;
        }
    }

    if (_physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }

    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProps{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR
    };
    VkPhysicalDeviceProperties2 props2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
    props2.pNext = &rtProps;
    vkGetPhysicalDeviceProperties2(_physicalDevice, &props2);
}

vkEnginePhysicalDevice::~vkEnginePhysicalDevice()
{
    _physicalDevice = VK_NULL_HANDLE;
}

std::shared_ptr<vkEngine> vkEnginePhysicalDevice::getEngine()
{
    return _engine;
}

VkPhysicalDevice &vkEnginePhysicalDevice::getVkPhysicalDevice()
{
    return _physicalDevice;
}

QueueFamilyIndices vkEnginePhysicalDevice::findQueueFamilies()
{
    if(_physicalDevice == VK_NULL_HANDLE){
        std::cerr << "this physical device is nullptr" << std::endl;
    }

    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }

        i++;
    }

    return indices;
}

bool vkEnginePhysicalDevice::isDeviceSuitable(VkPhysicalDevice device)
{
    if(!findQueueFamilies(device).isComplete()){
        return false;
    }

    if(!checkDeviceExtensionSupport(device)){
        return false;
    }

    // 检查设备扩展支持
    VkPhysicalDeviceVulkan12Features vulkan12Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
    VkPhysicalDeviceRayQueryFeaturesKHR rqFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR };
    VkPhysicalDeviceBufferDeviceAddressFeatures bdaFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES };
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR };
    VkPhysicalDeviceAccelerationStructureFeaturesKHR asFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR };

    bdaFeatures.pNext = &vulkan12Features;
    rqFeatures.pNext = &bdaFeatures;
    rtFeatures.pNext = &rqFeatures;
    asFeatures.pNext = &rtFeatures;

    // 创建物理设备特性2
    VkPhysicalDeviceFeatures2 features2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
    features2.pNext = &asFeatures;

    // 获取物理设备特性2
    vkGetPhysicalDeviceFeatures2(device, &features2);
    
    return asFeatures.accelerationStructure
        && rtFeatures.rayTracingPipeline
        && rqFeatures.rayQuery
        && bdaFeatures.bufferDeviceAddress
        && vulkan12Features.scalarBlockLayout;
}

QueueFamilyIndices vkEnginePhysicalDevice::findQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }

        i++;
    }

    return indices;
}

bool vkEnginePhysicalDevice::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(_deviceExtensions.begin(), _deviceExtensions.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}