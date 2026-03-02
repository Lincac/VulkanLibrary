#pragma once

#include <string>
#include <vector>

#include <volk/volk.h>

class Instance
{
public:
    Instance(
        const std::string& applicationName,
        bool enableValidationLayers = true,
        std::vector<const char*> requiredExtensions = {},
        std::vector<const char*> requestedValidationLayers = {"VK_LAYER_KHRONOS_validation"});
    ~Instance();

    Instance(const Instance&) = delete;
    Instance& operator=(const Instance&) = delete;
    Instance(Instance&& other) noexcept;
    Instance& operator=(Instance&& other) noexcept;

    VkInstance Get() const noexcept;
    const std::vector<const char*>& GetEnabledExtensions() const noexcept;
    const std::vector<const char*>& GetEnabledValidationLayers() const noexcept;
    bool IsValidationLayersEnabled() const noexcept;

    static std::vector<VkExtensionProperties> EnumerateAvailableExtensions();
    static std::vector<VkLayerProperties> EnumerateAvailableValidationLayers();

private:
    void CreateInstance(const std::string& applicationName);
    void Destroy() noexcept;
    bool CheckValidationLayerSupport(const std::vector<const char*>& validationLayers) const;
    std::vector<const char*> BuildEnabledExtensions(const std::vector<const char*>& requiredExtensions) const;

private:
    VkInstance instance_{VK_NULL_HANDLE};
    bool validationLayersEnabled_{false};
    std::vector<const char*> enabledExtensions_{};
    std::vector<const char*> enabledValidationLayers_{};
};
