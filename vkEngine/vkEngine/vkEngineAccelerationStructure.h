#pragma once 

#include "vkEngineBuffer.h"

// BLAS : Bottom Level Acceleration Structure 底层加速结构
/*
 * BLAS 存储：
 * 三角形网格（vertex/index buffer）
 * AABB（用于 procedural primitives）
 * 每个 mesh 的局部空间几何体

 * 特点：
 * 与实例无关
 * 一个 mesh 一个 BLAS（通常）
 */

// TLAS : Top Level Acceleration Structure 顶层加速结构
/*
 * TLAS 存储：
 * 对多个 BLAS 的引用（Instance）
 * 每个实例的变换矩阵（位置、旋转、缩放）
 * 实例 ID、mask、shader binding info

 * 特点：
 * 场景级别结构
 * 一个场景一个 TLAS（通常）
 */

/// @brief 加速结构体类
/// BLAS = 几何体本身（你的 1 个三角形）
/// TLAS = 场景实例列表（引用 BLAS + 变换矩阵）
class vkEngineAccelerationStructure 
{
public:
    enum class Type { BLAS, TLAS };

    /// @brief 构造函数
    /// @param device 逻辑设备
    /// @param type 类型
    vkEngineAccelerationStructure(std::shared_ptr<vkEngineLogicalDevice> device, Type type);
    ~vkEngineAccelerationStructure();

    // BLAS 专用：设置三角形几何
    /// @brief 设置三角形几何
    /// @param vertexAddress 顶点地址
    /// @param vertexCount 顶点数量
    void setTriangleGeometry(VkDeviceAddress vertexAddress, uint32_t vertexCount);

    /// @brief 设置实例
    /// @param blas 底层加速结构
    /// @param transform 变换矩阵
    void setInstance(vkEngineAccelerationStructure& blas, const glm::mat4& transform = glm::mat4(1.0f));

    // 查询 size、创建 buffer、提交 build（一步完成）
    /// @brief 构建
    /// @param commandPool 命令池
    void build(std::shared_ptr<vkEngineCommandPool> commandPool);

    /// @brief 获取句柄
    /// @return 句柄
    VkAccelerationStructureKHR& getHandle();

    /// @brief 获取设备地址
    /// @return 设备地址
    VkDeviceAddress getDeviceAddress();

private:
    std::shared_ptr<vkEngineLogicalDevice> _device; // 逻辑设备

    Type _type; // 类型

    VkAccelerationStructureKHR _handle = VK_NULL_HANDLE; // 句柄

    std::shared_ptr<vkEngineBuffer> _asBuffer;      // 存 AS 本体
    std::shared_ptr<vkEngineBuffer> _scratchBuffer; // build 临时用，build 完可不管

    // BLAS 参数
    VkDeviceAddress _vertexAddress = 0; // 顶点地址
    uint32_t _vertexCount = 0; // 顶点数量

    // TLAS 参数
    std::shared_ptr<vkEngineBuffer> _instanceBuffer; // instance 数据 buffer
    std::vector<VkAccelerationStructureInstanceKHR> _instances; // instance 列表
};