#include "RenderPass.h"

#include <stdexcept>

bool RenderPass::isInitialized() const {
    return device != VK_NULL_HANDLE && renderPass != VK_NULL_HANDLE;
}

void RenderPass::initialize(
    VkDevice inDevice,
    VkFormat colorFormat,
    VkExtent2D inExtent,
    const std::vector<VkImageView>& swapchainImageViews) {
    if (inDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("RenderPass::initialize: device is null");
    }
    if (swapchainImageViews.empty()) {
        throw std::runtime_error("RenderPass::initialize: swapchain image views are empty");
    }

    if (renderPass != VK_NULL_HANDLE || !framebuffers.empty()) {
        cleanup();
    }

    device = inDevice;
    extent = inExtent;

    createRenderPass(colorFormat);
    createFramebuffers(swapchainImageViews);
}

void RenderPass::cleanup() {
    destroyFramebuffers();

    if (device != VK_NULL_HANDLE && renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, renderPass, nullptr);
    }
    renderPass = VK_NULL_HANDLE;
    device = VK_NULL_HANDLE;
    extent = {};
}

void RenderPass::recreateFramebuffers(
    VkExtent2D newExtent,
    const std::vector<VkImageView>& swapchainImageViews) {
    if (!isInitialized()) {
        throw std::runtime_error("RenderPass::recreateFramebuffers: not initialized");
    }
    if (swapchainImageViews.empty()) {
        throw std::runtime_error("RenderPass::recreateFramebuffers: swapchain image views are empty");
    }

    extent = newExtent;
    destroyFramebuffers();
    createFramebuffers(swapchainImageViews);
}

VkFramebuffer RenderPass::getFramebuffer(uint32_t index) const {
    if (index >= framebuffers.size()) {
        throw std::runtime_error("RenderPass::getFramebuffer: index out of range");
    }
    return framebuffers[index];
}

void RenderPass::createRenderPass(VkFormat colorFormat) {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = colorFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

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
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass");
    }
}

void RenderPass::createFramebuffers(const std::vector<VkImageView>& swapchainImageViews) {
    framebuffers.resize(swapchainImageViews.size(), VK_NULL_HANDLE);

    for (size_t i = 0; i < swapchainImageViews.size(); ++i) {
        VkImageView attachments[] = { swapchainImageViews[i] };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS) {
            // Partial creation happened, destroy existing framebuffers first.
            destroyFramebuffers();
            throw std::runtime_error("Failed to create framebuffer");
        }
    }
}

void RenderPass::destroyFramebuffers() {
    if (device == VK_NULL_HANDLE) {
        framebuffers.clear();
        return;
    }

    for (VkFramebuffer framebuffer : framebuffers) {
        if (framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
    }
    framebuffers.clear();
}
