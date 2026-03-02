#pragma once

#include <volk/volk.h>

class RenderPass
{
public:
    RenderPass() = default;
    RenderPass(
        VkDevice device,
        VkFormat colorFormat,
        VkAttachmentLoadOp colorLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        VkAttachmentStoreOp colorStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
        VkImageLayout finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    ~RenderPass();

    RenderPass(const RenderPass&) = delete;
    RenderPass& operator=(const RenderPass&) = delete;
    RenderPass(RenderPass&& other) noexcept;
    RenderPass& operator=(RenderPass&& other) noexcept;

    void Recreate(
        VkFormat colorFormat,
        VkAttachmentLoadOp colorLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        VkAttachmentStoreOp colorStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
        VkImageLayout finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    void Destroy() noexcept;

    VkRenderPass Get() const noexcept;
    bool IsValid() const noexcept;

private:
    void Create(
        VkFormat colorFormat,
        VkAttachmentLoadOp colorLoadOp,
        VkAttachmentStoreOp colorStoreOp,
        VkImageLayout finalLayout);

private:
    VkDevice device_{VK_NULL_HANDLE};
    VkRenderPass renderPass_{VK_NULL_HANDLE};
};
