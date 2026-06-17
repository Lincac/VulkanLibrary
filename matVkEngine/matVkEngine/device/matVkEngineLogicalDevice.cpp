#include "matVkEngineLogicalDevice.h"

#include <set>
#include <stdexcept>

namespace mat {

    VkEngineLogicalDevice::VkEngineLogicalDevice(std::shared_ptr<VkEnginePhysicalDevice> physicalDevice) {
        std::set<uint32_t> uniqueQueueFamilies = {physicalDevice->getGraphicsFamily().value()};

        float queuePriority = 1.0f;
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        auto physicalConfig = physicalDevice->getConfig();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(physicalConfig.extensions.size());
        createInfo.ppEnabledExtensionNames = physicalConfig.extensions.data();

        VkPhysicalDeviceFeatures2 deviceFeatures{};
        deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures.pNext = nullptr;

        createInfo.pEnabledFeatures = nullptr;
        createInfo.pNext = &deviceFeatures;

        if (vkCreateDevice(physicalDevice->getVkPhysicalDevice(), &createInfo, nullptr, &_device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }

        vkGetDeviceQueue(_device, physicalDevice->getGraphicsFamily().value(), 0, &_graphicsQueue);

        volkLoadDevice(_device);
    }

    VkEngineLogicalDevice::~VkEngineLogicalDevice() {
        if (_device != VK_NULL_HANDLE) {
            vkDestroyDevice(_device, nullptr);
            _device = VK_NULL_HANDLE;
        }

        _graphicsQueue = VK_NULL_HANDLE;
    }

    VkDevice VkEngineLogicalDevice::getVkDevice() const {
        return _device;
    }

    VkQueue VkEngineLogicalDevice::getGraphicsQueue() const {
        return _graphicsQueue;
    }

};  // namespace mat