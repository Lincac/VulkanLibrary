#pragma once

#include <volk.h>
#include <vector>

class CommandPool {
public:
    // Create command pool and cache queue/device handles.
    void initialize(
        VkDevice inDevice,
        uint32_t inGraphicsQueueFamily,
        VkQueue inGraphicsQueue,
        VkCommandPoolCreateFlags flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    // Destroy command pool and reset internal handles.
    void cleanup();

    // Allocate one command buffer from the pool.
    VkCommandBuffer allocateCommandBuffer(
        VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const;

    // Allocate multiple command buffers from the pool.
    std::vector<VkCommandBuffer> allocateCommandBuffers(
        uint32_t count,
        VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) const;

    // Free one command buffer back to the pool.
    void freeCommandBuffer(VkCommandBuffer commandBuffer) const;

    // Free multiple command buffers back to the pool.
    void freeCommandBuffers(const std::vector<VkCommandBuffer>& commandBuffers) const;

    // Begin a short-lived command buffer for setup/copy operations.
    VkCommandBuffer beginSingleTimeCommands() const;

    // End, submit to graphics queue, wait idle, then free the command buffer.
    void endSingleTimeCommands(VkCommandBuffer commandBuffer) const;

    VkCommandPool getCommandPool() const { return commandPool; }
    VkDevice getDevice() const { return device; }
    VkQueue getGraphicsQueue() const { return graphicsQueue; }

private:
    bool isInitialized() const;

private:
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
};
