#include "VertexBuffer.h"

#include <cstring>
#include <stdexcept>

void VertexBuffer::initialize(
    VkDevice inDevice,
    VkPhysicalDevice inPhysicalDevice,
    const std::vector<Vertex>& vertices) {
    if (inDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("VertexBuffer::initialize: device is null");
    }
    if (inPhysicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("VertexBuffer::initialize: physical device is null");
    }
    if (vertices.empty()) {
        throw std::runtime_error("VertexBuffer::initialize: vertex list is empty");
    }

    cleanup();

    device = inDevice;
    physicalDevice = inPhysicalDevice;
    vertexCount = static_cast<uint32_t>(vertices.size());
    const VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        cleanup();
        throw std::runtime_error("Failed to create vertex buffer");
    }

    VkMemoryRequirements memRequirements{};
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(
        memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &memory) != VK_SUCCESS) {
        cleanup();
        throw std::runtime_error("Failed to allocate vertex buffer memory");
    }

    if (vkBindBufferMemory(device, buffer, memory, 0) != VK_SUCCESS) {
        cleanup();
        throw std::runtime_error("Failed to bind vertex buffer memory");
    }

    void* mappedData = nullptr;
    if (vkMapMemory(device, memory, 0, bufferSize, 0, &mappedData) != VK_SUCCESS) {
        cleanup();
        throw std::runtime_error("Failed to map vertex buffer memory");
    }
    std::memcpy(mappedData, vertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(device, memory);
}

void VertexBuffer::cleanup() {
    if (device != VK_NULL_HANDLE && buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, buffer, nullptr);
    }
    if (device != VK_NULL_HANDLE && memory != VK_NULL_HANDLE) {
        vkFreeMemory(device, memory, nullptr);
    }

    buffer = VK_NULL_HANDLE;
    memory = VK_NULL_HANDLE;
    device = VK_NULL_HANDLE;
    physicalDevice = VK_NULL_HANDLE;
    vertexCount = 0;
}

void VertexBuffer::bind(VkCommandBuffer commandBuffer) const {
    if (buffer == VK_NULL_HANDLE) {
        throw std::runtime_error("VertexBuffer::bind: buffer is null");
    }

    VkBuffer vertexBuffers[] = { buffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
}

uint32_t VertexBuffer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
    VkPhysicalDeviceMemoryProperties memoryProperties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1u << i)) &&
            (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable vertex buffer memory type");
}
