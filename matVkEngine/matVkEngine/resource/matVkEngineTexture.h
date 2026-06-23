#pragma once

#include "device/matVkEngineLogicalDevice.h"
#include "resource/matVkEngineImage.h"

namespace mat {

    class VkEngineTexture {
    public:
        VkEngineTexture();
        ~VkEngineTexture();

        void create(std::shared_ptr<VkEngineLogicalDevice> logicalDevice, std::shared_ptr<VkEngineImage> image);

        void release(std::shared_ptr<VkEngineLogicalDevice> logicalDevice);

        std::shared_ptr<VkEngineImage> getImage() const;

        VkSampler getVkSampler() const;

        void getResolution(uint32_t& w, uint32_t& h, uint32_t& d) const;

    private:
        VkEngineTexture(const VkEngineTexture&) = delete;
        VkEngineTexture(VkEngineTexture&&) = delete;
        VkEngineTexture& operator=(const VkEngineTexture&) = delete;
        VkEngineTexture& operator=(VkEngineTexture&&) = delete;

        void createSampler(std::shared_ptr<VkEngineLogicalDevice> logicalDevice, ImageType type);

        std::shared_ptr<VkEngineImage> _image;
        VkSampler _sampler = VK_NULL_HANDLE;
    };

};  // namespace mat
