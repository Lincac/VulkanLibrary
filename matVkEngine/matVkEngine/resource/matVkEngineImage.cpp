#include "matVkEngineImage.h"

#include <stdexcept>

#include "common/matVkEngineCommon.h"

namespace mat {

    VkEngineImage::VkEngineImage() : width(1), height(1), depth(1) {
        _format = VK_FORMAT_R8G8B8A8_UNORM;
        _usage = VK_IMAGE_USAGE_STORAGE_BIT;
    }

    VkEngineImage::~VkEngineImage() {}

    void VkEngineImage::setResolution(uint32_t w, uint32_t h, uint32_t d) {
        width = w;
        height = h;
        depth = d;
    }

    void VkEngineImage::setVkFormat(VkFormat format) {
        _format = format;
    }

    void VkEngineImage::setVkImageUsageFlags(VkImageUsageFlags usage) {
        _usage = usage;
    }

    void VkEngineImage::create(std::shared_ptr<VkEnginePhysicalDevice> physicalDevice,
                               std::shared_ptr<VkEngineLogicalDevice> logicalDevice) {
        VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = {width, height, depth};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = _format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = _usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        vkCreateImage(logicalDevice->getVkDevice(), &imageInfo, nullptr, &_image);

        VkMemoryRequirements memReq{};
        vkGetImageMemoryRequirements(logicalDevice->getVkDevice(), _image, &memReq);

        VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex = findMemoryType(physicalDevice->getVkPhysicalDevice(), memReq.memoryTypeBits,
                                                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        vkAllocateMemory(logicalDevice->getVkDevice(), &allocInfo, nullptr, &_memory);
        vkBindImageMemory(logicalDevice->getVkDevice(), _image, _memory, 0);

        VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.image = _image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = _format;
        viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

        vkCreateImageView(logicalDevice->getVkDevice(), &viewInfo, nullptr, &_imageView);
    }

    void VkEngineImage::getResolution(uint32_t& w, uint32_t& h, uint32_t& d) {
        w = width;
        h = height;
        d = depth;
    }

    VkImage VkEngineImage::getVkImage() const {
        return _image;
    }

    VkImageView VkEngineImage::getVkImageView() const {
        return _imageView;
    }

    VkDeviceMemory VkEngineImage::getVkDeviceMemory() const {
        return _memory;
    }

    void VkEngineImage::release(std::shared_ptr<VkEngineLogicalDevice> logicalDevice) {
        if (logicalDevice == nullptr) {
            throw std::runtime_error("Logical Device is nullptr!");
        }

        width = 1;
        height = 1;

        _format = VK_FORMAT_R8G8B8A8_UNORM;
        _usage = VK_IMAGE_USAGE_STORAGE_BIT;

        if (_imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(logicalDevice->getVkDevice(), _imageView, nullptr);
            _imageView = VK_NULL_HANDLE;
        }

        if (_image != VK_NULL_HANDLE) {
            vkDestroyImage(logicalDevice->getVkDevice(), _image, nullptr);
            _image = VK_NULL_HANDLE;
        }

        if (_memory != VK_NULL_HANDLE) {
            vkFreeMemory(logicalDevice->getVkDevice(), _memory, nullptr);
            _memory = VK_NULL_HANDLE;
        }
    }

};  // namespace mat