#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#include "vkEngineImage.h"
#include "vkEngineBuffer.h"
#include "vkEngineHelp.h"

#include <vector>

vkEngineImage::vkEngineImage(std::shared_ptr<vkEngineLogicalDevice> device, glm::ivec2 resolution)
                             : _device(device)
{
    _format = VK_FORMAT_R8G8B8A8_UNORM;
    _usage = VK_IMAGE_USAGE_STORAGE_BIT;
}

vkEngineImage::~vkEngineImage()
{
    auto device = _device->getVkDevice();

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

void vkEngineImage::setResolution(glm::ivec2 resolution)
{
    _resolution = resolution;
}

void vkEngineImage::setFormat(VkFormat format)
{
    _format = format;
}

void vkEngineImage::setImageUsageFlags(VkImageUsageFlags usage)
{
    _usage = usage;
}

void vkEngineImage::generate()
{
    auto physicalDevice = _device->getPhysicalDevice();

    VkImageCreateInfo imageInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = { (uint32_t)_resolution.x, (uint32_t)_resolution.y, 1 };
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = _format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = _usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateImage(_device->getVkDevice(), &imageInfo, nullptr, &_image);    
    
    VkMemoryRequirements memReq{};
    vkGetImageMemoryRequirements(_device->getVkDevice(), _image, &memReq);

    VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = findMemoryType(
        physicalDevice->getVkPhysicalDevice(),
        memReq.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vkAllocateMemory(_device->getVkDevice(), &allocInfo, nullptr, &_memory);
    vkBindImageMemory(_device->getVkDevice(), _image, _memory, 0);

    VkImageViewCreateInfo viewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    viewInfo.image = _image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = _format;
    viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

    vkCreateImageView(_device->getVkDevice(), &viewInfo, nullptr, &_imageView);
}

VkImage &vkEngineImage::getImage()
{
    return _image;
}

VkImageView &vkEngineImage::getImageView()
{
    return _imageView;
}

VkDeviceMemory &vkEngineImage::getMemory()
{
    return _memory;
}

glm::ivec2 vkEngineImage::getResolution() const
{
    return _resolution;
}

void vkEngineImage::saveToPng(vkEngineCommandPool& commandPool, const std::string& filepath,
    std::function<void(VkCommandBuffer)> beforeCopy)
{
    const uint32_t width = static_cast<uint32_t>(_resolution.x);
    const uint32_t height = static_cast<uint32_t>(_resolution.y);
    const VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * height * 4;

    auto device = _device->getVkDevice();

    vkEngineBuffer stagingBuffer(_device);
    stagingBuffer.setSize(imageSize);
    stagingBuffer.setUsage(VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    stagingBuffer.setMemoryProperties(
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    stagingBuffer.create();

    commandPool.submitOneTimeCommands([&](VkCommandBuffer cmd) {
        if (beforeCopy) {
            beforeCopy(cmd);
        }

        VkImageMemoryBarrier barrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = _image;
        barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkBufferImageCopy region{};
        region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        region.imageExtent = { width, height, 1 };

        vkCmdCopyImageToBuffer(cmd, _image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            stagingBuffer.getBuffer(), 1, &region);
    });

    void* mapped = nullptr;
    vkMapMemory(device, stagingBuffer.getMemory(), 0, imageSize, 0, &mapped);

    if (!stbi_write_png(filepath.c_str(), static_cast<int>(width), static_cast<int>(height),
            4, mapped, static_cast<int>(width * 4))) {
        vkUnmapMemory(device, stagingBuffer.getMemory());
        throw std::runtime_error("failed to write png: " + filepath);
    }

    vkUnmapMemory(device, stagingBuffer.getMemory());
}
