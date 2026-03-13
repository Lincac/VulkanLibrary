#include "vkEngine.h"

#include <stdexcept>

vkEngine::vkEngine(const std::string& appName, GLFWwindow* window)
	: 
    _applicationName(appName),
    _window(window),
    _enableValidationLayers(false),
    _debugMessenger(VK_NULL_HANDLE),
    _instance(VK_NULL_HANDLE),
    _surface(VK_NULL_HANDLE),
    _physicalDevice(VK_NULL_HANDLE),
    _logicalDevice(VK_NULL_HANDLE),
    _graphicsQueue(VK_NULL_HANDLE),
    _presentQueue(VK_NULL_HANDLE),
    _swapChain(VK_NULL_HANDLE),
    _swapChainImageFormat(VK_FORMAT_UNDEFINED),
    _swapChainExtent({ 0, 0 }),
    _opaqueRenderPass(VK_NULL_HANDLE),
    _commandPool(VK_NULL_HANDLE)
{
}

int vkEngine::init()
{
    initInstance();
    
    if (glfwCreateWindowSurface(_instance, _window, nullptr, &_surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }

    initPhysicalDevice();
    initLogicalDevice();

    initSwapChain();

    initCommandPool();

	return 0;
}

GLFWwindow* vkEngine::getWindow() const noexcept
{
    return _window;
}

VkInstance vkEngine::getInstance() const noexcept
{
    return _instance;
}

VkSurfaceKHR vkEngine::getSurface() const noexcept
{
    return _surface;
}

VkPhysicalDevice vkEngine::getPhysicalDevice() const noexcept
{
    return _physicalDevice;
}

VkDevice vkEngine::getLogicalDevice() const noexcept
{
    return _logicalDevice;
}

VkSwapchainKHR vkEngine::getSwapChain() const noexcept
{
    return _swapChain;
}

VkFormat vkEngine::getSwapChainImageFormat() const noexcept
{
    return _swapChainImageFormat;
}

uint32_t vkEngine::getSwapChainImageCount() const noexcept
{
    return static_cast<uint32_t>(_swapChainImageViews.size());
}

VkImageView vkEngine::getSwapChainImageView(uint32_t index) const noexcept
{
    if (index >= _swapChainImageViews.size()) {
        return VK_NULL_HANDLE;
    }

    return _swapChainImageViews[index];
}

VkCommandBuffer vkEngine::getCommandBuffer(uint32_t frame)
{
    return _commandBuffers[frame];
}

VkQueue vkEngine::getGraphicsQueue() const noexcept
{
    return _graphicsQueue;
}

VkQueue vkEngine::getPresentQueue() const noexcept
{
    return _presentQueue;
}

VkExtent2D vkEngine::getSwapChainExtent() const noexcept
{
    return _swapChainExtent;
}

VkFramebuffer vkEngine::getFramebuffer(uint32_t frame) const noexcept
{
    if (frame >= _swapChainFramebuffers.size()) {
        return VK_NULL_HANDLE;
    }
    return _swapChainFramebuffers[frame];
}
