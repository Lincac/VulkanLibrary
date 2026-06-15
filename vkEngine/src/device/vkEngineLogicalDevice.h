#pragma once

#include "device/vkEnginePhysicalDevice.h"

/// @brief 逻辑设备类
class vkEngineLogicalDevice
{
public:
    /// @brief 构造函数
    /// @param physicalDevice 物理设备
    vkEngineLogicalDevice(std::shared_ptr<vkEnginePhysicalDevice> physicalDevice);

    ~vkEngineLogicalDevice();

    /// @brief 获取物理设备
    /// @return 物理设备
    std::shared_ptr<vkEnginePhysicalDevice> getPhysicalDevice();

    /// @brief 获取逻辑设备
    /// @return 逻辑设备
    VkDevice& getVkDevice();

    /// @brief 获取图形队列
    /// @return 图形队列
    VkQueue& getGraphicsQueue();

private:
    std::shared_ptr<vkEnginePhysicalDevice> _physicalDevice; // 物理设备

    VkDevice _device = VK_NULL_HANDLE; // 逻辑设备
    VkQueue _graphicsQueue = VK_NULL_HANDLE; // 队列
};