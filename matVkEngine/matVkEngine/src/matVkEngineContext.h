#pragma once

#include <functional>
#include <optional>

#include "matVkEngineCommon.h"

#define MAX_FRAMES_IN_FLIGHT 1

namespace mat {

    class VkEngineContext {
    public:
        explicit VkEngineContext();
        ~VkEngineContext();

        VkInstance getVkInstance() const;

        VkPhysicalDevice getVkPhysicalDevice() const;

        VkDevice getVkDevice() const;

        VkCommandPool getVkCommandPool() const;

        void submitOneTimeCommands(std::function<void(VkCommandBuffer)> recordFunc);

    private:
        VkEngineContext(const VkEngineContext&) = delete;
        VkEngineContext(VkEngineContext&&) = delete;
        VkEngineContext& operator=(const VkEngineContext&) = delete;
        VkEngineContext& operator=(VkEngineContext&&) = delete;

        VkInstance _instance = VK_NULL_HANDLE;

#ifdef _DEBUG
        VkDebugUtilsMessengerEXT _debugMessenger = VK_NULL_HANDLE;
#endif

        std::optional<uint32_t> _graphicsFamily;
        VkPhysicalDevice _device = VK_NULL_HANDLE;

        VkDevice _logDevice = VK_NULL_HANDLE;
        VkQueue _graphicsQueue = VK_NULL_HANDLE;

        VkCommandPool _commandPool = VK_NULL_HANDLE;
        std::vector<VkCommandBuffer> _commandBuffers;
    };

};  // namespace mat
