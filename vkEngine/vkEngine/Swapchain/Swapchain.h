#pragma once

#include <cstdint>
#include <vector>

#include <volk/volk.h>

#include "SwapchainImageViews.h"

struct GLFWwindow;

class Context;

class Swapchain
{
public:
    Swapchain(Context& context, GLFWwindow* window, bool enableVSync = true);
    ~Swapchain();

    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;
    Swapchain(Swapchain&& other) noexcept;
    Swapchain& operator=(Swapchain&& other) noexcept;

    void Recreate();
    uint32_t AcquireNextImage(VkSemaphore imageAvailableSemaphore, VkFence inFlightFence = VK_NULL_HANDLE) const;
    VkResult Present(uint32_t imageIndex, const std::vector<VkSemaphore>& waitSemaphores = {}) const;

    VkSwapchainKHR Get() const noexcept;
    VkSurfaceKHR GetSurface() const noexcept;
    VkFormat GetImageFormat() const noexcept;
    VkExtent2D GetExtent() const noexcept;
    VkPresentModeKHR GetPresentMode() const noexcept;
    uint32_t GetImageCount() const noexcept;

    const std::vector<VkImage>& GetImages() const noexcept;
    const std::vector<VkImageView>& GetImageViews() const noexcept;

private:
    struct SwapchainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities{};
        std::vector<VkSurfaceFormatKHR> formats{};
        std::vector<VkPresentModeKHR> presentModes{};
    };

    void CreateSurface();
    void CreateSwapchain();
    void CreateImageViews();
    void CleanupSwapchain() noexcept;
    void CleanupSurface() noexcept;
    void Destroy() noexcept;

    SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice physicalDevice) const;
    uint32_t FindPresentQueueFamily(VkPhysicalDevice physicalDevice) const;
    VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const;
    VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& presentModes) const;
    VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

private:
    Context* context_{nullptr};
    GLFWwindow* window_{nullptr};
    bool enableVSync_{true};

    VkSurfaceKHR surface_{VK_NULL_HANDLE};
    VkSwapchainKHR swapchain_{VK_NULL_HANDLE};
    VkFormat imageFormat_{VK_FORMAT_UNDEFINED};
    VkExtent2D extent_{};
    VkPresentModeKHR presentMode_{VK_PRESENT_MODE_FIFO_KHR};

    uint32_t presentQueueFamilyIndex_{0};
    VkQueue presentQueue_{VK_NULL_HANDLE};

    std::vector<VkImage> images_{};
    SwapchainImageViews imageViews_{};
};
