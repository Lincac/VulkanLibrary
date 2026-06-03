#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "vkEngineTexture.h"
#include "vkEngineBuffer.h"
#include "vkEngineHelp.h"

#include <cstring>
#include <vector>

vkEngineTexture::vkEngineTexture(std::shared_ptr<vkEngineLogicalDevice> device)
    : _device(device)
{
}

vkEngineTexture::~vkEngineTexture()
{
    auto device = _device->getVkDevice();

    if (_sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, _sampler, nullptr);
        _sampler = VK_NULL_HANDLE;
    }

    if (_imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, _imageView, nullptr);
        _imageView = VK_NULL_HANDLE;
    }

    if (_image != VK_NULL_HANDLE) {
        vkDestroyImage(device, _image, nullptr);
        _image = VK_NULL_HANDLE;
    }

    if (_memory != VK_NULL_HANDLE) {
        vkFreeMemory(device, _memory, nullptr);
        _memory = VK_NULL_HANDLE;
    }
}

void vkEngineTexture::loadHdr(std::shared_ptr<vkEngineCommandPool> commandPool, const std::string& relativePath)
{
    const std::string path = resolvePathNextToExe(relativePath);

    int width = 0;
    int height = 0;
    int channels = 0;
    float* pixels = stbi_loadf(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels || width <= 0 || height <= 0) {
        throw std::runtime_error("failed to load HDR image: " + path);
    }

    _resolution = glm::ivec2(width, height);
    const VkDeviceSize pixelBytes = static_cast<VkDeviceSize>(width) * height * 4 * sizeof(float);

    auto physicalDevice = _device->getPhysicalDevice()->getVkPhysicalDevice();
    auto device = _device->getVkDevice();

    VkImageCreateInfo imageInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, &_image) != VK_SUCCESS) {
        stbi_image_free(pixels);
        throw std::runtime_error("failed to create HDR texture image!");
    }

    VkMemoryRequirements memReq{};
    vkGetImageMemoryRequirements(device, _image, &memReq);

    VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = findMemoryType(
        physicalDevice, memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &_memory) != VK_SUCCESS) {
        stbi_image_free(pixels);
        throw std::runtime_error("failed to allocate HDR texture memory!");
    }

    vkBindImageMemory(device, _image, _memory, 0);

    VkImageViewCreateInfo viewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    viewInfo.image = _image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    if (vkCreateImageView(device, &viewInfo, nullptr, &_imageView) != VK_SUCCESS) {
        stbi_image_free(pixels);
        throw std::runtime_error("failed to create HDR texture image view!");
    }

    VkSamplerCreateInfo samplerInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &_sampler) != VK_SUCCESS) {
        stbi_image_free(pixels);
        throw std::runtime_error("failed to create HDR texture sampler!");
    }

    vkEngineBuffer stagingBuffer(_device);
    stagingBuffer.setSize(pixelBytes);
    stagingBuffer.setUsage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    stagingBuffer.setMemoryProperties(
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    stagingBuffer.create();

    void* mapped = nullptr;
    vkMapMemory(device, stagingBuffer.getMemory(), 0, pixelBytes, 0, &mapped);
    std::memcpy(mapped, pixels, static_cast<size_t>(pixelBytes));
    vkUnmapMemory(device, stagingBuffer.getMemory());
    stbi_image_free(pixels);

    commandPool->submitOneTimeCommands([&](VkCommandBuffer cmd) {
        transitionImageLayout(cmd, _image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkBufferImageCopy region{};
        region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        region.imageExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1 };

        vkCmdCopyBufferToImage(cmd, stagingBuffer.getBuffer(), _image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        transitionImageLayout(cmd, _image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    });
}

VkImageView& vkEngineTexture::getImageView()
{
    return _imageView;
}

VkSampler& vkEngineTexture::getSampler()
{
    return _sampler;
}

glm::ivec2 vkEngineTexture::getResolution() const
{
    return _resolution;
}
