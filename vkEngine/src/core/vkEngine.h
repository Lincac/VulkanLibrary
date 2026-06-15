#pragma once

#include "common/vkEngineHelp.h"

/// @brief vk 引擎类
class vkEngine
{
public:
    /// @brief 构造函数
    /// @param applicationName 应用程序名称
    /// @param layerSupport 是否支持验证层
    vkEngine(const std::string& applicationName = "vkEngine", bool layerSupport = true);

    ~vkEngine();

    /// @brief 获取实例
    /// @return 实例
    VkInstance& getInstance();

private:
    // 验证层检查接口
    bool checkValidationLayerSupport();

    // 获取所需扩展
    std::vector<const char*> getRequiredExtensions(bool layerSupport);

    // 填充调试消息生成器创建信息
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    // 设置调试消息生成器接口
    void setupDebugMessenger(bool layerSupport);

private:
    std::string _applicationName; // 应用程序名称

    const std::vector<const char*> _validationLayers = { // 验证层名称
        "VK_LAYER_KHRONOS_validation" // 验证层名称
    };

    VkInstance _instance = VK_NULL_HANDLE; // 实例
    VkDebugUtilsMessengerEXT _debugMessenger = VK_NULL_HANDLE; // 调试消息生成器
};