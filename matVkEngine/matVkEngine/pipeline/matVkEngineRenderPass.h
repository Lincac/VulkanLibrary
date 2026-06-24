#pragma once

#include <optional>

#include "device/matVkEngineLogicalDevice.h"

namespace mat {

    class VkEngineRenderPass {
    public:
        VkEngineRenderPass();
        ~VkEngineRenderPass();

        void addColorAttachmentDes(const VkAttachmentDescription& des);

        void addSubpass(const VkSubpassDescription& subpass);

        void addDependency(const VkSubpassDependency& dep);

        void create(VkDevice device);

    private:
        VkEngineRenderPass(const VkEngineRenderPass&) = delete;
        VkEngineRenderPass(VkEngineRenderPass&&) = delete;
        VkEngineRenderPass& operator=(const VkEngineRenderPass&) = delete;
        VkEngineRenderPass& operator=(VkEngineRenderPass&&) = delete;

        VkRenderPass _renderPass = VK_NULL_HANDLE;

        std::vector<VkAttachmentDescription> _attachments;
        std::vector<VkSubpassDescription> _subpasses;
        std::vector<VkSubpassDependency> _dependencies;
    };

};  // namespace mat