#include "RenderPass.h"

#include <stdexcept>
#include <utility>

RenderPass::RenderPass(
    VkDevice device,
    VkFormat colorFormat,
    VkAttachmentLoadOp colorLoadOp,
    VkAttachmentStoreOp colorStoreOp,
    VkImageLayout finalLayout)
    : device_(device)
{
    if (device_ == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Cannot create render pass with a null device.");
    }

    Create(colorFormat, colorLoadOp, colorStoreOp, finalLayout);
}

RenderPass::~RenderPass()
{
    Destroy();
}

RenderPass::RenderPass(RenderPass&& other) noexcept
    : device_(other.device_),
      renderPass_(other.renderPass_)
{
    other.device_ = VK_NULL_HANDLE;
    other.renderPass_ = VK_NULL_HANDLE;
}

RenderPass& RenderPass::operator=(RenderPass&& other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    Destroy();

    device_ = other.device_;
    renderPass_ = other.renderPass_;
    other.device_ = VK_NULL_HANDLE;
    other.renderPass_ = VK_NULL_HANDLE;
    return *this;
}

void RenderPass::Recreate(
    VkFormat colorFormat,
    VkAttachmentLoadOp colorLoadOp,
    VkAttachmentStoreOp colorStoreOp,
    VkImageLayout finalLayout)
{
    if (device_ == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Cannot recreate render pass with a null device.");
    }

    Destroy();
    Create(colorFormat, colorLoadOp, colorStoreOp, finalLayout);
}

void RenderPass::Destroy() noexcept
{
    if (device_ != VK_NULL_HANDLE && renderPass_ != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(device_, renderPass_, nullptr);
    }
    renderPass_ = VK_NULL_HANDLE;
}

VkRenderPass RenderPass::Get() const noexcept
{
    return renderPass_;
}

bool RenderPass::IsValid() const noexcept
{
    return renderPass_ != VK_NULL_HANDLE;
}

void RenderPass::Create(
    VkFormat colorFormat,
    VkAttachmentLoadOp colorLoadOp,
    VkAttachmentStoreOp colorStoreOp,
    VkImageLayout finalLayout)
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = colorFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = colorLoadOp;
    colorAttachment.storeOp = colorStoreOp;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = finalLayout;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = 1;
    createInfo.pAttachments = &colorAttachment;
    createInfo.subpassCount = 1;
    createInfo.pSubpasses = &subpass;
    createInfo.dependencyCount = 1;
    createInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device_, &createInfo, nullptr, &renderPass_) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan render pass.");
    }
}
