#include "matVkEngineCmdPool.h"

#include <stdexcept>

namespace mat {

    VkEngineCmdPool::VkEngineCmdPool(std::shared_ptr<VkEnginePhysicalDevice> physicalDevice,
                                     std::shared_ptr<VkEngineLogicalDevice> logicalDevice) {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = physicalDevice->getGraphicsFamily().value();

        if (vkCreateCommandPool(logicalDevice->getVkDevice(), &poolInfo, nullptr, &_commandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }

        _commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = _commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)_commandBuffers.size();

        if (vkAllocateCommandBuffers(logicalDevice->getVkDevice(), &allocInfo, _commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    VkEngineCmdPool::~VkEngineCmdPool() {}

    void VkEngineCmdPool::release(std::shared_ptr<VkEngineLogicalDevice> logicalDevice) {
        if (_commandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(logicalDevice->getVkDevice(), _commandPool, nullptr);
            _commandPool = VK_NULL_HANDLE;
        }

        _commandBuffers.clear();
    }

    void VkEngineCmdPool::submitOneTimeCommands(std::shared_ptr<VkEngineLogicalDevice> logicalDevice,
                                                std::function<void(VkCommandBuffer)> recordFunc) {
        VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        VkFence fence;
        vkCreateFence(logicalDevice->getVkDevice(), &fenceInfo, nullptr, &fence);

        VkCommandBufferAllocateInfo cmdAlloc{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        cmdAlloc.commandPool = _commandPool;
        cmdAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdAlloc.commandBufferCount = 1;

        VkCommandBuffer cmd;
        vkAllocateCommandBuffers(logicalDevice->getVkDevice(), &cmdAlloc, &cmd);

        VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(cmd, &beginInfo);

        recordFunc(cmd);

        vkEndCommandBuffer(cmd);

        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &cmd;

        vkQueueSubmit(logicalDevice->getGraphicsQueue(), 1, &submit, fence);
        vkWaitForFences(logicalDevice->getVkDevice(), 1, &fence, VK_TRUE, UINT64_MAX);

        vkFreeCommandBuffers(logicalDevice->getVkDevice(), _commandPool, 1, &cmd);
        vkDestroyFence(logicalDevice->getVkDevice(), fence, nullptr);
    }

};  // namespace mat