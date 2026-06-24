#include "matVkEngineFramebuffer.h"

#include <stdexcept>

namespace mat {

    VkEngineFramebuffer::VkEngineFramebuffer() {}

    VkEngineFramebuffer::~VkEngineFramebuffer() {}

    void VkEngineFramebuffer::create(VkDevice device, VkRenderPass renderPass, std::vector<VkImageView> attachments,
                                     uint32_t w, uint32_t h, uint32_t layers) {
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = w;
        framebufferInfo.height = h;
        framebufferInfo.layers = layers;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &_frameBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }

};  // namespace mat