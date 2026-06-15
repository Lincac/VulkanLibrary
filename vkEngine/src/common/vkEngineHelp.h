#pragma once

#include <Volk/volk.h>

#include <string>
#include <vector>
#include <stdexcept>
#include <optional>
#include <iostream>
#include <set>
#include <fstream>
#include <sstream>
#include <functional>
#include <cstddef>
#include <cstring>

#ifdef _WIN32
#include <Windows.h>
#endif

#include <glm/glm.hpp>

/// @brief 查找内存类型
/// @param physDev 物理设备
/// @param typeFilter 类型过滤
/// @param properties 内存属性
/// @return 内存类型
inline uint32_t findMemoryType(VkPhysicalDevice physDev, uint32_t typeFilter, VkMemoryPropertyFlags properties){
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
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
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

/// @brief OBJ 顶点（pos 供 BLAS 读取，normal 供 closest hit 着色）
/// @note GLSL 侧须用 float pos[3]/normal[3]（std430），勿用 vec3，避免 16 字节对齐导致 stride 不一致
struct ObjVertex {
    float pos[3];
    float normal[3];
};

static_assert(offsetof(ObjVertex, pos) == 0, "BLAS expects position at offset 0");
static_assert(offsetof(ObjVertex, normal) == 12, "ObjVertex normal offset mismatch");

/// @brief 从 OBJ 解析出的三角网格（非索引 triangle list，每 3 个顶点一个三角形）
struct ObjMesh {
    std::vector<ObjVertex> vertices;

    uint32_t triangleCount() const
    {
        return static_cast<uint32_t>(vertices.size() / 3);
    }
};

/// @brief 加载 OBJ 模型（仅解析 v / f，面会被三角化并展开为 triangle list）
/// @param relativePath 相对 exe 或当前工作目录的路径
/// @return 归一化到 [-0.5, 0.5]^3 单位包围盒内、含平滑法线的三角网格数据
inline ObjMesh loadObj(const std::string& relativePath)
{
    const std::string path = resolvePathNextToExe(relativePath);
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open obj file: " + path);
    }

    struct FaceTri {
        int32_t i0;
        int32_t i1;
        int32_t i2;
    };

    std::vector<glm::vec3> positions;
    std::vector<FaceTri> faceTris;
    ObjMesh mesh;

    auto resolveIndex = [&](int32_t index) -> size_t {
        if (index > 0) {
            return static_cast<size_t>(index - 1);
        }
        if (index < 0) {
            return positions.size() - static_cast<size_t>(-index);
        }
        throw std::runtime_error("invalid vertex index 0 in obj: " + path);
    };

    auto parseFaceLine = [&](std::istringstream& iss) {
        std::vector<int32_t> faceIndices;
        std::string token;
        while (iss >> token) {
            const size_t slash = token.find('/');
            const std::string indexToken = (slash == std::string::npos) ? token : token.substr(0, slash);
            faceIndices.push_back(static_cast<int32_t>(std::stoi(indexToken)));
        }

        if (faceIndices.size() < 3) {
            return;
        }

        for (size_t i = 1; i + 1 < faceIndices.size(); ++i) {
            faceTris.push_back({
                faceIndices[0],
                faceIndices[i],
                faceIndices[static_cast<size_t>(i + 1)]
            });
        }
    };

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v") {
            glm::vec3 position{};
            iss >> position.x >> position.y >> position.z;
            positions.push_back(position);
        }
    }

    if (positions.empty()) {
        throw std::runtime_error("obj mesh has no vertices: " + path);
    }

    {
        glm::vec3 minPos = positions[0];
        glm::vec3 maxPos = positions[0];
        for (const glm::vec3& position : positions) {
            minPos = glm::min(minPos, position);
            maxPos = glm::max(maxPos, position);
        }

        const glm::vec3 center = (minPos + maxPos) * 0.5f;
        const glm::vec3 extent = maxPos - minPos;
        const float maxExtent = glm::max(glm::max(extent.x, extent.y), extent.z);

        if (maxExtent <= 0.0f) {
            throw std::runtime_error("obj mesh has zero-size bounds: " + path);
        }

        for (glm::vec3& position : positions) {
            position = (position - center) / maxExtent;
        }
    }

    file.clear();
    file.seekg(0);

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "f") {
            parseFaceLine(iss);
        }
    }

    if (faceTris.empty()) {
        throw std::runtime_error("obj mesh has no triangles: " + path);
    }

    std::vector<glm::vec3> positionNormals(positions.size(), glm::vec3(0.0f));
    for (const FaceTri& tri : faceTris) {
        const size_t i0 = resolveIndex(tri.i0);
        const size_t i1 = resolveIndex(tri.i1);
        const size_t i2 = resolveIndex(tri.i2);
        if (i0 >= positions.size() || i1 >= positions.size() || i2 >= positions.size()) {
            throw std::runtime_error("face index out of range in obj: " + path);
        }

        const glm::vec3& p0 = positions[i0];
        const glm::vec3& p1 = positions[i1];
        const glm::vec3& p2 = positions[i2];
        const glm::vec3 faceNormal = glm::cross(p1 - p0, p2 - p0);

        positionNormals[i0] += faceNormal;
        positionNormals[i1] += faceNormal;
        positionNormals[i2] += faceNormal;
    }

    for (glm::vec3& normal : positionNormals) {
        const float len = glm::length(normal);
        normal = (len > 0.0f) ? normal / len : glm::vec3(0.0f, 0.0f, 1.0f);
    }

    mesh.vertices.reserve(faceTris.size() * 3);
    for (const FaceTri& tri : faceTris) {
        const size_t indices[3] = { resolveIndex(tri.i0), resolveIndex(tri.i1), resolveIndex(tri.i2) };
        for (size_t idx : indices) {
            const glm::vec3& position = positions[idx];
            const glm::vec3& normal = positionNormals[idx];
            mesh.vertices.push_back({
                { position.x, position.y, position.z },
                { normal.x, normal.y, normal.z }
            });
        }
    }

    if (mesh.vertices.empty() || mesh.vertices.size() % 3 != 0) {
        throw std::runtime_error("obj mesh has no triangles: " + path);
    }

    return mesh;
}