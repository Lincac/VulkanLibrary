#pragma once

#include <optional>
#include <vector>

#include <volk/volk.h>

#include "PhysicalDevice.h"

class Device
{
public:
    Device() = default;
    Device(
        const PhysicalDevice& physicalDevice,
        std::vector<const char*> enabledExtensions = {},
        const VkPhysicalDeviceFeatures* enabledFeatures = nullptr);
    ~Device();

    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;
    Device(Device&& other) noexcept;
    Device& operator=(Device&& other) noexcept;

    VkDevice Get() const noexcept;
    bool IsValid() const noexcept;

    const PhysicalDevice& GetPhysicalDevice() const noexcept;

    std::optional<uint32_t> GetGraphicsQueueFamilyIndex() const noexcept;
    std::optional<uint32_t> GetComputeQueueFamilyIndex() const noexcept;
    std::optional<uint32_t> GetTransferQueueFamilyIndex() const noexcept;

    VkQueue GetGraphicsQueue() const noexcept;
    VkQueue GetComputeQueue() const noexcept;
    VkQueue GetTransferQueue() const noexcept;
    VkQueue GetQueue(uint32_t familyIndex) const;

private:
    void CreateDevice(
        const PhysicalDevice& physicalDevice,
        const std::vector<const char*>& enabledExtensions,
        const VkPhysicalDeviceFeatures* enabledFeatures);
    void CacheQueues();
    void Destroy() noexcept;
    std::vector<uint32_t> BuildQueueFamiliesToCreate() const;

private:
    const PhysicalDevice* physicalDevice_{nullptr};
    VkDevice device_{VK_NULL_HANDLE};

    std::optional<uint32_t> graphicsQueueFamilyIndex_{};
    std::optional<uint32_t> computeQueueFamilyIndex_{};
    std::optional<uint32_t> transferQueueFamilyIndex_{};

    VkQueue graphicsQueue_{VK_NULL_HANDLE};
    VkQueue computeQueue_{VK_NULL_HANDLE};
    VkQueue transferQueue_{VK_NULL_HANDLE};
};

