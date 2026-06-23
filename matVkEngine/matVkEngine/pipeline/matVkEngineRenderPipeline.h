#pragma once

#include "device/matVkEngineLogicalDevice.h"

namespace mat {

    class VkEngineRenderPipeline {
    public:
        VkEngineRenderPipeline();
        ~VkEngineRenderPipeline();

    private:
        VkEngineRenderPipeline(const VkEngineRenderPipeline&) = delete;
        VkEngineRenderPipeline(VkEngineRenderPipeline&&) = delete;
        VkEngineRenderPipeline& operator=(const VkEngineRenderPipeline&) = delete;
        VkEngineRenderPipeline& operator=(VkEngineRenderPipeline&&) = delete;

        VkPipeline _pipeline = VK_NULL_HANDLE;
    };

};  // namespace mat