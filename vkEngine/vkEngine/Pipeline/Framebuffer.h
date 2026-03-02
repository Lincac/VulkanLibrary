#pragma once

#include <cstdint>
#include <vector>

#include <volk/volk.h>

class Framebuffer
{
public:
    Framebuffer() = default;
    Framebuffer(
        VkDevice device,
        VkRenderPass renderPass,
        const std::vector<VkImageView>& colorAttachmentViews,
        VkExtent2D extent);
    ~Framebuffer();

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;
    Framebuffer(Framebuffer&& other) noexcept;
    Framebuffer& operator=(Framebuffer&& other) noexcept;

    void Recreate(
        VkRenderPass renderPass,
        const std::vector<VkImageView>& colorAttachmentViews,
        VkExtent2D extent);
    void Destroy() noexcept;

    VkFramebuffer Get(uint32_t index) const;
    const std::vector<VkFramebuffer>& GetAll() const noexcept;
    uint32_t GetCount() const noexcept;
    bool IsValid() const noexcept;
    VkExtent2D GetExtent() const noexcept;

private:
    void Create(
        VkRenderPass renderPass,
        const std::vector<VkImageView>& colorAttachmentViews,
        VkExtent2D extent);

private:
    VkDevice device_{VK_NULL_HANDLE};
    VkExtent2D extent_{};
    std::vector<VkFramebuffer> framebuffers_{};
};
