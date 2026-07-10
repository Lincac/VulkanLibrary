#pragma once

#include "matVkEngineContext.h"

namespace mat {

    enum class ImageType {
        LDR2D,
        HDR2D,
        Volume3D,
    };

    class VkEngineImage {
    public:
        VkEngineImage();
        ~VkEngineImage();

        void load(const std::string& path);

        void load(const std::string& path, uint32_t width, uint32_t height, uint32_t depth);

        void setResolution(uint32_t w, uint32_t h, uint32_t d = 1);

        void setFormat(VkFormat format);

        void setImageUsageFlags(VkImageUsageFlags usage);

        void create(const VkEngineContext& context);

        void getResolution(uint32_t& w, uint32_t& h, uint32_t& d) const;

        VkFormat getFormat() const;

        ImageType getImageType() const;

        VkImage getImage() const;

        VkImageView getImageView() const;

        VkDeviceMemory getDeviceMemory() const;

        void release(VkDevice logDevice);

    private:
        VkEngineImage(const VkEngineImage&) = delete;
        VkEngineImage(VkEngineImage&&) = delete;
        VkEngineImage& operator=(const VkEngineImage&) = delete;
        VkEngineImage& operator=(VkEngineImage&&) = delete;

        void adoptPixelData(ImageType kind, int width, int height, int depth, VkFormat format, const void* data,
                            VkDeviceSize size);

        uint32_t width, height, depth;

        VkFormat _format;
        VkImageUsageFlags _usage;
        ImageType _imageType = ImageType::LDR2D;
        std::vector<uint8_t> _pixelData;
        VkDeviceSize _pixelSize = 0;

        VkImage _image = VK_NULL_HANDLE;
        VkImageView _imageView = VK_NULL_HANDLE;
        VkDeviceMemory _memory = VK_NULL_HANDLE;
    };

};  // namespace mat
