#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "Device.h"

#include <vector>

class Swapchain
{
public:
    Swapchain(
        const PhysicalDevice& physicalDevice, 
        const Device& logicalDevice, 
        VkSurfaceKHR surface,
        GLFWwindow* window);
    ~Swapchain();

    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    VkFormat getSwapChainImageFormat() const;

    VkSwapchainKHR getSwapChain() const;

    VkFramebuffer getSwapChainFrameBuffer(uint32_t index) const;

    VkExtent2D getSwapChainExtent() const;

    uint32_t getImageCount() const;

    void setRenderPass(const Device& logicalDevice, VkRenderPass renderPass);

    void resetSwapChain(
        const PhysicalDevice& physicalDevice,
        const Device& logicalDevice,
        VkSurfaceKHR surface,
        GLFWwindow* window);

private:

    void initSwapChain(
        const PhysicalDevice& physicalDevice,
        const Device& logicalDevice,
        VkSurfaceKHR surface,
        GLFWwindow* window);

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);

private:

    VkSwapchainKHR _swapChain;
    std::vector<VkImage> _swapChainImages;

    std::vector<VkImageView> _swapChainImageViews;
    std::vector<VkFramebuffer> _swapChainFramebuffers;

    VkFormat _swapChainImageFormat;
    VkExtent2D _swapChainExtent;

};
