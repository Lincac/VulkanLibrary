#pragma once

#include <cstdint>
#include <vector>

#include <volk/volk.h>

class SwapchainImageViews
{
public:
    SwapchainImageViews() = default;
    SwapchainImageViews(
        VkDevice device,
        const std::vector<VkImage>& swapchainImages,
        VkFormat imageFormat,
        VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);
    ~SwapchainImageViews();

    SwapchainImageViews(const SwapchainImageViews&) = delete;
    SwapchainImageViews& operator=(const SwapchainImageViews&) = delete;
    SwapchainImageViews(SwapchainImageViews&& other) noexcept;
    SwapchainImageViews& operator=(SwapchainImageViews&& other) noexcept;

    void Recreate(const std::vector<VkImage>& swapchainImages, VkFormat imageFormat);
    void Destroy() noexcept;

    VkImageView Get(uint32_t index) const;
    const std::vector<VkImageView>& GetAll() const noexcept;
    uint32_t GetCount() const noexcept;
    bool IsValid() const noexcept;

private:
    void CreateImageViews(const std::vector<VkImage>& swapchainImages, VkFormat imageFormat);

private:
    VkDevice device_{VK_NULL_HANDLE};
    VkImageAspectFlags aspectMask_{VK_IMAGE_ASPECT_COLOR_BIT};
    std::vector<VkImageView> imageViews_{};
};
