#pragma once

#include "config/matVkEngineConfig.h"

namespace mat {

    class VkEngineContext {
    public:
        explicit VkEngineContext(const VkEngineConfig& config = {});
        ~VkEngineContext();

        VkInstance getVkInstance() const;

        VkEngineConfig getConfig() const;

    private:
        VkEngineContext(const VkEngineContext&) = delete;
        VkEngineContext(VkEngineContext&&) = delete;
        VkEngineContext& operator=(const VkEngineContext&) = delete;
        VkEngineContext& operator=(VkEngineContext&&) = delete;

        VkEngineConfig _config;
        VkInstance _instance = VK_NULL_HANDLE;

#ifdef _DEBUG
        VkDebugUtilsMessengerEXT _debugMessenger = VK_NULL_HANDLE;
#endif  // _DEBUG
    };

};  // namespace mat
