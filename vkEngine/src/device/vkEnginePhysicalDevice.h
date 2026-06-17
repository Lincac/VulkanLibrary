#pragma once

#include "core/vkEngineContext.h"
#include "common/vkEngineHelp.h"

struct QueueFamilyIndices {
    /// @brief 图形队列族索引
    std::optional<uint32_t> graphicsFamily;
    /// @brief 呈现队列族索引
    std::optional<uint32_t> presentFamily;

    bool isComplete() const {
        if (!graphicsFamily /* || !presentFamily */) {
            return false;
        }
        
        return true;
    }
};

/// @brief 物理设备类
class vkEnginePhysicalDevice
{
public:
    explicit vkEnginePhysicalDevice(std::shared_ptr<vkEngineContext> context);
    ~vkEnginePhysicalDevice();

    /// @brief 获取上下文
    /// @return 上下文
    std::shared_ptr<vkEngineContext> getContext() const;

    /// @brief 获取物理设备
    /// @return 物理设备
    VkPhysicalDevice getVkPhysicalDevice() const;

    /// @brief 构造时缓存的队列族索引
    /// @return 队列族索引
    const QueueFamilyIndices& getQueueFamilies() const;

    /// @brief 物理设备需求配置
    /// @return 物理设备需求配置
    const vkEnginePhysicalDeviceReq& getPhysicalDeviceReq() const;

private:
    vkEnginePhysicalDevice(const vkEnginePhysicalDevice&) = delete;
    vkEnginePhysicalDevice(vkEnginePhysicalDevice&&) = delete;
    vkEnginePhysicalDevice& operator=(const vkEnginePhysicalDevice&) = delete;
    vkEnginePhysicalDevice& operator=(vkEnginePhysicalDevice&&) = delete;

    std::shared_ptr<vkEngineContext> _context;
    VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
    QueueFamilyIndices _queueFamilyIndices;
};
