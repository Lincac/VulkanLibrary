#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "Device.h"

class Swapchain
{
public:
    Swapchain();

public:

    void setDependice(Device* logicalDevice, GLFWwindow* window);

    int create();

    VkFormat getSwapChainImageFormat() const;

    VkSwapchainKHR getSwapChain() const;

    VkFramebuffer getSwapChainFrameBuffer(uint32_t index) const;

    VkExtent2D getSwapChainExtent() const;

    uint32_t getImageCount() const;

    const Device* getDevice() const;

    void setRenderPass(VkRenderPass renderPass);

    void resetSwapChain();

private:

    void initSwapChain();

    SwapChainSupportDetails querySwapChainSupport();

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

private:

    Device* _logicalDevice;
    GLFWwindow* _window;

private:

    VkSwapchainKHR _swapChain;
    std::vector<VkImage> _swapChainImages;

    std::vector<VkImageView> _swapChainImageViews;
    std::vector<VkFramebuffer> _swapChainFramebuffers;

    VkFormat _swapChainImageFormat;
    VkExtent2D _swapChainExtent;

};
