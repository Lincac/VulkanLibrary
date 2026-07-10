#pragma once

#include "matVkEngineImage.h"

namespace mat {

    class VkEngineTexture {
    public:
        VkEngineTexture();
        ~VkEngineTexture();

        void create(VkDevice logDevice, const VkEngineImage& image);

        void release(VkDevice logDevice);

        VkSampler getVkSampler() const;

    private:
        VkEngineTexture(const VkEngineTexture&) = delete;
        VkEngineTexture(VkEngineTexture&&) = delete;
        VkEngineTexture& operator=(const VkEngineTexture&) = delete;
        VkEngineTexture& operator=(VkEngineTexture&&) = delete;

        VkSampler _sampler = VK_NULL_HANDLE;
    };

};  // namespace mat
