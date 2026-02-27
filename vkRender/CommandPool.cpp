#include "CommandPool.h"

#include <stdexcept>

bool CommandPool::isInitialized() const {
    return device != VK_NULL_HANDLE &&
           graphicsQueue != VK_NULL_HANDLE &&
           commandPool != VK_NULL_HANDLE;
}

void CommandPool::initialize(
    VkDevice inDevice,
    uint32_t inGraphicsQueueFamily,
    VkQueue inGraphicsQueue,
    VkCommandPoolCreateFlags flags) {
    if (inDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("CommandPool::initialize: device is null");
    }
    if (inGraphicsQueue == VK_NULL_HANDLE) {
        throw std::runtime_error("CommandPool::initialize: graphics queue is null");
    }

    if (commandPool != VK_NULL_HANDLE) {
        cleanup();
    }

    device = inDevice;
    graphicsQueue = inGraphicsQueue;

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = flags;
    poolInfo.queueFamilyIndex = inGraphicsQueueFamily;

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        device = VK_NULL_HANDLE;
        graphicsQueue = VK_NULL_HANDLE;
        throw std::runtime_error("Failed to create command pool");
    }
}

void CommandPool::cleanup() {
    if (device != VK_NULL_HANDLE && commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device, commandPool, nullptr);
    }

    commandPool = VK_NULL_HANDLE;
    graphicsQueue = VK_NULL_HANDLE;
    device = VK_NULL_HANDLE;
}

VkCommandBuffer CommandPool::allocateCommandBuffer(VkCommandBufferLevel level) const {
    if (!isInitialized()) {
        throw std::runtime_error("CommandPool::allocateCommandBuffer: not initialized");
    }

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = level;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffer");
    }

    return commandBuffer;
}

std::vector<VkCommandBuffer> CommandPool::allocateCommandBuffers(
    uint32_t count,
    VkCommandBufferLevel level) const {
    if (!isInitialized()) {
        throw std::runtime_error("CommandPool::allocateCommandBuffers: not initialized");
    }
    if (count == 0) {
        return {};
    }

    std::vector<VkCommandBuffer> commandBuffers(count, VK_NULL_HANDLE);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = level;
    allocInfo.commandBufferCount = count;

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers");
    }

    return commandBuffers;
}

void CommandPool::freeCommandBuffer(VkCommandBuffer commandBuffer) const {
    if (!isInitialized()) {
        throw std::runtime_error("CommandPool::freeCommandBuffer: not initialized");
    }
    if (commandBuffer == VK_NULL_HANDLE) {
        return;
    }

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void CommandPool::freeCommandBuffers(const std::vector<VkCommandBuffer>& commandBuffers) const {
    if (!isInitialized()) {
        throw std::runtime_error("CommandPool::freeCommandBuffers: not initialized");
    }
    if (commandBuffers.empty()) {
        return;
    }

    vkFreeCommandBuffers(
        device,
        commandPool,
        static_cast<uint32_t>(commandBuffers.size()),
        commandBuffers.data());
}

VkCommandBuffer CommandPool::beginSingleTimeCommands() const {
    VkCommandBuffer commandBuffer = allocateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        freeCommandBuffer(commandBuffer);
        throw std::runtime_error("Failed to begin single-time command buffer");
    }

    return commandBuffer;
}

void CommandPool::endSingleTimeCommands(VkCommandBuffer commandBuffer) const {
    if (!isInitialized()) {
        throw std::runtime_error("CommandPool::endSingleTimeCommands: not initialized");
    }
    if (commandBuffer == VK_NULL_HANDLE) {
        return;
    }

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        freeCommandBuffer(commandBuffer);
        throw std::runtime_error("Failed to end single-time command buffer");
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        freeCommandBuffer(commandBuffer);
        throw std::runtime_error("Failed to submit single-time command buffer");
    }

    if (vkQueueWaitIdle(graphicsQueue) != VK_SUCCESS) {
        freeCommandBuffer(commandBuffer);
        throw std::runtime_error("Failed to wait graphics queue idle");
    }

    freeCommandBuffer(commandBuffer);
}
