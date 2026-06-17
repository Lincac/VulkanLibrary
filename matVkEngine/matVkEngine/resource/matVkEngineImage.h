#pragma once

#include "device/matVkEngineLogicalDevice.h"

namespace mat {

    class VkEngineImage {
    public:
        VkEngineImage();
        ~VkEngineImage();

        void setResolution(uint32_t w, uint32_t h, uint32_t d = 1);

        void setVkFormat(VkFormat format);

        void setVkImageUsageFlags(VkImageUsageFlags usage);

        void create(std::shared_ptr<VkEnginePhysicalDevice> physicalDevice,
                    std::shared_ptr<VkEngineLogicalDevice> logicalDevice);

        void getResolution(uint32_t& w, uint32_t& h, uint32_t& d);

        VkImage getVkImage() const;

        VkImageView getVkImageView() const;

        VkDeviceMemory getVkDeviceMemory() const;

        void release(std::shared_ptr<VkEngineLogicalDevice> logicalDevice);

    private:
        VkEngineImage(const VkEngineImage&) = delete;
        VkEngineImage(VkEngineImage&&) = delete;
        VkEngineImage& operator=(const VkEngineImage&) = delete;
        VkEngineImage& operator=(VkEngineImage&&) = delete;

        uint32_t width, height, depth;

        VkFormat _format;
        VkImageUsageFlags _usage;

        VkImage _image = VK_NULL_HANDLE;
        VkImageView _imageView = VK_NULL_HANDLE;
        VkDeviceMemory _memory = VK_NULL_HANDLE;
    };

};  // namespace mat
