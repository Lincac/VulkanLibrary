#pragma once

#include "config/vkEngineConfig.h"

/// @brief vk 上下文类
class vkEngineContext
{
public:
    /// @brief 构造函数
    /// @param config 配置
    explicit vkEngineContext(const vkEngineConfig& config = {});
    ~vkEngineContext();

    /// @brief 获取实例
    /// @return 实例
    VkInstance getVkInstance() const;

    /// @brief 获取配置
    /// @return 配置
    const vkEngineConfig& getConfig() const;

private:
    vkEngineContext(const vkEngineContext&) = delete;
    vkEngineContext(vkEngineContext&&) = delete;
    vkEngineContext& operator=(const vkEngineContext&) = delete;
    vkEngineContext& operator=(vkEngineContext&&) = delete;

    /// @brief 配置项
    vkEngineConfig _config;

    VkInstance _instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT _debugMessenger = VK_NULL_HANDLE;
};