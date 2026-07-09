#pragma once

#include <functional>
#include <optional>

#include "matVkEngineCommon.h"

namespace mat {

    struct VkEngineSurface {
        std::vector<const char*> instanceExtensions;

        std::function<VkResult(VkInstance instance, VkSurfaceKHR* outSurface)> createSurface;
        std::function<VkExtent2D()> queryFramebufferExtent;
    };

    class VkEngineContext {
    public:
        explicit VkEngineContext(std::optional<VkEngineSurface> surface = {});
        ~VkEngineContext();

        VkInstance getInstance() const;

        VkPhysicalDevice getPhysicalDevice() const;

        VkDevice getDevice() const;

        VkCommandPool getVkCommandPool() const;

        VkQueue getGraphicsQueue() const;

        VkQueue getPresentQueue() const;

        VkSwapchainKHR getSwapChain() const;

        VkFormat getSwapChainImageFormat() const;

        VkExtent2D getSwapChainImageExtent() const;

        std::vector<VkImageView> getSwapChainImageViews() const;

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

        std::optional<uint32_t> _presentFamily;
        VkSurfaceKHR _surface = VK_NULL_HANDLE;
        VkQueue _presentQueue = VK_NULL_HANDLE;

        VkSwapchainKHR _swapChain = VK_NULL_HANDLE;
        std::vector<VkImage> _swapChainImages;
        VkFormat _swapChainImageFormat;
        VkExtent2D _swapChainExtent;

        std::vector<VkImageView> _swapChainImageViews;
    };

};  // namespace mat
