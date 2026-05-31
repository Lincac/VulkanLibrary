#pragma once

#include "vkEngine.h"

#include <optional>

/// @brief 队列族索引
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily; // 图形队列族索引
    std::optional<uint32_t> presentFamily; // 呈现队列族索引

    bool isComplete() { // 判断队列族索引是否完整
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

class vkEngineLogicalDevice;
class vkEngineSwapChain;
class vkEngineCommandPool;

/// @brief 物理设备类
class vkEnginePhysicalDevice
{
public:
    /// @brief 构造函数
    /// @param engine 引擎
    vkEnginePhysicalDevice(vkEngine& engine);

    ~vkEnginePhysicalDevice();

    /// @brief 获取引擎
    /// @return 引擎
    vkEngine& getVkEngine();

    /// @brief 获取物理设备
    /// @return 物理设备
    VkPhysicalDevice& getVkPhysicalDevice();

private:

    friend class vkEngineLogicalDevice;
    friend class vkEngineSwapChain;
    friend class vkEngineCommandPool;

    /// @brief 判断设备是否适合
    /// @param device 设备
    /// @return 是否适合
    bool isDeviceSuitable(VkPhysicalDevice device);

    /// @brief 查找队列族索引
    /// @param device 设备
    /// @return 队列族索引
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    QueueFamilyIndices findQueueFamilies();

    /// @brief 检查设备扩展支持
    /// @param device 设备
    /// @return 是否支持
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);

    /// @brief 查询交换链支持细节
    /// @param device 设备
    /// @return 交换链支持细节
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

    /// @brief 查询交换链支持细节(当前物理设备)
    /// @return 交换链支持细节
    SwapChainSupportDetails querySwapChainSupport();

private:
    // 引擎 
    vkEngine& _engine; // 引用成员必须在构造时绑定目标对象，只能通过初始化列表完成

    // 物理设备
    VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE; 

    // 设备扩展
    const std::vector<const char*> _deviceExtensions = { // 设备扩展名称
        VK_KHR_SWAPCHAIN_EXTENSION_NAME // 交换链扩展名称
    };
};