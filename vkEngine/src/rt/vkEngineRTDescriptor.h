#pragma once

#include "rt/vkEngineAccelerationStructure.h"
#include "resource/vkEngineImage.h"
#include "resource/vkEngineTexture.h"

/// @brief RT 描述符集（TLAS + Storage Image + Vertex Buffer）
class vkEngineRTDescriptor
{
public:
    /// @brief 构造函数
    /// @param device 逻辑设备
    vkEngineRTDescriptor(std::shared_ptr<vkEngineLogicalDevice> device);
    ~vkEngineRTDescriptor();

    /// @brief 创建 layout 与 pool
    void create();

    /// @brief 分配 set 并写入 TLAS / 输出图 / 顶点 buffer
    /// @param tlas 顶层加速结构
    /// @param image 输出 storage image
    /// @param vertexBuffer 顶点 buffer（pos + normal，供 closest hit 读取）
    /// @param environmentMap 环境贴图
    /// @param settingsBuffer 路径追踪 uniform（binding = 5）
    void setup(std::shared_ptr<vkEngineAccelerationStructure> tlas, std::shared_ptr<vkEngineImage> image,
        std::shared_ptr<vkEngineBuffer> vertexBuffer, std::shared_ptr<vkEngineTexture> environmentMap,
        std::shared_ptr<vkEngineBuffer> settingsBuffer);

    /// @brief 获取描述符集布局
    /// @return 描述符集布局
    VkDescriptorSetLayout& getLayout();

    /// @brief 获取描述符集
    /// @return 描述符集
    VkDescriptorSet& getSet();

private:
    std::shared_ptr<vkEngineLogicalDevice> _device; // 逻辑设备

    // 资源接口表的结构定义
    /*
    * 描述符集的模板/契约，告诉 Vulkan 和 shader：

    * 有几个 binding（binding = 0, 1, 2...）
    * 每个 binding 是什么类型（TLAS、Storage Image、Storage Buffer 等）
    * 哪些 shader stage 可以访问
    */
    VkDescriptorSetLayout _layout = VK_NULL_HANDLE; // 描述符集布局

    // 绑定资源
    VkDescriptorPool _pool = VK_NULL_HANDLE; // 	分配描述符集的内存池
    VkDescriptorSet _set = VK_NULL_HANDLE; // 一组具体的描述符实例
};
