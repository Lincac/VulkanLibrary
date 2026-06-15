#pragma once

#include "device/vkEngineCommandPool.h"

/// @brief 缓冲区类
class vkEngineBuffer 
{
public:
    /// @brief 构造函数
    /// @param device 逻辑设备
    vkEngineBuffer(std::shared_ptr<vkEngineLogicalDevice> device);
    ~vkEngineBuffer();

    /// @brief 设置大小
    /// @param size 大小
    void setSize(VkDeviceSize size);

    /// @brief 设置使用标志
    /// @param usage 使用标志
    void setUsage(VkBufferUsageFlags usage);
    
    /// @brief 设置内存属性
    /// @param properties 内存属性
    void setMemoryProperties(VkMemoryPropertyFlags properties);

    /// @brief 创建
    void create();                              // 只创建，不上传

    /// @brief 上传
    /// @param commandPool 命令池
    /// @param data 数据
    /// @param size 大小
    void upload(std::shared_ptr<vkEngineCommandPool> mandPool, const void* data, VkDeviceSize size);  // staging → device local

    /// @brief 获取缓冲区
    /// @return 缓冲区
    VkBuffer& getBuffer();

    /// @brief 获取设备内存
    /// @return 设备内存
    VkDeviceMemory& getMemory();

    /// @brief 获取设备地址
    /// @return 设备地址
    VkDeviceAddress getDeviceAddress();         // AS 必须用到

private:
    std::shared_ptr<vkEngineLogicalDevice> _device; // 逻辑设备

    VkBuffer _buffer = VK_NULL_HANDLE; // 缓冲区（逻辑描述，没有实际存储）
    VkDeviceMemory _memory = VK_NULL_HANDLE; // 设备内存（实际内存）

    VkDeviceSize _size = 0; // 大小
    VkBufferUsageFlags _usage = 0; // 使用标志
    VkMemoryPropertyFlags _memoryProperties = 0; // 内存属性
};