#pragma once

#include "vkEngineLogicalDevice.h"

/// @brief 交换链类
class vkEngineSwapChain
{
public:
    /// @brief 构造函数
    /// @param device 逻辑设备
    /// @param window 窗口
    vkEngineSwapChain(vkEngineLogicalDevice& device, GLFWwindow* window = nullptr);
    ~vkEngineSwapChain();

private:
    /// @brief 选择交换链表面格式
    /// @param availableFormats 可用表面格式
    /// @return 选择表面格式
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

    /// @brief 选择交换链呈现模式
    /// @param availablePresentModes 可用呈现模式
    /// @return 选择呈现模式
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

    /// @brief 选择交换链扩展
    /// @param capabilities 表面能力
    /// @param window 窗口
    /// @return 选择扩展
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);

private:
    // 逻辑设备
    vkEngineLogicalDevice& _device;

    VkSwapchainKHR _swapChain = VK_NULL_HANDLE; // 交换链
    std::vector<VkImage> _swapChainImages; // 交换链图像
    std::vector<VkImageView> _swapChainImageViews; // 交换链图像视图

    VkFormat _swapChainImageFormat; // 交换链图像格式
    VkExtent2D _swapChainExtent; // 交换链扩展

};