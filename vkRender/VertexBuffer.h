#pragma once

#include <volk.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

struct Vertex {
    float pos[2];

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription binding{};
        binding.binding = 0;
        binding.stride = sizeof(Vertex);
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return binding;
    }

    static std::array<VkVertexInputAttributeDescription, 1> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 1> attributes{};
        attributes[0].binding = 0;
        attributes[0].location = 0;
        attributes[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributes[0].offset = static_cast<uint32_t>(offsetof(Vertex, pos));
        return attributes;
    }
};

class VertexBuffer {
public:
    void initialize(
        VkDevice inDevice,
        VkPhysicalDevice inPhysicalDevice,
        const std::vector<Vertex>& vertices);

    void cleanup();

    void bind(VkCommandBuffer commandBuffer) const;

    uint32_t getVertexCount() const { return vertexCount; }

private:
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

private:
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    uint32_t vertexCount = 0;
};
