#include "CommandPool.h"

#include <stdexcept>

CommandPool::CommandPool(const PhysicalDevice& physicalDevice, const Device& logicalDevice)
{
    QueueFamilyIndices queueFamilyIndices = physicalDevice.getQueueFamilyIndices();

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(logicalDevice.getDevice(), &poolInfo, nullptr, &_commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }

    _commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = _commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)_commandBuffers.size();

    if (vkAllocateCommandBuffers(logicalDevice.getDevice(), &allocInfo, _commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

VkCommandBuffer CommandPool::getCommandBuffer(uint32_t index)
{
    return _commandBuffers[index];
}

VkCommandBuffer* CommandPool::getCommandBuffers()
{
    return _commandBuffers.data();
}
