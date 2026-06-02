#pragma once

#include "vkEngine.h"

/// @brief 队列族索引
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily; // 图形队列族索引

    bool isComplete() { // 判断队列族索引是否完整
        return graphicsFamily.has_value();
    }
};

class vkEngineLogicalDevice;
class vkEngineCommandPool;

/// @brief 物理设备类
class vkEnginePhysicalDevice
{
public:
    /// @brief 构造函数
    /// @param engine 引擎
    vkEnginePhysicalDevice(std::shared_ptr<vkEngine> engine);

    ~vkEnginePhysicalDevice();

    /// @brief 获取引擎
    /// @return 引擎
    std::shared_ptr<vkEngine> getEngine();

    /// @brief 获取物理设备
    /// @return 物理设备
    VkPhysicalDevice& getVkPhysicalDevice();

    /// @brief 查找队列族索引(当前物理设备)
    /// @return 队列族索引
    QueueFamilyIndices findQueueFamilies();

private:
    friend class vkEngineLogicalDevice;
    friend class vkEngineCommandPool;

    /// @brief 判断设备是否适合
    /// @param device 设备
    /// @return 是否适合
    bool isDeviceSuitable(VkPhysicalDevice device);

    /// @brief 查找队列族索引
    /// @param device 设备
    /// @return 队列族索引
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

    /// @brief 检查设备扩展支持
    /// @param device 设备
    /// @return 是否支持
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);

private:
    std::shared_ptr<vkEngine> _engine; // 引用成员必须在构造时绑定目标对象，只能通过初始化列表完成 // 引擎 
    
    VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;  // 物理设备
    const std::vector<const char*> _deviceExtensions = { // 设备扩展名称 
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, // 延迟主机操作扩展名称
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, // 加速结构扩展名称
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, // 光线追踪管道扩展名称
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, // 缓冲区设备地址扩展名称
    };
};