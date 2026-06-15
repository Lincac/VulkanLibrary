#pragma once

#include "core/vkEngineContext.h"
#include "common/vkEngineHelp.h"

struct QueueFamilyIndices {
    /// @brief 图形队列族索引
    std::optional<uint32_t> graphicsFamily;
    /// @brief 呈现队列族索引
    std::optional<uint32_t> presentFamily;

    bool isComplete(const vkEnginePhysicalDeviceReq& req) const {
        if (req.enableGraphicsFamily && !graphicsFamily) return false;
        if (req.enablePresentFamily && !presentFamily) return false;
        return true;
    }
};

/// @brief 物理设备类
class vkEnginePhysicalDevice
{
public:
    explicit vkEnginePhysicalDevice(std::shared_ptr<vkEngineContext> context);
    ~vkEnginePhysicalDevice();

    std::shared_ptr<vkEngineContext> getContext() const;

    VkPhysicalDevice getVkPhysicalDevice() const;

    /// @brief 构造时缓存的队列族索引
    const QueueFamilyIndices& getQueueFamilies() const;

    /// @brief 物理设备需求配置
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
