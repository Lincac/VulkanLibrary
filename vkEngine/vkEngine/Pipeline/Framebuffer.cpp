#include "Framebuffer.h"

#include <stdexcept>
#include <utility>

Framebuffer::Framebuffer(
    VkDevice device,
    VkRenderPass renderPass,
    const std::vector<VkImageView>& colorAttachmentViews,
    VkExtent2D extent)
    : device_(device)
{
    if (device_ == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Cannot create framebuffer set with a null device.");
    }

    Create(renderPass, colorAttachmentViews, extent);
}

Framebuffer::~Framebuffer()
{
    Destroy();
}

Framebuffer::Framebuffer(Framebuffer&& other) noexcept
    : device_(other.device_),
      extent_(other.extent_),
      framebuffers_(std::move(other.framebuffers_))
{
    other.device_ = VK_NULL_HANDLE;
    other.extent_ = {};
}

Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    Destroy();

    device_ = other.device_;
    extent_ = other.extent_;
    framebuffers_ = std::move(other.framebuffers_);

    other.device_ = VK_NULL_HANDLE;
    other.extent_ = {};
    return *this;
}

void Framebuffer::Recreate(
    VkRenderPass renderPass,
    const std::vector<VkImageView>& colorAttachmentViews,
    VkExtent2D extent)
{
    if (device_ == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Cannot recreate framebuffers with a null device.");
    }

    Destroy();
    Create(renderPass, colorAttachmentViews, extent);
}

void Framebuffer::Destroy() noexcept
{
    if (device_ == VK_NULL_HANDLE)
    {
        framebuffers_.clear();
        return;
    }

    for (VkFramebuffer framebuffer : framebuffers_)
    {
        if (framebuffer != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(device_, framebuffer, nullptr);
        }
    }

    framebuffers_.clear();
    extent_ = {};
}

VkFramebuffer Framebuffer::Get(uint32_t index) const
{
    if (index >= framebuffers_.size())
    {
        throw std::out_of_range("Framebuffer index is out of range.");
    }
    return framebuffers_[index];
}

const std::vector<VkFramebuffer>& Framebuffer::GetAll() const noexcept
{
    return framebuffers_;
}

uint32_t Framebuffer::GetCount() const noexcept
{
    return static_cast<uint32_t>(framebuffers_.size());
}

bool Framebuffer::IsValid() const noexcept
{
    return device_ != VK_NULL_HANDLE;
}

VkExtent2D Framebuffer::GetExtent() const noexcept
{
    return extent_;
}

void Framebuffer::Create(
    VkRenderPass renderPass,
    const std::vector<VkImageView>& colorAttachmentViews,
    VkExtent2D extent)
{
    if (renderPass == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Cannot create framebuffers with a null render pass.");
    }
    if (colorAttachmentViews.empty())
    {
        throw std::runtime_error("Cannot create framebuffers from empty attachment views.");
    }
    if (extent.width == 0 || extent.height == 0)
    {
        throw std::runtime_error("Cannot create framebuffers with zero extent.");
    }

    extent_ = extent;
    framebuffers_.clear();
    framebuffers_.reserve(colorAttachmentViews.size());

    for (VkImageView colorView : colorAttachmentViews)
    {
        VkImageView attachments[] = {colorView};

        VkFramebufferCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.renderPass = renderPass;
        createInfo.attachmentCount = 1;
        createInfo.pAttachments = attachments;
        createInfo.width = extent.width;
        createInfo.height = extent.height;
        createInfo.layers = 1;

        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        if (vkCreateFramebuffer(device_, &createInfo, nullptr, &framebuffer) != VK_SUCCESS)
        {
            for (VkFramebuffer createdFramebuffer : framebuffers_)
            {
                vkDestroyFramebuffer(device_, createdFramebuffer, nullptr);
            }
            framebuffers_.clear();
            extent_ = {};
            throw std::runtime_error("Failed to create Vulkan framebuffer.");
        }

        framebuffers_.push_back(framebuffer);
    }
}
