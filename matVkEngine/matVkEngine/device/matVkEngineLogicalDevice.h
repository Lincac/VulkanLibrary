#pragma once

#include "device/matVkEnginePhysicalDevice.h"

namespace mat {

    class VkEngineLogicalDevice {
    public:
        VkEngineLogicalDevice(std::shared_ptr<VkEnginePhysicalDevice> physicalDevice);
        ~VkEngineLogicalDevice();

        VkDevice getVkDevice() const;

        VkQueue getGraphicsQueue() const;

    private:
        VkEngineLogicalDevice(const VkEngineLogicalDevice&) = delete;
        VkEngineLogicalDevice(VkEngineLogicalDevice&&) = delete;
        VkEngineLogicalDevice& operator=(const VkEngineLogicalDevice&) = delete;
        VkEngineLogicalDevice& operator=(VkEnginePhysicalDevice&&) = delete;

        VkDevice _device = VK_NULL_HANDLE;
        VkQueue _graphicsQueue = VK_NULL_HANDLE;
    };

};  // namespace mat
