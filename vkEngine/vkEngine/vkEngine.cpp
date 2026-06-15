#include "vkEngine.h"

vkEngine::vkEngine(const std::string& applicationName, bool layerSupport)
    : _applicationName(applicationName){
        
    if (volkInitialize() != VK_SUCCESS) {
        throw std::runtime_error("Volk initialization of Vulkan loader failed!");
    }

    // 验证层检测，是否支持
    if(layerSupport && !checkValidationLayerSupport()){
        throw std::runtime_error("Verification layer detection failed, no relevant support found!");
    }

    VkApplicationInfo appInfo{}; // 应用程序信息
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO; // 结构体类型   
    appInfo.pApplicationName = _applicationName.c_str(); // 应用程序名称
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0); // 应用程序版本
    appInfo.pEngineName = "vkEngine"; // 引擎名称
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0); // 引擎版本
    appInfo.apiVersion = VK_API_VERSION_1_3; // API版本

    auto extensions = getRequiredExtensions(layerSupport); // 获取所需扩展

    VkInstanceCreateInfo createInfo{}; // 实例创建信息
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO; // 结构体类型
    createInfo.pApplicationInfo = &appInfo; // 应用程序信息
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size()); // 启用扩展数量
    createInfo.ppEnabledExtensionNames = extensions.data(); // 启用扩展名称

    if (layerSupport){
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{}; // 调试消息生成器创建信息

        createInfo.enabledLayerCount = static_cast<uint32_t>(_validationLayers.size()); // 启用验证层数量
        createInfo.ppEnabledLayerNames = _validationLayers.data(); // 启用验证层名称

        populateDebugMessengerCreateInfo(debugCreateInfo); // 填充调试消息生成器创建信息
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo; // 下一个信息
    }
    else{
        createInfo.enabledLayerCount = 0; // 启用验证层数量
        createInfo.pNext = nullptr; // 下一个信息
    }

    if (vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance!");
    }

    volkLoadInstance(_instance);

    setupDebugMessenger(layerSupport);
}

void destroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator){

    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

vkEngine::~vkEngine(){
    if (_debugMessenger != VK_NULL_HANDLE) {
        destroyDebugUtilsMessengerEXT(_instance, _debugMessenger, nullptr);
        _debugMessenger = VK_NULL_HANDLE;
    }

    if (_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(_instance, nullptr);
        _instance = VK_NULL_HANDLE;
    }
}

VkInstance& vkEngine::getInstance(){
    return _instance;
}

bool vkEngine::checkValidationLayerSupport(){
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr); 

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()); 

    for (const char* layerName : _validationLayers) { // 验证层名称
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

std::vector<const char *> vkEngine::getRequiredExtensions(bool layerSupport){

    std::vector<const char*> extensions; // 扩展名称
    if (layerSupport) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); // 调试工具扩展名称
    }

    return extensions;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData){
    std::cerr << "Verification layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

void vkEngine::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo){
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

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

void vkEngine::setupDebugMessenger(bool layerSupport){
    if (!layerSupport) {
        return;
    }

    VkDebugUtilsMessengerCreateInfoEXT createInfo{}; // 调试消息生成器创建信息
    populateDebugMessengerCreateInfo(createInfo); // 填充调试消息生成器创建信息

    if (createDebugUtilsMessengerEXT(_instance, &createInfo, nullptr, &_debugMessenger) != VK_SUCCESS) { // 创建调试消息生成器
        throw std::runtime_error("Unable to create debug generator!");
    }
}
