#include "matVkEngineTexture.h"

namespace mat {

    VkEngineTexture::VkEngineTexture() {}

    VkEngineTexture::~VkEngineTexture() {}

    void VkEngineTexture::create(VkDevice logDevice, std::shared_ptr<VkEngineImage> image) {
        if (image == nullptr) {
            VK_ERROR("texture source image is null!");
        }
        if (image->getVkImage() == VK_NULL_HANDLE) {
            VK_ERROR("texture source image is not created!");
        }

        release(logDevice);

        _image = image;

        VkSamplerCreateInfo samplerInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = image->getImageType() == ImageType::HDR2D ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
                                                                             : VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;

        if (image->getImageType() == ImageType::Volume3D) {
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        }

        VK_CHECK(vkCreateSampler(logDevice, &samplerInfo, nullptr, &_sampler));
    }

    void VkEngineTexture::release(VkDevice logDevice) {
        if (logDevice == nullptr) {
            VK_ERROR("Logical Device is nullptr!");
        }

        if (_sampler != VK_NULL_HANDLE) {
            vkDestroySampler(logDevice, _sampler, nullptr);
            _sampler = VK_NULL_HANDLE;
        }

        _image.reset();
    }

    std::shared_ptr<VkEngineImage> VkEngineTexture::getImage() const {
        return _image;
    }

    VkSampler VkEngineTexture::getVkSampler() const {
        return _sampler;
    }

    void VkEngineTexture::getResolution(uint32_t& w, uint32_t& h, uint32_t& d) const {
        if (_image == nullptr) {
            w = 0;
            h = 0;
            d = 0;
            return;
        }

        _image->getResolution(w, h, d);
    }

};  // namespace mat
