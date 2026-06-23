#include "matVkEngineTexture.h"

#include <stdexcept>

namespace mat {

    VkEngineTexture::VkEngineTexture() {}

    VkEngineTexture::~VkEngineTexture() {}

    void VkEngineTexture::create(std::shared_ptr<VkEngineLogicalDevice> logicalDevice,
                                 std::shared_ptr<VkEngineImage> image) {
        if (image == nullptr) {
            throw std::runtime_error("texture source image is null!");
        }
        if (image->getVkImage() == VK_NULL_HANDLE) {
            throw std::runtime_error("texture source image is not created!");
        }

        release(logicalDevice);

        _image = image;
        createSampler(logicalDevice, _image->getImageType());
    }

    void VkEngineTexture::release(std::shared_ptr<VkEngineLogicalDevice> logicalDevice) {
        if (logicalDevice == nullptr) {
            throw std::runtime_error("Logical Device is nullptr!");
        }

        if (_sampler != VK_NULL_HANDLE) {
            vkDestroySampler(logicalDevice->getVkDevice(), _sampler, nullptr);
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

    void VkEngineTexture::createSampler(std::shared_ptr<VkEngineLogicalDevice> logicalDevice, ImageType type) {
        if (_sampler != VK_NULL_HANDLE) {
            return;
        }

        VkSamplerCreateInfo samplerInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV =
            type == ImageType::HDR2D ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;

        if (type == ImageType::Volume3D) {
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        }

        if (vkCreateSampler(logicalDevice->getVkDevice(), &samplerInfo, nullptr, &_sampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }

};  // namespace mat
