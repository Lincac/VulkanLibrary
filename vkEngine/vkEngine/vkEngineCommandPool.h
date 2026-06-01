#pragma once

#include "vkEngineLogicalDevice.h"

/// @brief 命令池类
class vkEngineCommandPool
{
public:
    /// @brief 构造函数
    /// @param device 逻辑设备
    vkEngineCommandPool(vkEngineLogicalDevice& device);
    ~vkEngineCommandPool();

private:
    vkEngineLogicalDevice& _device; // 逻辑设备
    
    VkCommandPool _commandPool; // 命令池
    std::vector<VkCommandBuffer> _commandBuffers; // 命令缓冲区
};