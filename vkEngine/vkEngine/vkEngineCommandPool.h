#pragma once

#include "vkEngineLogicalDevice.h"

/// @brief 命令池类
class vkEngineCommandPool
{
public:
    /// @brief 构造函数
    /// @param device 逻辑设备
    vkEngineCommandPool(std::shared_ptr<vkEngineLogicalDevice> vice);
    ~vkEngineCommandPool();

    /// @brief 提交一次命令
    /// @param recordFunc 记录函数，参数为命令缓冲区
    void submitOneTimeCommands(std::function<void(VkCommandBuffer)> recordFunc);

private:
    std::shared_ptr<vkEngineLogicalDevice> _device; // 逻辑设备
    
    VkCommandPool _commandPool = VK_NULL_HANDLE; // 命令池
    std::vector<VkCommandBuffer> _commandBuffers; // 命令缓冲区
};