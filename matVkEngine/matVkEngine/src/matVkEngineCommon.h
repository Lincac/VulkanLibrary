#pragma once

#include <Volk/volk.h>

#include <string>
#include <vector>

static const char* vkResultToString(VkResult result) {
    switch (result) {
        case VK_SUCCESS:
            return "VK_SUCCESS";
        case VK_NOT_READY:
            return "VK_NOT_READY";
        case VK_TIMEOUT:
            return "VK_TIMEOUT";
        case VK_EVENT_SET:
            return "VK_EVENT_SET";
        case VK_EVENT_RESET:
            return "VK_EVENT_RESET";
        case VK_INCOMPLETE:
            return "VK_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED:
            return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST:
            return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED:
            return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT:
            return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS:
            return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTED_POOL:
            return "VK_ERROR_FRAGMENTED_POOL";
        default:
            return "UNKNOWN_ERROR";
    }
}

#define VK_CHECK(arg)                                                                                              \
    do {                                                                                                           \
        VkResult err = arg;                                                                                        \
        if (err != VK_SUCCESS) {                                                                                   \
            fprintf(stderr, "[Vulkan error]: %s (%d) at %s:%d\n", vkResultToString(err), err, __FILE__, __LINE__); \
            abort();                                                                                               \
        }                                                                                                          \
    } while (0)

#define VK_PRINT(arg)                                   \
    do {                                                \
        fprintf(stderr, "[Vulkan Message]: %s\n", arg); \
        abort();                                        \
    } while (0)

#define VK_ERROR(arg)                                 \
    do {                                              \
        fprintf(stderr, "[Vulkan ERROR]: %s\n", arg); \
        abort();                                      \
    } while (0)

namespace mat {

    uint32_t findMemoryType(VkPhysicalDevice physDev, uint32_t typeFilter, VkMemoryPropertyFlags properties);

    VkShaderModule loadShader(VkDevice device, const std::string& path);

    void createBuffer(VkPhysicalDevice device, VkDevice logDevice, VkDeviceSize size, VkBufferUsageFlags flags,
                      VkMemoryPropertyFlags memFlags, VkBuffer& buffer, VkDeviceMemory& memory);

    void copyBuffer(VkDevice logDevice, VkQueue queue, VkCommandPool cmd, VkBuffer srcBuffer, VkBuffer dstBuffer,
                    VkDeviceSize size);

}  // namespace mat
