#include "core/vkEngineContext.h"

#include <stdexcept>
#include <iostream>
#include <cstring>

namespace {

/// @brief 销毁调试消息生成器
/// @param instance 实例
/// @param debugMessenger 调试消息生成器
/// @param pAllocator 分配器
void destroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator){

    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

/// @brief 检查验证层支持
/// @param config 配置
/// @return 是否支持
bool checkValidationLayerSupport(const vkEngineConfig& config){
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr); 

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()); 

    for (const char* layerName : config.validationLayers) { // 验证层名称
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

/// @brief 获取所需扩展
/// @param config 配置
/// @return 所需扩展
std::vector<const char *> getRequiredExtensions(const vkEngineConfig& config){
    std::vector<const char*> extensions;

    auto addUnique = [&extensions](const char* name) {
        for (const char* ext : extensions) {
            if (strcmp(ext, name) == 0) {
                return;
            }
        }
        extensions.push_back(name);
    };

    for (const char* ext : config.instanceExtensions) {
        addUnique(ext);
    }

    return extensions;
}

/// @brief 默认调试回调
/// @param messageSeverity 消息严重性
/// @param messageType 消息类型
/// @param pCallbackData 回调数据
/// @param pUserData 用户数据
/// @return 是否继续
static VKAPI_ATTR VkBool32 VKAPI_CALL defaultDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData){
    (void)messageSeverity; // 消息严重性
    (void)messageType; // 消息类型
    (void)pUserData; // 用户数据
    std::cerr << "Verification layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

/// @brief 填充调试消息生成器创建信息
/// @param config 配置
/// @param createInfo 创建信息
void populateDebugMessengerCreateInfo(vkEngineConfig& config, VkDebugUtilsMessengerCreateInfoEXT &createInfo){
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = defaultDebugCallback;
    createInfo.pUserData = nullptr;
}

/// @brief 创建调试消息生成器
/// @param instance 实例
/// @param pCreateInfo 创建信息
/// @param pAllocator 分配器
/// @param pDebugMessenger 调试消息生成器
/// @return 结果
VkResult createDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}
} // namespace 


vkEngineContext::vkEngineContext(const vkEngineConfig& config)
    : _config(config){
        
    // 验证层检测，是否支持
    if(_config.enableValidationLayers && !checkValidationLayerSupport(_config)){
        throw std::runtime_error("Verification layer detection failed, no relevant support found!");
    }

    VkApplicationInfo appInfo{}; // 应用程序信息
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO; // 结构体类型   
    appInfo.pApplicationName = _config.applicationName.c_str(); // 应用程序名称
    appInfo.applicationVersion = _config.applicationVersion; // 应用程序版本
    appInfo.pEngineName = _config.engineName.c_str(); // 引擎名称
    appInfo.engineVersion = _config.engineVersion; // 引擎版本
    appInfo.apiVersion = _config.apiVersion; // API版本

    VkInstanceCreateInfo createInfo{}; // 实例创建信息
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO; // 结构体类型
    createInfo.pApplicationInfo = &appInfo; // 应用程序信息

    auto extensions = getRequiredExtensions(_config); // 获取所需扩展
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size()); // 启用扩展数量
    createInfo.ppEnabledExtensionNames = extensions.data(); // 启用扩展名称

    if (_config.enableValidationLayers){
        createInfo.enabledLayerCount = static_cast<uint32_t>(_config.validationLayers.size()); // 启用验证层数量
        createInfo.ppEnabledLayerNames = _config.validationLayers.data(); // 启用验证层名称
    }
    else{
        createInfo.enabledLayerCount = 0; // 启用验证层数量
    }
    
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{}; // 调试消息生成器创建信息
    if(_config.enableDebugMessenger)
    {
        populateDebugMessengerCreateInfo(_config, debugCreateInfo); // 填充调试消息生成器创建信息

        createInfo.pNext = &debugCreateInfo; // 下一个信息
    }
    else{
        createInfo.pNext = nullptr; // 下一个信息
    }

    if (vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance!");
    }

    volkLoadInstance(_instance);

    if(_config.enableDebugMessenger)
    {
        populateDebugMessengerCreateInfo(_config, debugCreateInfo); // 填充调试消息生成器创建信息

        if (createDebugUtilsMessengerEXT(_instance, &debugCreateInfo, nullptr, &_debugMessenger) != VK_SUCCESS) { // 创建调试消息生成器
            throw std::runtime_error("Unable to create debug generator!");
        }
    }
}

vkEngineContext::~vkEngineContext(){
    if (_debugMessenger != VK_NULL_HANDLE) {
        destroyDebugUtilsMessengerEXT(_instance, _debugMessenger, nullptr);
        _debugMessenger = VK_NULL_HANDLE;
    }

    if (_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(_instance, nullptr);
        _instance = VK_NULL_HANDLE;
    }
}

VkInstance vkEngineContext::getVkInstance() const{
    return _instance;
}

const vkEngineConfig &vkEngineContext::getConfig() const
{
    return _config;
}