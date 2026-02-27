#pragma once

#include <volk.h>
#include <vector>

class RenderPass {
public:
    // 创建 RenderPass 与对应的 Framebuffer 列表（1:1 对应 swapchain image views）。
    void initialize(
        VkDevice inDevice,
        VkFormat colorFormat,
        VkExtent2D inExtent,
        const std::vector<VkImageView>& swapchainImageViews);

    // 销毁 Framebuffer 与 RenderPass。
    void cleanup();

    // 仅重建 Framebuffer（适用于 swapchain 重建后 image views 变化）。
    void recreateFramebuffers(
        VkExtent2D newExtent,
        const std::vector<VkImageView>& swapchainImageViews);

    VkRenderPass getRenderPass() const { return renderPass; }
    VkExtent2D getExtent() const { return extent; }
    const std::vector<VkFramebuffer>& getFramebuffers() const { return framebuffers; }
    VkFramebuffer getFramebuffer(uint32_t index) const;

private:
    bool isInitialized() const;
    void createRenderPass(VkFormat colorFormat);
    void createFramebuffers(const std::vector<VkImageView>& swapchainImageViews);
    void destroyFramebuffers();

private:
    VkDevice device = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkExtent2D extent{};
    std::vector<VkFramebuffer> framebuffers;
};
