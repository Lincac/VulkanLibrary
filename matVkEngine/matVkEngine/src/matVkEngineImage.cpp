#include "matVkEngineImage.h"

#include <cstdio>
#include <cstring>

#define STB_IMAGE_IMPLEMENTATION
#include "func/stb_image.h"

namespace mat {

    static std::vector<unsigned char> readFileToMemory(const std::string& path) {
        FILE* file = std::fopen(path.c_str(), "rb");
        if (file == nullptr) {
            VK_ERROR("failed to open image file: " + path);
        }

        if (std::fseek(file, 0, SEEK_END) != 0) {
            std::fclose(file);
            VK_ERROR("failed to seek image file: " + path);
        }

        const long fileSize = std::ftell(file);
        if (fileSize <= 0) {
            std::fclose(file);
            VK_ERROR("image file is empty: " + path);
        }

        std::vector<unsigned char> buffer(static_cast<size_t>(fileSize));
        if (std::fseek(file, 0, SEEK_SET) != 0) {
            std::fclose(file);
            VK_ERROR("failed to seek image file: " + path);
        }

        const size_t readSize = std::fread(buffer.data(), 1, buffer.size(), file);
        std::fclose(file);
        if (readSize != buffer.size()) {
            VK_ERROR("failed to read image file: " + path);
        }

        return buffer;
    }

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

    void VkEngineImage::load(const std::string& path) {
        _pixelData.clear();
        _pixelSize = 0;

        int w = 0;
        int h = 0;
        int d = 1;
        int channels = 0;

        if (stbi_is_hdr(path.c_str())) {
            float* pixels = stbi_loadf(path.c_str(), &w, &h, &channels, STBI_rgb_alpha);
            if (pixels == nullptr || w <= 0 || h <= 0) {
                VK_ERROR("failed to load HDR image: " + path);
            }

            const VkDeviceSize pixelBytes = static_cast<VkDeviceSize>(w) * h * 4 * sizeof(float);
            adoptPixelData(ImageType::HDR2D, w, h, 1, VK_FORMAT_R32G32B32A32_SFLOAT, pixels, pixelBytes);
            stbi_image_free(pixels);
            return;
        }

        const std::vector<unsigned char> fileData = readFileToMemory(path);
        const int fileSize = static_cast<int>(fileData.size());

        int* delays = nullptr;
        stbi_uc* volumePixels =
            stbi_load_gif_from_memory(fileData.data(), fileSize, &delays, &w, &h, &d, &channels, STBI_rgb_alpha);
        if (delays != nullptr) {
            STBI_FREE(delays);
        }

        if (volumePixels != nullptr && d > 1) {
            const VkDeviceSize pixelBytes = static_cast<VkDeviceSize>(w) * h * d * 4;
            adoptPixelData(ImageType::Volume3D, w, h, d, VK_FORMAT_R8G8B8A8_UNORM, volumePixels, pixelBytes);
            stbi_image_free(volumePixels);
            return;
        }

        if (volumePixels != nullptr) {
            stbi_image_free(volumePixels);
        }

        stbi_uc* pixels = stbi_load_from_memory(fileData.data(), fileSize, &w, &h, &channels, STBI_rgb_alpha);
        if (pixels == nullptr || w <= 0 || h <= 0) {
            VK_ERROR("failed to load 2D image: " + path);
        }

        const VkDeviceSize pixelBytes = static_cast<VkDeviceSize>(w) * h * 4;
        adoptPixelData(ImageType::LDR2D, w, h, 1, VK_FORMAT_R8G8B8A8_UNORM, pixels, pixelBytes);
        stbi_image_free(pixels);
    }

    void VkEngineImage::load(const std::string& path, uint32_t w, uint32_t h, uint32_t d) {
        if (w == 0 || h == 0 || d == 0) {
            VK_ERROR("invalid volume dimension!");
        }

        _pixelData.clear();
        _pixelSize = 0;

        const std::vector<unsigned char> fileData = readFileToMemory(path);
        const VkDeviceSize expectedSize = static_cast<VkDeviceSize>(w) * h * d * 4;
        if (fileData.size() != expectedSize) {
            VK_ERROR("volume file size mismatch: " + path);
        }

        adoptPixelData(ImageType::Volume3D, static_cast<int>(w), static_cast<int>(h), static_cast<int>(d),
                       VK_FORMAT_R8G8B8A8_UNORM, fileData.data(), expectedSize);
    }

    void VkEngineImage::create(VkPhysicalDevice device, VkDevice logDevice) {
        if (_imageView != VK_NULL_HANDLE) {
            return;
        }

        const VkImageType imageType = depth > 1 ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
        const VkImageViewType viewType = depth > 1 ? VK_IMAGE_VIEW_TYPE_3D : VK_IMAGE_VIEW_TYPE_2D;

        VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        imageInfo.imageType = imageType;
        imageInfo.extent = {width, height, depth};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = _format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = _usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VK_CHECK(vkCreateImage(logDevice, &imageInfo, nullptr, &_image));

        VkMemoryRequirements memReq{};
        vkGetImageMemoryRequirements(logDevice, _image, &memReq);

        VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex = findMemoryType(device, memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VK_CHECK(vkAllocateMemory(logDevice, &allocInfo, nullptr, &_memory));

        vkBindImageMemory(logDevice, _image, _memory, 0);

        VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewInfo.image = _image;
        viewInfo.viewType = viewType;
        viewInfo.format = _format;
        viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

        VK_CHECK(vkCreateImageView(logDevice, &viewInfo, nullptr, &_imageView));
    }

    void VkEngineImage::getResolution(uint32_t& w, uint32_t& h, uint32_t& d) const {
        w = width;
        h = height;
        d = depth;
    }

    VkFormat VkEngineImage::getVkFormat() const {
        return _format;
    }

    ImageType VkEngineImage::getImageType() const {
        return _imageType;
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

    void VkEngineImage::release(VkDevice logDevice) {
        if (logDevice == nullptr) {
            VK_ERROR("Logical Device is nullptr!");
        }

        _pixelData.clear();
        _pixelSize = 0;

        width = 1;
        height = 1;
        depth = 1;

        _format = VK_FORMAT_R8G8B8A8_UNORM;
        _usage = VK_IMAGE_USAGE_STORAGE_BIT;
        _imageType = ImageType::LDR2D;

        if (_imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(logDevice, _imageView, nullptr);
            _imageView = VK_NULL_HANDLE;
        }

        if (_image != VK_NULL_HANDLE) {
            vkDestroyImage(logDevice, _image, nullptr);
            _image = VK_NULL_HANDLE;
        }

        if (_memory != VK_NULL_HANDLE) {
            vkFreeMemory(logDevice, _memory, nullptr);
            _memory = VK_NULL_HANDLE;
        }
    }

    void VkEngineImage::adoptPixelData(ImageType kind, int w, int h, int d, VkFormat format, const void* data,
                                       VkDeviceSize size) {
        _imageType = kind;
        _pixelSize = size;
        _pixelData.resize(static_cast<size_t>(size));
        std::memcpy(_pixelData.data(), data, static_cast<size_t>(size));

        width = static_cast<uint32_t>(w);
        height = static_cast<uint32_t>(h);
        depth = static_cast<uint32_t>(d);
        _format = format;
        _usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    }

};  // namespace mat
