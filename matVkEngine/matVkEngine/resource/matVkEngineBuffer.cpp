#include "matVkEngineBuffer.h"

#include <stdexcept>

#include "common/matVkEngineCommon.h"

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

    void VkEngineBuffer::create(std::shared_ptr<VkEnginePhysicalDevice> physicalDevice,
                                std::shared_ptr<VkEngineLogicalDevice> logicalDevice) {
        if (_size == 0 || _usage == 0) {
            throw std::runtime_error("buffer size/usage not set!");
        }

        VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bufferInfo.size = _size;
        bufferInfo.usage = _usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(logicalDevice->getVkDevice(), &bufferInfo, nullptr, &_buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }

        VkMemoryRequirements memReq{};
        vkGetBufferMemoryRequirements(logicalDevice->getVkDevice(), _buffer, &memReq);

        VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex =
            findMemoryType(physicalDevice->getVkPhysicalDevice(), memReq.memoryTypeBits, _memoryProperties);

        VkMemoryAllocateFlagsInfo allocFlags{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO};

        if (_usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
            allocFlags.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
            allocInfo.pNext = &allocFlags;
        }

        if (vkAllocateMemory(logicalDevice->getVkDevice(), &allocInfo, nullptr, &_memory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        vkBindBufferMemory(logicalDevice->getVkDevice(), _buffer, _memory, 0);
    }

    void VkEngineBuffer::upload(std::shared_ptr<VkEnginePhysicalDevice> physicalDevice,
                                std::shared_ptr<VkEngineLogicalDevice> logicalDevice,
                                std::shared_ptr<VkEngineCmdPool> cmd, const void* data, VkDeviceSize size) {
        if (size > _size) {
            throw std::runtime_error("upload size exceeds buffer size!");
        }

        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

        VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bufferInfo.size = size;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCreateBuffer(logicalDevice->getVkDevice(), &bufferInfo, nullptr, &stagingBuffer);

        VkMemoryRequirements memReq{};
        vkGetBufferMemoryRequirements(logicalDevice->getVkDevice(), stagingBuffer, &memReq);

        VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex =
            findMemoryType(physicalDevice->getVkPhysicalDevice(), memReq.memoryTypeBits,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        vkAllocateMemory(logicalDevice->getVkDevice(), &allocInfo, nullptr, &stagingMemory);

        vkBindBufferMemory(logicalDevice->getVkDevice(), stagingBuffer, stagingMemory, 0);

        void* mapped = nullptr;
        vkMapMemory(logicalDevice->getVkDevice(), stagingMemory, 0, size, 0, &mapped);
        memcpy(mapped, data, static_cast<size_t>(size));
        vkUnmapMemory(logicalDevice->getVkDevice(), stagingMemory);

        cmd->submitOneTimeCommands(logicalDevice, [&](VkCommandBuffer cmd) {
            VkBufferCopy copyRegion{};
            copyRegion.size = size;
            vkCmdCopyBuffer(cmd, stagingBuffer, _buffer, 1, &copyRegion);
        });

        vkDestroyBuffer(logicalDevice->getVkDevice(), stagingBuffer, nullptr);
        vkFreeMemory(logicalDevice->getVkDevice(), stagingMemory, nullptr);
    }

    VkBuffer& VkEngineBuffer::getVkBuffer() {
        return _buffer;
    }

    VkDeviceMemory& VkEngineBuffer::getVkDeviceMemory() {
        return _memory;
    }

    VkDeviceAddress VkEngineBuffer::getVkDeviceAddress(std::shared_ptr<VkEngineLogicalDevice> logicalDevice) {
        VkBufferDeviceAddressInfo info{VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
        info.buffer = _buffer;

        return vkGetBufferDeviceAddress(logicalDevice->getVkDevice(), &info);
    }

    void VkEngineBuffer::release(std::shared_ptr<VkEngineLogicalDevice> logicalDevice) {
        if (logicalDevice == nullptr) {
            throw std::runtime_error("Logical Device is nullptr!");
        }

        if (_buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(logicalDevice->getVkDevice(), _buffer, nullptr);
            _buffer = VK_NULL_HANDLE;
        }

        if (_memory != VK_NULL_HANDLE) {
            vkFreeMemory(logicalDevice->getVkDevice(), _memory, nullptr);
            _memory = VK_NULL_HANDLE;
        }
    }

};  // namespace mat