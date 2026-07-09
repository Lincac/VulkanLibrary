#pragma once

#include <cstring>

#include "matVkEngineContext.h"

namespace mat {

    class VkEngineBuffer {
    public:
        VkEngineBuffer();
        ~VkEngineBuffer();

        void setDeviceSize(VkDeviceSize size);

        void setBufferUsageFlags(VkBufferUsageFlags usage);

        void setMemoryPropertyFlags(VkMemoryPropertyFlags properties);

        template <typename T>
        void create(const VkEngineContext& context, T* data);

        VkBuffer& getBuffer();

        VkDeviceMemory& getDeviceMemory();

        VkDeviceAddress getDeviceAddress(VkDevice logDevice);

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

    template <typename T>
    inline void VkEngineBuffer::create(const VkEngineContext& context, T* data) {
        if (_size == 0 || _usage == 0) {
            VK_ERROR("Buffer size & usage is not meet the criteria!");
        }

        if (data == nullptr) {
            VK_ERROR("Buffer upload data is null");
        }

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(context.getPhysicalDevice(), context.getDevice(), _size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
                     stagingBufferMemory);

        void* temp;
        vkMapMemory(context.getDevice(), stagingBufferMemory, 0, _size, 0, &temp);
        std::memcpy(temp, data, (size_t)_size);
        vkUnmapMemory(context.getDevice(), stagingBufferMemory);

        createBuffer(context.getPhysicalDevice(), context.getDevice(), _size, _usage, _memoryProperties, _buffer,
                     _memory);

        copyBuffer(context.getDevice(), context.getGraphicsQueue(), context.getVkCommandPool(), stagingBuffer, _buffer,
                   _size);

        vkDestroyBuffer(context.getDevice(), stagingBuffer, nullptr);
        vkFreeMemory(context.getDevice(), stagingBufferMemory, nullptr);
    }

};  // namespace mat