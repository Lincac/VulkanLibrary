#pragma once

#include <functional>

#include "device/matVkEngineLogicalDevice.h"

namespace mat {

    class VkEngineCmdPool {
    public:
        explicit VkEngineCmdPool(std::shared_ptr<VkEnginePhysicalDevice> physicalDevice,
                                 std::shared_ptr<VkEngineLogicalDevice> logicalDevice);
        ~VkEngineCmdPool();

        void release(std::shared_ptr<VkEngineLogicalDevice> logicalDevice);

        void submitOneTimeCommands(std::shared_ptr<VkEngineLogicalDevice> logicalDevice,
                                   std::function<void(VkCommandBuffer)> recordFunc);

    private:
        VkEngineCmdPool(const VkEngineCmdPool&) = delete;
        VkEngineCmdPool(VkEngineCmdPool&&) = delete;
        VkEngineCmdPool& operator=(const VkEngineCmdPool&) = delete;
        VkEngineCmdPool& operator=(VkEngineCmdPool&&) = delete;

        VkCommandPool _commandPool = VK_NULL_HANDLE;
        std::vector<VkCommandBuffer> _commandBuffers;
    };

};  // namespace mat
