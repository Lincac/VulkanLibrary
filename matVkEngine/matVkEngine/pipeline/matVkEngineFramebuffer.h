#pragma once

#include "device/matVkEngineLogicalDevice.h"

namespace mat {

    class VkEngineFramebuffer {
    public:
        VkEngineFramebuffer();
        ~VkEngineFramebuffer();

    private:
        VkEngineFramebuffer(const VkEngineFramebuffer&) = delete;
        VkEngineFramebuffer(VkEngineFramebuffer&&) = delete;
        VkEngineFramebuffer& operator=(const VkEngineFramebuffer&) = delete;
        VkEngineFramebuffer& operator=(VkEngineFramebuffer&&) = delete;
    };

};