#pragma once

#include "device/matVkEngineCmdPool.h"
#include "device/matVkEngineLogicalDevice.h"

namespace mat {

    class VkEngineBuffer {
    public:
        VkEngineBuffer();
        ~VkEngineBuffer();

        void setVkDeviceSize(VkDeviceSize size);

        void setVkBufferUsageFlags(VkBufferUsageFlags usage);

        void setVkMemoryPropertyFlags(VkMemoryPropertyFlags properties);

        void create(std::shared_ptr<VkEnginePhysicalDevice> physicalDevice,
                    std::shared_ptr<VkEngineLogicalDevice> logicalDevice);

        VkBuffer& getVkBuffer();

        VkDeviceMemory& getVkDeviceMemory();

        VkDeviceAddress getVkDeviceAddress(std::shared_ptr<VkEngineLogicalDevice> logicalDevice);

        void release(std::shared_ptr<VkEngineLogicalDevice> logicalDevice);

    private:
        VkEngineBuffer(const VkEngineBuffer&) = delete;
        VkEngineBuffer(VkEngineBuffer&&) = delete;
        VkEngineBuffer& operator=(const VkEngineBuffer&) = delete;
        VkEngineBuffer& operator=(VkEngineBuffer&&) = delete;

        VkBuffer _buffer = VK_NULL_HANDLE;
        VkDeviceMemory _memory = VK_NULL_HANDLE;

        VkDeviceSize _size;
        VkBufferUsageFlags _usage;
        VkMemoryPropertyFlags _memoryProperties;
    };

};  // namespace mat