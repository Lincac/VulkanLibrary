#pragma once

#include <Volk/volk.h>

#include <string>
#include <vector>

#define MAX_FRAMES_IN_FLIGHT 1

namespace mat {

    struct VkEngineConfig {
        std::string applicationName = "vkEngine";
        uint32_t applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        std::string engineName = "vkEngine";
        uint32_t engineVersion = VK_MAKE_VERSION(1, 0, 0);
        uint32_t apiVersion = VK_API_VERSION_1_3;
    };

    struct VkEnginePhysicalDeviceConfig {
        std::vector<const char*> extensions = {VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME};
    };

};  // namespace mat