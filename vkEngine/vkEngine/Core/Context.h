#pragma once

#include <optional>
#include <string>
#include <vector>

#include <volk/volk.h>

class Context
{
public:
    Context(
        const std::string& applicationName,
        bool enableValidationLayers = true,
        std::vector<const char*> instanceExtensions = {},
        std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"},
        std::vector<const char*> deviceExtensions = {},
        const VkPhysicalDeviceFeatures* enabledDeviceFeatures = nullptr,
        VkPhysicalDeviceType preferredPhysicalDeviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU);

    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;
    Context(Context&& other) noexcept = default;
    Context& operator=(Context&& other) noexcept = default;
    ~Context() = default;

    const Instance& GetInstance() const noexcept;
    const PhysicalDevice& GetPhysicalDevice() const noexcept;
    const Device& GetDevice() const noexcept;

    VkInstance GetVkInstance() const noexcept;
    VkPhysicalDevice GetVkPhysicalDevice() const noexcept;
    VkDevice GetVkDevice() const noexcept;

    std::optional<uint32_t> GetGraphicsQueueFamilyIndex() const noexcept;
    std::optional<uint32_t> GetComputeQueueFamilyIndex() const noexcept;
    std::optional<uint32_t> GetTransferQueueFamilyIndex() const noexcept;

    VkQueue GetGraphicsQueue() const noexcept;
    VkQueue GetComputeQueue() const noexcept;
    VkQueue GetTransferQueue() const noexcept;

private:
    Instance instance_;
    PhysicalDevice physicalDevice_;
    Device device_;
};

