#pragma once

#include "vkEnginePhysicalDevice.h"

/// @brief 逻辑设备类
class vkEngineLogicalDevice
{
public:
    /// @brief 构造函数
    /// @param physicalDevice 物理设备
    vkEngineLogicalDevice(vkEnginePhysicalDevice& physicalDevice);

    ~vkEngineLogicalDevice();

    /// @brief 获取物理设备
    /// @return 物理设备
    vkEnginePhysicalDevice& getVkPhysicalDevice();

    /// @brief 获取逻辑设备
    /// @return 逻辑设备
    VkDevice& getVkDevice();

private:
    vkEnginePhysicalDevice& _physicalDevice; // 物理设备

    VkDevice _device = VK_NULL_HANDLE; // 逻辑设备
    VkQueue _graphicsQueue = VK_NULL_HANDLE; // 队列
};