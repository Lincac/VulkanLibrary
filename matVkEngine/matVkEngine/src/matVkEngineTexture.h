#pragma once

#include "matVkEngineImage.h"

#include <memory>

namespace mat {

    class VkEngineTexture {
    public:
        VkEngineTexture();
        ~VkEngineTexture();

        void create(VkDevice logDevice, std::shared_ptr<VkEngineImage> image);

        void release(VkDevice logDevice);

        std::shared_ptr<VkEngineImage> getImage() const;

        VkSampler getVkSampler() const;

        void getResolution(uint32_t& w, uint32_t& h, uint32_t& d) const;

    private:
        VkEngineTexture(const VkEngineTexture&) = delete;
        VkEngineTexture(VkEngineTexture&&) = delete;
        VkEngineTexture& operator=(const VkEngineTexture&) = delete;
        VkEngineTexture& operator=(VkEngineTexture&&) = delete;

        std::shared_ptr<VkEngineImage> _image;
        VkSampler _sampler = VK_NULL_HANDLE;
    };

};  // namespace mat
