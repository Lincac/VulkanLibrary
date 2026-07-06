#pragma once

#include "matVkEngineCommon.h"

namespace mat {

    class VkEngineBuffer {
    public:
        VkEngineBuffer();
        ~VkEngineBuffer();

        void setVkDeviceSize(VkDeviceSize size);

        void setVkBufferUsageFlags(VkBufferUsageFlags usage);

        void setVkMemoryPropertyFlags(VkMemoryPropertyFlags properties);

        void create(VkPhysicalDevice device, VkDevice logDevice);

        VkBuffer& getVkBuffer();

        VkDeviceMemory& getVkDeviceMemory();

        VkDeviceAddress getVkDeviceAddress(VkDevice logDevice);

        void release(VkDevice logDevice);

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