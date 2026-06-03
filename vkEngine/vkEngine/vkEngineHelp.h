#pragma once

#include <Volk/volk.h>

#include <string>
#include <vector>
#include <stdexcept>
#include <optional>
#include <iostream>
#include <set>
#include <fstream>
#include <functional>

#ifdef _WIN32
#include <Windows.h>
#endif

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
    else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }

    vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0,
        0, nullptr, 0, nullptr, 1, &barrier);
}

/// @brief 转换变换矩阵
/// @param m 变换矩阵
/// @return 变换矩阵
inline VkTransformMatrixKHR toVkTransform(const glm::mat4& m) 
{
    VkTransformMatrixKHR t{};
    t.matrix[0][0] = m[0][0]; t.matrix[0][1] = m[1][0]; t.matrix[0][2] = m[2][0]; t.matrix[0][3] = m[3][0];
    t.matrix[1][0] = m[0][1]; t.matrix[1][1] = m[1][1]; t.matrix[1][2] = m[2][1]; t.matrix[1][3] = m[3][1];
    t.matrix[2][0] = m[0][2]; t.matrix[2][1] = m[1][2]; t.matrix[2][2] = m[2][2]; t.matrix[2][3] = m[3][2];
    return t;
}

/// @brief 读取文件
/// @param filename 文件名
/// @return 文件内容
inline std::vector<char> readFile(const std::string& filename) 
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()){
        throw std::runtime_error("failed to open file: " + filename);
    }

    size_t size = file.tellg();
    std::vector<char> buffer(size);
    file.seekg(0);
    file.read(buffer.data(), size);

    return buffer;
}

/// @brief 解析路径
/// @param relativePath 相对路径
/// @return 绝对路径
inline std::string resolvePathNextToExe(const std::string& relativePath)
{
    std::ifstream direct(relativePath, std::ios::binary);
    if (direct.good()) {
        return relativePath;
    }

#ifdef _WIN32
    char exePath[MAX_PATH]{};
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    std::string fullPath = exePath;
    const size_t slash = fullPath.find_last_of("\\/");
    if (slash != std::string::npos) {
        fullPath = fullPath.substr(0, slash + 1) + relativePath;
        std::ifstream resolved(fullPath, std::ios::binary);
        if (resolved.good()) {
            return fullPath;
        }
    }
#endif

    throw std::runtime_error("failed to resolve file path: " + relativePath);
}

/// @brief 获取可执行文件所在目录
/// @param relativePath 相对路径
/// @return 绝对路径
inline std::string pathNextToExe(const std::string& relativePath)
{
#ifdef _WIN32
    char exePath[MAX_PATH]{};
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    std::string fullPath = exePath;
    const size_t slash = fullPath.find_last_of("\\/");
    if (slash != std::string::npos) {
        return fullPath.substr(0, slash + 1) + relativePath;
    }
#endif
    return relativePath;
}