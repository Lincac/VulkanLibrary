#pragma once

#include <Volk/volk.h>

namespace mat {

    enum class ImageShaderDomain {
        Compute,
        Fragment,
        ColorAttachment,
        RayTracing,
    };

    uint32_t findMemoryType(VkPhysicalDevice physDev, uint32_t typeFilter, VkMemoryPropertyFlags properties);

    void transitionImageLayout(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
                               ImageShaderDomain shaderDomain = ImageShaderDomain::Compute);

}  // namespace mat
