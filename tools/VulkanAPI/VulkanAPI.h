#pragma once

#include <iostream>
#include <vector>

#include <vulkan/vulkan.h>

class VulkanAPI
{
public:

    VulkanAPI();
    ~VulkanAPI();

    /// @brief 初始化 vulkan 应用程序，链接显卡接口，渲染环境搭建的初始阶段
    /// @param appName 应用程序名
    /// @param appVersion 应用程序版本
    /// @param engineName 引擎名称（指市面上的游戏引擎，用于指定应用是否运行在某个图形或游戏引擎之上）
    /// @param engineVersion 引擎名称
    /// @param apiVersion vulkan 版本
    void setApplicationInfo(const char* appName = nullptr, uint32_t appVersion = 0,
          const char* engineName = nullptr, uint32_t engineVersion = 0,
          uint32_t apiVersion = VK_API_VERSION_1_0);

    const VkApplicationInfo* getApplicationInfo();

    /// @brief 创建 vulkan 实例，
    /// @param extensions vulkan 实例需要链接的窗口拓展
    /// @param extensionCount 窗口拓展数量
    void createInstance(const char** extensions, uint32_t extensionCount);

    const VkInstanceCreateInfo* getInstanceInfo();

    /// @brief 获取 vulkan 支持的设备拓展属性
    /// @return 返回 vulkan 支持的设备拓展属性
    static std::vector<VkExtensionProperties> getVKExtensionProperties();

    void destroy();

private:

    VkInstanceCreateInfo *vk_instanceInfo;
    VkApplicationInfo *vk_appInfo;
    VkInstance *vk_instance; // vulkan 实例句柄

};
