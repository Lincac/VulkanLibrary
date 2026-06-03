#include "vkEngineBuffer.h"

vkEngineBuffer::vkEngineBuffer(std::shared_ptr<vkEngineLogicalDevice> device)
    : _device(device)
{
}

vkEngineBuffer::~vkEngineBuffer()
{
    auto device = _device->getVkDevice();

    if (_buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, _buffer, nullptr);
        _buffer = VK_NULL_HANDLE;
    }

    if (_memory != VK_NULL_HANDLE) {
        vkFreeMemory(device, _memory, nullptr);
        _memory = VK_NULL_HANDLE;
    }
}

void vkEngineBuffer::setSize(VkDeviceSize size)
{
    _size = size;
}

void vkEngineBuffer::setUsage(VkBufferUsageFlags usage)
{
    _usage = usage;
}

void vkEngineBuffer::setMemoryProperties(VkMemoryPropertyFlags properties)
{
    _memoryProperties = properties;
}

void vkEngineBuffer::create()
{
    if (_size == 0 || _usage == 0) {
        throw std::runtime_error("buffer size/usage not set!");
    }

    auto physicalDevice = _device->getPhysicalDevice()->getVkPhysicalDevice();
    auto device = _device->getVkDevice();

    VkBufferCreateInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size = _size;
    bufferInfo.usage = _usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &_buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memReq{};
    vkGetBufferMemoryRequirements(device, _buffer, &memReq);

    VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = findMemoryType(
        physicalDevice,
        memReq.memoryTypeBits,
        _memoryProperties);

    // 如果 buffer 带了 SHADER_DEVICE_ADDRESS，内存也要带这个 flag
    VkMemoryAllocateFlagsInfo allocFlags{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO };
    
    if (_usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        allocFlags.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
        allocInfo.pNext = &allocFlags;
    }

    if (vkAllocateMemory(device, &allocInfo, nullptr, &_memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device, _buffer, _memory, 0);    
}

void vkEngineBuffer::upload(std::shared_ptr<vkEngineCommandPool> commandPool, const void *data, VkDeviceSize size)
{
    if (size > _size) {
        throw std::runtime_error("upload size exceeds buffer size!");
    }

    auto device = _device->getVkDevice();
    auto physDev = _device->getPhysicalDevice()->getVkPhysicalDevice();

    // --- 1. 创建 staging buffer ---
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

    VkBufferCreateInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer);

    VkMemoryRequirements memReq{};
    vkGetBufferMemoryRequirements(device, stagingBuffer, &memReq);

    VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = findMemoryType(
        physDev, memReq.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    vkAllocateMemory(device, &allocInfo, nullptr, &stagingMemory);

    vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0);

    // --- 2. CPU 写入 staging ---
    void* mapped = nullptr;
    vkMapMemory(device, stagingMemory, 0, size, 0, &mapped);
    memcpy(mapped, data, static_cast<size_t>(size));
    vkUnmapMemory(device, stagingMemory);

    // --- 3. GPU copy ---
    commandPool->submitOneTimeCommands([&](VkCommandBuffer cmd) {
        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(cmd, stagingBuffer, _buffer, 1, &copyRegion);
    });

    // --- 4. 销毁 staging（临时资源）---
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);
}

VkBuffer &vkEngineBuffer::getBuffer()
{
    return _buffer;
}

VkDeviceMemory &vkEngineBuffer::getMemory()
{
    return _memory;
}

VkDeviceAddress vkEngineBuffer::getDeviceAddress()
{
    VkBufferDeviceAddressInfo info{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
    info.buffer = _buffer;

    return vkGetBufferDeviceAddress(_device->getVkDevice(), &info);
}