#include "Context.h"

#include <utility>

Context::Context(
    const std::string& applicationName,
    bool enableValidationLayers,
    std::vector<const char*> instanceExtensions,
    std::vector<const char*> validationLayers,
    std::vector<const char*> deviceExtensions,
    const VkPhysicalDeviceFeatures* enabledDeviceFeatures,
    VkPhysicalDeviceType preferredPhysicalDeviceType)
    : instance_(
          applicationName,
          enableValidationLayers,
          std::move(instanceExtensions),
          std::move(validationLayers)),
      physicalDevice_(
          instance_.Get(),
          deviceExtensions,
          preferredPhysicalDeviceType),
      device_(
          physicalDevice_,
          std::move(deviceExtensions),
          enabledDeviceFeatures)
{
}

const Instance& Context::GetInstance() const noexcept
{
    return instance_;
}

const PhysicalDevice& Context::GetPhysicalDevice() const noexcept
{
    return physicalDevice_;
}

const Device& Context::GetDevice() const noexcept
{
    return device_;
}

VkInstance Context::GetVkInstance() const noexcept
{
    return instance_.Get();
}

VkPhysicalDevice Context::GetVkPhysicalDevice() const noexcept
{
    return physicalDevice_.Get();
}

VkDevice Context::GetVkDevice() const noexcept
{
    return device_.Get();
}

std::optional<uint32_t> Context::GetGraphicsQueueFamilyIndex() const noexcept
{
    return device_.GetGraphicsQueueFamilyIndex();
}

std::optional<uint32_t> Context::GetComputeQueueFamilyIndex() const noexcept
{
    return device_.GetComputeQueueFamilyIndex();
}

std::optional<uint32_t> Context::GetTransferQueueFamilyIndex() const noexcept
{
    return device_.GetTransferQueueFamilyIndex();
}

VkQueue Context::GetGraphicsQueue() const noexcept
{
    return device_.GetGraphicsQueue();
}

VkQueue Context::GetComputeQueue() const noexcept
{
    return device_.GetComputeQueue();
}

VkQueue Context::GetTransferQueue() const noexcept
{
    return device_.GetTransferQueue();
}
