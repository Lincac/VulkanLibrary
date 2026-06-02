#pragma once

#include <Volk/volk.h>

#include <string>
#include <vector>
#include <stdexcept>
#include <optional>
#include <iostream>
#include <set>
#include <functional>

#include <glm/glm.hpp>

/// @brief 查找内存类型
/// @param physDev 物理设备
/// @param typeFilter 类型过滤
/// @param properties 内存属性
/// @return 内存类型
inline uint32_t findMemoryType(VkPhysicalDevice physDev, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProps{};
    vkGetPhysicalDeviceMemoryProperties(physDev, &memProps);

    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
        if ((typeFilter & (1u << i)) &&
            (memProps.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}

/// @brief 转换图像布局
/// @param cmd 命令缓冲区
/// @param image 图像
/// @param oldLayout 旧布局
/// @param newLayout 新布局
inline void transitionImageLayout(VkCommandBuffer cmd, VkImage image,
    VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = 0;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
    }

    vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0,
        0, nullptr, 0, nullptr, 1, &barrier);
}

/// @brief 转换变换矩阵
/// @param m 变换矩阵
/// @return 变换矩阵
inline VkTransformMatrixKHR toVkTransform(const glm::mat4& m) {
    VkTransformMatrixKHR t{};
    t.matrix[0][0] = m[0][0]; t.matrix[0][1] = m[1][0]; t.matrix[0][2] = m[2][0]; t.matrix[0][3] = m[3][0];
    t.matrix[1][0] = m[0][1]; t.matrix[1][1] = m[1][1]; t.matrix[1][2] = m[2][1]; t.matrix[1][3] = m[3][1];
    t.matrix[2][0] = m[0][2]; t.matrix[2][1] = m[1][2]; t.matrix[2][2] = m[2][2]; t.matrix[2][3] = m[3][2];
    return t;
}