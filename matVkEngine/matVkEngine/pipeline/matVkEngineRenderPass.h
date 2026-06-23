#pragma once

#include "device/matVkEngineLogicalDevice.h"

namespace mat {

    class VkEngineRenderPass {
    public:
        VkEngineRenderPass();
        ~VkEngineRenderPass();

    private:
        VkEngineRenderPass(const VkEngineRenderPass&) = delete;
        VkEngineRenderPass(VkEngineRenderPass&&) = delete;
        VkEngineRenderPass& operator=(const VkEngineRenderPass&) = delete;
        VkEngineRenderPass& operator=(VkEngineRenderPass&&) = delete;

        VkRenderPass _renderPass = VK_NULL_HANDLE;
        std::vector<VkSubpassDescription> _subpasses;
    };

};  // namespace mat