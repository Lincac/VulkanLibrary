#include "VulkanAPI.h"

VulkanAPI::VulkanAPI()
{
    vk_instance = nullptr;
    vk_instanceInfo = nullptr;
    vk_appInfo = nullptr;
}

VulkanAPI::~VulkanAPI()
{
    if(vk_instance != nullptr)
    {
        vkDestroyInstance(*vk_instance, nullptr);
        vk_instance = nullptr;
    }

    if(vk_appInfo != nullptr)
    {
        delete vk_appInfo;
        vk_appInfo = nullptr;
    }

    if(vk_instanceInfo != nullptr)
    {
        delete vk_instanceInfo;
        vk_instanceInfo = nullptr;
    }
}

void VulkanAPI::setApplicationInfo(const char *appName, uint32_t appVersion, 
    const char *engineName, uint32_t engineVersion, uint32_t apiVersion)
{
    if(vk_appInfo != nullptr)
    {
        delete vk_appInfo;
        vk_appInfo = nullptr;
    }
    vk_appInfo = new VkApplicationInfo();

    vk_appInfo->sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    vk_appInfo->pApplicationName = appName;
    vk_appInfo->applicationVersion = appVersion;
    vk_appInfo->pEngineName = engineName;
    vk_appInfo->engineVersion = engineVersion;
    vk_appInfo->apiVersion = apiVersion;
}

const VkApplicationInfo *VulkanAPI::getApplicationInfo()
{
    return vk_appInfo;
}

void VulkanAPI::createInstance(const char **extensions, uint32_t extensionCount)
{
    if(vk_instanceInfo != nullptr)
    {
        delete vk_instanceInfo;
        vk_instanceInfo = nullptr;
    }
    vk_instanceInfo = new VkInstanceCreateInfo();

    vk_instanceInfo->sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    vk_instanceInfo->enabledExtensionCount = extensionCount;
    vk_instanceInfo->ppEnabledExtensionNames = extensions;
    vk_instanceInfo->pApplicationInfo = vk_appInfo;
    vk_instanceInfo->enabledLayerCount = 0;

    std::vector<const char*> requiredExtensions;
    for(uint32_t i = 0; i < extensionCount; i++) 
    {
        requiredExtensions.emplace_back(extensions[i]);
    }
    requiredExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

    vk_instanceInfo->flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    vk_instanceInfo->enabledExtensionCount = (uint32_t) requiredExtensions.size();
    vk_instanceInfo->ppEnabledExtensionNames = requiredExtensions.data();
    vk_instanceInfo->enabledLayerCount = 0;

    if (vkCreateInstance(vk_instanceInfo, nullptr, vk_instance) != VK_SUCCESS) 
    {
        throw std::runtime_error("failed to create instance!");
    }
}

const VkInstanceCreateInfo *VulkanAPI::getInstanceInfo()
{
    return vk_instanceInfo;
}

std::vector<VkExtensionProperties> VulkanAPI::getVKExtensionProperties()
{
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    return extensions;
}

void VulkanAPI::destroy()
{
    if(vk_instance != nullptr)
    {
        vkDestroyInstance(*vk_instance, nullptr);
    }
}
