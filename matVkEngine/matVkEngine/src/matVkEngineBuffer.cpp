#include "matVkEngineBuffer.h"

namespace mat {

    VkEngineBuffer::VkEngineBuffer() {
        _size = 0;
        _usage = 0;
        _memoryProperties = 0;
    }

    VkEngineBuffer::~VkEngineBuffer() {}

    void VkEngineBuffer::setDeviceSize(VkDeviceSize size) {
        _size = size;
    }

    void VkEngineBuffer::setBufferUsageFlags(VkBufferUsageFlags usage) {
        _usage = usage;
    }

    void VkEngineBuffer::setMemoryPropertyFlags(VkMemoryPropertyFlags properties) {
        _memoryProperties = properties;
    }

    VkBuffer& VkEngineBuffer::getBuffer() {
        return _buffer;
    }

    VkDeviceMemory& VkEngineBuffer::getDeviceMemory() {
        return _memory;
    }

    VkDeviceAddress VkEngineBuffer::getDeviceAddress(VkDevice logDevice) {
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