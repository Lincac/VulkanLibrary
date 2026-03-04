#include "Instance.h"

#include <algorithm>
#include <cstring>
#include <iostream>
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

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    (void)pUserData;
    const char* severity = "INFO";
    if ((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) != 0)
    {
        severity = "ERROR";
    }
    else if ((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) != 0)
    {
        severity = "WARN";
    }

    const char* type = "GENERAL";
    if ((messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) != 0)
    {
        type = "VALIDATION";
    }
    else if ((messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) != 0)
    {
        type = "PERFORMANCE";
    }

    const char* message = (pCallbackData != nullptr && pCallbackData->pMessage != nullptr)
                              ? pCallbackData->pMessage
                              : "No debug message";
    std::cerr << "[Vulkan][" << severity << "][" << type << "] " << message << '\n';
    return VK_FALSE;
}

void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = DebugCallback;
    createInfo.pUserData = nullptr;
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
      debugMessenger_(other.debugMessenger_),
      validationLayersEnabled_(other.validationLayersEnabled_),
      enabledExtensions_(std::move(other.enabledExtensions_)),
      enabledValidationLayers_(std::move(other.enabledValidationLayers_))
{
    other.instance_ = VK_NULL_HANDLE;
    other.debugMessenger_ = VK_NULL_HANDLE;
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
    debugMessenger_ = other.debugMessenger_;
    validationLayersEnabled_ = other.validationLayersEnabled_;
    enabledExtensions_ = std::move(other.enabledExtensions_);
    enabledValidationLayers_ = std::move(other.enabledValidationLayers_);

    other.instance_ = VK_NULL_HANDLE;
    other.debugMessenger_ = VK_NULL_HANDLE;
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
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        PopulateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = &debugCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledLayerNames = nullptr;
        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &instance_) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan instance.");
    }

    SetupDebugMessenger();
}

void Instance::SetupDebugMessenger()
{
    if (!validationLayersEnabled_)
    {
        return;
    }

    const auto createDebugUtilsMessenger =
        reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance_, "vkCreateDebugUtilsMessengerEXT"));
    if (createDebugUtilsMessenger == nullptr)
    {
        return;
    }

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    PopulateDebugMessengerCreateInfo(createInfo);

    if (createDebugUtilsMessenger(instance_, &createInfo, nullptr, &debugMessenger_) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to set up Vulkan debug messenger.");
    }
}

void Instance::DestroyDebugMessenger() noexcept
{
    if (instance_ == VK_NULL_HANDLE || debugMessenger_ == VK_NULL_HANDLE)
    {
        return;
    }

    const auto destroyDebugUtilsMessenger =
        reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance_, "vkDestroyDebugUtilsMessengerEXT"));
    if (destroyDebugUtilsMessenger != nullptr)
    {
        destroyDebugUtilsMessenger(instance_, debugMessenger_, nullptr);
    }

    debugMessenger_ = VK_NULL_HANDLE;
}

void Instance::Destroy() noexcept
{
    DestroyDebugMessenger();
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
