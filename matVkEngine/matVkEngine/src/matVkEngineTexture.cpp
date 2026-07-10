#include "matVkEngineTexture.h"

namespace mat {

    VkEngineTexture::VkEngineTexture() {}

    VkEngineTexture::~VkEngineTexture() {}

    void VkEngineTexture::create(VkDevice logDevice, const VkEngineImage& image) {
        if (image.getImage() == VK_NULL_HANDLE) {
            VK_ERROR("texture source image is not created!");
        }

        release(logDevice);

        VkSamplerCreateInfo samplerInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;

        if (image.getImageType() == ImageType::Volume3D) {
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
    }

    VkSampler VkEngineTexture::getVkSampler() const {
        return _sampler;
    }

};  // namespace mat
