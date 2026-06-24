#pragma once

#include "device/matVkEngineLogicalDevice.h"

namespace mat {

    class VkEngineFramebuffer {
    public:
        VkEngineFramebuffer();
        ~VkEngineFramebuffer();

        void create(VkDevice device, VkRenderPass renderPass, std::vector<VkImageView> attachments, uint32_t w,
                    uint32_t h, uint32_t layers = 1);

    private:
        VkEngineFramebuffer(const VkEngineFramebuffer&) = delete;
        VkEngineFramebuffer(VkEngineFramebuffer&&) = delete;
        VkEngineFramebuffer& operator=(const VkEngineFramebuffer&) = delete;
        VkEngineFramebuffer& operator=(VkEngineFramebuffer&&) = delete;

        VkFramebuffer _frameBuffer = VK_NULL_HANDLE;
    };

};  // namespace mat