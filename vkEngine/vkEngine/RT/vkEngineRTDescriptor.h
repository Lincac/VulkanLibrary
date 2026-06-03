#pragma once

#include "vkEngineAccelerationStructure.h"
#include "vkEngineImage.h"

/// @brief RT 描述符集（TLAS + Storage Image）
class vkEngineRTDescriptor
{
public:
    /// @brief 构造函数
    /// @param device 逻辑设备
    vkEngineRTDescriptor(std::shared_ptr<vkEngineLogicalDevice> device);
    ~vkEngineRTDescriptor();

    /// @brief 创建 layout 与 pool
    void create();

    /// @brief 分配 set 并写入 TLAS / 输出图
    /// @param tlas 顶层加速结构
    /// @param image 输出 storage image
    void setup(std::shared_ptr<vkEngineAccelerationStructure> tlas, std::shared_ptr<vkEngineImage> image);

    /// @brief 获取描述符集布局
    /// @return 描述符集布局
    VkDescriptorSetLayout& getLayout();

    /// @brief 获取描述符集
    /// @return 描述符集
    VkDescriptorSet& getSet();

private:
    std::shared_ptr<vkEngineLogicalDevice> _device; // 逻辑设备

    VkDescriptorSetLayout _layout = VK_NULL_HANDLE; // 描述符集布局
    VkDescriptorPool _pool = VK_NULL_HANDLE; // 描述符池
    VkDescriptorSet _set = VK_NULL_HANDLE; // 描述符集
};
