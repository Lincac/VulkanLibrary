#include "Instance.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string>

namespace
{
bool ContainsExtension(const std::vector<VkExtensionProperties>& availableExtensions, const char* extensionName)
{
    return std::any_of(
        availableExtensions.begin(),
        availableExtensions.end(),
        [extensionName](const VkExtensionProperties& ext)
        {
            return std::strcmp(ext.extensionName, extensionName) == 0;
        });
}
} // namespace

Instance::Instance(
    const std::string& applicationName,
    bool enableValidationLayers,
    std::vector<const char*> requiredExtensions,
    std::vector<const char*> requestedValidationLayers)
    : validationLayersEnabled_(enableValidationLayers)
{
    enabledValidationLayers_ = std::move(requestedValidationLayers);
    enabledExtensions_ = BuildEnabledExtensions(requiredExtensions);

    if (validationLayersEnabled_ && !CheckValidationLayerSupport(enabledValidationLayers_))
    {
        throw std::runtime_error("Requested Vulkan validation layers are not available.");
    }

    CreateInstance(applicationName);
}

Instance::~Instance()
{
    Destroy();
}

Instance::Instance(Instance&& other) noexcept
    : instance_(other.instance_),
      validationLayersEnabled_(other.validationLayersEnabled_),
      enabledExtensions_(std::move(other.enabledExtensions_)),
      enabledValidationLayers_(std::move(other.enabledValidationLayers_))
{
    other.instance_ = VK_NULL_HANDLE;
    other.validationLayersEnabled_ = false;
}

Instance& Instance::operator=(Instance&& other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    Destroy();

    instance_ = other.instance_;
    validationLayersEnabled_ = other.validationLayersEnabled_;
    enabledExtensions_ = std::move(other.enabledExtensions_);
    enabledValidationLayers_ = std::move(other.enabledValidationLayers_);

    other.instance_ = VK_NULL_HANDLE;
    other.validationLayersEnabled_ = false;
    return *this;
}

VkInstance Instance::Get() const noexcept
{
    return instance_;
}

const std::vector<const char*>& Instance::GetEnabledExtensions() const noexcept
{
    return enabledExtensions_;
}

const std::vector<const char*>& Instance::GetEnabledValidationLayers() const noexcept
{
    return enabledValidationLayers_;
}

bool Instance::IsValidationLayersEnabled() const noexcept
{
    return validationLayersEnabled_;
}

std::vector<VkExtensionProperties> Instance::EnumerateAvailableExtensions()
{
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> extensions(extensionCount);
    if (extensionCount > 0)
    {
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
    }

    return extensions;
}

std::vector<VkLayerProperties> Instance::EnumerateAvailableValidationLayers()
{
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> layers(layerCount);
    if (layerCount > 0)
    {
        vkEnumerateInstanceLayerProperties(&layerCount, layers.data());
    }

    return layers;
}

void Instance::CreateInstance(const std::string& applicationName)
{
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = applicationName.c_str();
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "vkEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions_.size());
    createInfo.ppEnabledExtensionNames = enabledExtensions_.empty() ? nullptr : enabledExtensions_.data();

    if (validationLayersEnabled_)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(enabledValidationLayers_.size());
        createInfo.ppEnabledLayerNames = enabledValidationLayers_.empty() ? nullptr : enabledValidationLayers_.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledLayerNames = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &instance_) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan instance.");
    }
}

void Instance::Destroy() noexcept
{
    if (instance_ != VK_NULL_HANDLE)
    {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }
}

bool Instance::CheckValidationLayerSupport(const std::vector<const char*>& validationLayers) const
{
    const auto availableLayers = EnumerateAvailableValidationLayers();

    for (const char* layerName : validationLayers)
    {
        const bool found = std::any_of(
            availableLayers.begin(),
            availableLayers.end(),
            [layerName](const VkLayerProperties& layerProperties)
            {
                return std::strcmp(layerProperties.layerName, layerName) == 0;
            });

        if (!found)
        {
            return false;
        }
    }

    return true;
}

std::vector<const char*> Instance::BuildEnabledExtensions(const std::vector<const char*>& requiredExtensions) const
{
    const auto availableExtensions = EnumerateAvailableExtensions();

    std::vector<const char*> extensions = requiredExtensions;

    for (const char* extensionName : requiredExtensions)
    {
        if (!ContainsExtension(availableExtensions, extensionName))
        {
            throw std::runtime_error(
                std::string("Required Vulkan instance extension is not available: ") + extensionName);
        }
    }

    if (validationLayersEnabled_)
    {
        constexpr const char* kDebugUtilsExtension = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        const bool hasDebugUtils = ContainsExtension(availableExtensions, kDebugUtilsExtension);
        const bool alreadyEnabled = std::any_of(
            extensions.begin(),
            extensions.end(),
            [](const char* extensionName)
            {
                return std::strcmp(extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0;
            });

        if (hasDebugUtils && !alreadyEnabled)
        {
            extensions.push_back(kDebugUtilsExtension);
        }
    }

    return extensions;
}
