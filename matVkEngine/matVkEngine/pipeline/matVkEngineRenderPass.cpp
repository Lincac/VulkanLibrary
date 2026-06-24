#include "matVkEngineRenderPass.h"

#include <stdexcept>

namespace mat {

    VkEngineRenderPass::VkEngineRenderPass() {}

    VkEngineRenderPass::~VkEngineRenderPass() {}

    void VkEngineRenderPass::addColorAttachmentDes(const VkAttachmentDescription& des) {
        _attachments.push_back(des);
    }

    void VkEngineRenderPass::addSubpass(const VkSubpassDescription& subpass) {
        _subpasses.push_back(subpass);
    }

    void VkEngineRenderPass::addDependency(const VkSubpassDependency& dep) {
        _dependencies.push_back(dep);
    }

    void VkEngineRenderPass::create(VkDevice device) {
        VkRenderPassCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        info.attachmentCount = _attachments.size();
        info.pAttachments = _attachments.data();
        info.subpassCount = _subpasses.size();
        info.pSubpasses = _subpasses.data();
        info.dependencyCount = _dependencies.size();
        info.pDependencies = _dependencies.data();

        if (vkCreateRenderPass(device, &info, nullptr, &_renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
    }

};  // namespace mat