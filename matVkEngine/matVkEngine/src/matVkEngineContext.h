#pragma once

#include <functional>
#include <optional>

#include "matVkEngineCommon.h"

#define MAX_FRAMES_IN_FLIGHT 1

/// <summary> GLFW
/// uint32_t n = 0;
/// const char** exts = glfwGetRequiredInstanceExtensions(&n);
///
/// VkEngineContextCreateInfo info{};
/// info.surface.instanceExtensions.assign(exts, exts + n);
/// info.surface.createSurface = [window](VkInstance inst, VkSurfaceKHR* out) {
///     return glfwCreateWindowSurface(inst, window, nullptr, out);
/// };
///
/// info.surface.chooseSwapExtent = [window](const VkSurfaceCapabilitiesKHR& caps) {
///    if (caps.currentExtent.width != UINT32_MAX) return caps.currentExtent;
///    int w, h;
///    glfwGetFramebufferSize(window, &w, &h);
///    return clampExtent({(uint32_t)w, (uint32_t)h}, caps);
/// };
/// </summary>

/// <summary> SDL
/// info.surface.instanceExtensions = { /* SDL_Vulkan_GetInstanceExtensions 返回的列表 */ };
/// info.surface.createSurface = [window](VkInstance inst, VkSurfaceKHR* out) {
///     return SDL_Vulkan_CreateSurface(window, inst, nullptr, out);
/// };
/// </summary>

/// <summary> Qt
/// info.surface.instanceExtensions = { /* QVulkanInstance::supportedExtensions() */ };
/// info.surface.createSurface = [qWindow](VkInstance inst, VkSurfaceKHR* out) {
///     return QVulkanInstance::surfaceForWindow(qWindow, out);  // 或平台等价 API
/// };
/// </summary>

namespace mat {

    struct VkEngineSurface {
        std::vector<const char*> instanceExtensions;

        std::function<VkResult(VkInstance instance, VkSurfaceKHR* outSurface)> createSurface;
        std::function<void(VkInstance instance, VkSurfaceKHR surface)> destroySurface;
        std::function<VkExtent2D()> queryFramebufferExtent;
    };

    class VkEngineContext {
    public:
        explicit VkEngineContext(std::optional<VkEngineSurface> surface = {});
        ~VkEngineContext();

        VkInstance getVkInstance() const;

        VkPhysicalDevice getVkPhysicalDevice() const;

        VkDevice getVkDevice() const;

        VkCommandPool getVkCommandPool() const;

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

        std::optional<uint32_t> _presentFamily;
        VkSurfaceKHR _surface = VK_NULL_HANDLE;
        VkQueue _presentQueue = VK_NULL_HANDLE;

        VkSwapchainKHR _swapChain = VK_NULL_HANDLE;
        std::vector<VkImage> _swapChainImages;
        VkFormat _swapChainImageFormat;
        VkExtent2D _swapChainExtent;
    };

};  // namespace mat
