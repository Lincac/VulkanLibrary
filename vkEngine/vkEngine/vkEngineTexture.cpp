#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "vkEngineTexture.h"
#include "vkEngineBuffer.h"
#include "vkEngineHelp.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace {

float luminance(glm::vec3 c)
{
    return 0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b;
}

} // namespace

vkEngineTexture::vkEngineTexture(std::shared_ptr<vkEngineLogicalDevice> device)
    : _device(device)
{
}

vkEngineTexture::~vkEngineTexture()
{
    auto device = _device->getVkDevice();

    if (_envCdfSampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, _envCdfSampler, nullptr);
        _envCdfSampler = VK_NULL_HANDLE;
    }

    if (_envCdfImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(device, _envCdfImageView, nullptr);
        _envCdfImageView = VK_NULL_HANDLE;
    }

    if (_envCdfImage != VK_NULL_HANDLE) {
        vkDestroyImage(device, _envCdfImage, nullptr);
        _envCdfImage = VK_NULL_HANDLE;
    }

    if (_envCdfMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device, _envCdfMemory, nullptr);
        _envCdfMemory = VK_NULL_HANDLE;
    }

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

float vkEngineTexture::sampleBilinearLuminance(const float* pixels, int width, int height, float u, float v)
{
    u = u * static_cast<float>(width) - 0.5f;
    v = v * static_cast<float>(height) - 0.5f;

    const int x0 = static_cast<int>(std::floor(u));
    const int y0 = static_cast<int>(std::floor(v));
    const int x1 = x0 + 1;
    const int y1 = y0 + 1;

    const float tx = u - static_cast<float>(x0);
    const float ty = v - static_cast<float>(y0);

    auto fetch = [&](int x, int y) {
        x = (x % width + width) % width;
        y = std::clamp(y, 0, height - 1);
        const float* px = pixels + (static_cast<size_t>(y) * width + x) * 4;
        return luminance(glm::vec3(px[0], px[1], px[2]));
    };

    const float c00 = fetch(x0, y0);
    const float c10 = fetch(x1, y0);
    const float c01 = fetch(x0, y1);
    const float c11 = fetch(x1, y1);

    const float cx0 = c00 + (c10 - c00) * tx;
    const float cx1 = c01 + (c11 - c01) * tx;
    return cx0 + (cx1 - cx0) * ty;
}

void vkEngineTexture::buildEnvironmentCdf(std::shared_ptr<vkEngineCommandPool> commandPool,
    const float* pixels, int srcWidth, int srcHeight)
{
    const int cdfW = _envCdfResolution.x;
    const int cdfH = _envCdfResolution.y;

    std::vector<float> weights(static_cast<size_t>(cdfW) * cdfH);
    std::vector<float> rowSums(cdfH, 0.0f);

    for (int j = 0; j < cdfH; ++j) {
        const float v = (static_cast<float>(j) + 0.5f) / static_cast<float>(cdfH);
        const float theta = v * 3.14159265358979323846f;
        const float sinTheta = std::sin(theta);

        for (int i = 0; i < cdfW; ++i) {
            const float u = (static_cast<float>(i) + 0.5f) / static_cast<float>(cdfW);
            const float w = sampleBilinearLuminance(pixels, srcWidth, srcHeight, u, v) * sinTheta;
            weights[static_cast<size_t>(j) * cdfW + i] = w;
            rowSums[j] += w;
        }
    }

    std::vector<float> cdfPixels(static_cast<size_t>(cdfW) * cdfH * 2);
    float totalSum = 0.0f;
    for (float rowSum : rowSums) {
        totalSum += rowSum;
    }

    if (totalSum <= 0.0f) {
        totalSum = 1.0f;
        for (float& rowSum : rowSums) {
            rowSum = 1.0f / static_cast<float>(cdfH);
        }
    }

    float marginalAccum = 0.0f;
    for (int j = 0; j < cdfH; ++j) {
        marginalAccum += rowSums[j] / totalSum;
        const float rowSum = rowSums[j] > 0.0f ? rowSums[j] : 1.0f;

        float condAccum = 0.0f;
        for (int i = 0; i < cdfW; ++i) {
            condAccum += weights[static_cast<size_t>(j) * cdfW + i] / rowSum;
            const size_t idx = (static_cast<size_t>(j) * cdfW + i) * 2;
            cdfPixels[idx + 0] = condAccum;
            cdfPixels[idx + 1] = marginalAccum;
        }
    }

    auto physicalDevice = _device->getPhysicalDevice()->getVkPhysicalDevice();
    auto device = _device->getVkDevice();

    VkImageCreateInfo imageInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = { static_cast<uint32_t>(cdfW), static_cast<uint32_t>(cdfH), 1 };
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R32G32_SFLOAT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, &_envCdfImage) != VK_SUCCESS) {
        throw std::runtime_error("failed to create environment CDF image!");
    }

    VkMemoryRequirements memReq{};
    vkGetImageMemoryRequirements(device, _envCdfImage, &memReq);

    VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = findMemoryType(
        physicalDevice, memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &_envCdfMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate environment CDF memory!");
    }

    vkBindImageMemory(device, _envCdfImage, _envCdfMemory, 0);

    VkImageViewCreateInfo viewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    viewInfo.image = _envCdfImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R32G32_SFLOAT;
    viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    if (vkCreateImageView(device, &viewInfo, nullptr, &_envCdfImageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create environment CDF image view!");
    }

    VkSamplerCreateInfo samplerInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &_envCdfSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create environment CDF sampler!");
    }

    const VkDeviceSize pixelBytes = static_cast<VkDeviceSize>(cdfW) * cdfH * 2 * sizeof(float);

    vkEngineBuffer stagingBuffer(_device);
    stagingBuffer.setSize(pixelBytes);
    stagingBuffer.setUsage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    stagingBuffer.setMemoryProperties(
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    stagingBuffer.create();

    void* mapped = nullptr;
    vkMapMemory(device, stagingBuffer.getMemory(), 0, pixelBytes, 0, &mapped);
    std::memcpy(mapped, cdfPixels.data(), static_cast<size_t>(pixelBytes));
    vkUnmapMemory(device, stagingBuffer.getMemory());

    commandPool->submitOneTimeCommands([&](VkCommandBuffer cmd) {
        transitionImageLayout(cmd, _envCdfImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkBufferImageCopy region{};
        region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        region.imageExtent = { static_cast<uint32_t>(cdfW), static_cast<uint32_t>(cdfH), 1 };

        vkCmdCopyBufferToImage(cmd, stagingBuffer.getBuffer(), _envCdfImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        transitionImageLayout(cmd, _envCdfImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    });
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

    buildEnvironmentCdf(commandPool, pixels, width, height);

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

VkImageView& vkEngineTexture::getEnvCdfImageView()
{
    return _envCdfImageView;
}

VkSampler& vkEngineTexture::getEnvCdfSampler()
{
    return _envCdfSampler;
}

glm::ivec2 vkEngineTexture::getEnvCdfResolution() const
{
    return _envCdfResolution;
}
