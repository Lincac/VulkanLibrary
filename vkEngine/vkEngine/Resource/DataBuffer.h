#pragma once

#include <volk/volk.h>
#include <glm/glm.hpp>

#include <array>
#include <vector>

struct Vertex {
    glm::vec3 position;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, position);

        return attributeDescriptions;
    }
};

template <typename T>
class DataBuffer
{

public:

    DataBuffer(VkDevice device, VkPhysicalDevice physicalDevice);

    void setData(T* data, unsigned int size);

    void setData(const std::vector<T>& data);

    int create();

private:

    void createBuffer(
        VkDeviceSize size, 
        VkBufferUsageFlags usage, 
        VkMemoryPropertyFlags properties, 
        VkBuffer& buffer, 
        VkDeviceMemory& bufferMemory);

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

private:

    std::vector<T> _data;

    VkDevice _logicalDevice;
    VkPhysicalDevice _physicalDevice;

    VkBuffer _buffer;
    VkDeviceMemory _deviceMemory;

};

