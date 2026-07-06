#include "matVkEngineBuffer.h"

namespace mat {

    VkEngineBuffer::VkEngineBuffer() {
        _size = 0;
        _usage = 0;
        _memoryProperties = 0;
    }

    VkEngineBuffer::~VkEngineBuffer() {}

    void VkEngineBuffer::setVkDeviceSize(VkDeviceSize size) {
        _size = size;
    }

    void VkEngineBuffer::setVkBufferUsageFlags(VkBufferUsageFlags usage) {
        _usage = usage;
    }

    void VkEngineBuffer::setVkMemoryPropertyFlags(VkMemoryPropertyFlags properties) {
        _memoryProperties = properties;
    }

    void VkEngineBuffer::create(VkPhysicalDevice device, VkDevice logDevice) {
        if (_size == 0 || _usage == 0) {
            VK_ERROR("Buffer size & usage is not meet the criteria!");
        }

        VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bufferInfo.size = _size;
        bufferInfo.usage = _usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VK_CHECK(vkCreateBuffer(logDevice, &bufferInfo, nullptr, &_buffer));

        VkMemoryRequirements memReq{};
        vkGetBufferMemoryRequirements(logDevice, _buffer, &memReq);

        VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex = findMemoryType(device, memReq.memoryTypeBits, _memoryProperties);

        VkMemoryAllocateFlagsInfo allocFlags{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO};

        if (_usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
            allocFlags.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
            allocInfo.pNext = &allocFlags;
        }

        VK_CHECK(vkAllocateMemory(logDevice, &allocInfo, nullptr, &_memory));

        vkBindBufferMemory(logDevice, _buffer, _memory, 0);
    }

    VkBuffer& VkEngineBuffer::getVkBuffer() {
        return _buffer;
    }

    VkDeviceMemory& VkEngineBuffer::getVkDeviceMemory() {
        return _memory;
    }

    VkDeviceAddress VkEngineBuffer::getVkDeviceAddress(VkDevice logDevice) {
        VkBufferDeviceAddressInfo info{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
        info.buffer = _buffer;

        return vkGetBufferDeviceAddress(logDevice, &info);
    }

    void VkEngineBuffer::release(VkDevice logDevice) {
        if (_buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(logDevice, _buffer, nullptr);
            _buffer = VK_NULL_HANDLE;
        }

        if (_memory != VK_NULL_HANDLE) {
            vkFreeMemory(logDevice, _memory, nullptr);
            _memory = VK_NULL_HANDLE;
        }
    }

};  // namespace mat