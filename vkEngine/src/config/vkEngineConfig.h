#pragma once

#include <Volk/volk.h>
#include <string>
#include <vector>

#define MAX_FRAMES_IN_FLIGHT 1

struct vkEnginePhysicalDeviceReq {
    std::vector<const char*> extensions = {
        /// @brief 允许 Vulkan 驱动把某些昂贵的 CPU 操作延后执行，而不是在调用点阻塞 CPU
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME
    };

    bool enableGraphicsFamily = true;
    bool enablePresentFamily = false;
};

struct vkEngineConfig {
    std::string applicationName = "vkEngine";
    uint32_t applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    std::string engineName = "vkEngine";
    uint32_t engineVersion = VK_MAKE_VERSION(1, 0, 0);
    uint32_t apiVersion = VK_API_VERSION_1_3;

    bool enableValidationLayers = true;
    std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

    bool enableDebugMessenger = true;
    std::vector<const char*> instanceExtensions = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME };  

    vkEnginePhysicalDeviceReq physicalDeviceReq;
};