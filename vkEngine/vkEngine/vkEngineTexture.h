#pragma once

#include "vkEngineLogicalDevice.h"
#include "vkEngineCommandPool.h"

/// @brief Sampled texture (HDR environment maps, etc.)
class vkEngineTexture
{
public:
    vkEngineTexture(std::shared_ptr<vkEngineLogicalDevice> device);
    ~vkEngineTexture();

    /// @brief 从 Radiance HDR 加载并上传到 GPU（R32G32B32A32_SFLOAT），同时烘焙环境光 CDF
    /// @param commandPool 用于一次性上传命令
    /// @param relativePath 相对 exe 的路径
    void loadHdr(std::shared_ptr<vkEngineCommandPool> commandPool, const std::string& relativePath);

    /// @brief 获取图像视图
    VkImageView& getImageView();

    /// @brief 获取采样器
    VkSampler& getSampler();

    /// @brief 获取分辨率
    glm::ivec2 getResolution() const;

    /// @brief 环境光 CDF 纹理（R=行内 conditional CDF，G=行 marginal CDF）
    VkImageView& getEnvCdfImageView();

    /// @brief 环境光 CDF 采样器
    VkSampler& getEnvCdfSampler();

    /// @brief 环境光 CDF 分辨率
    glm::ivec2 getEnvCdfResolution() const;

private:
    /// @brief 构建环境光 CDF
    /// @param commandPool 用于一次性上传命令
    /// @param pixels 像素数据
    /// @param srcWidth 源宽度
    /// @param srcHeight 源高度
    void buildEnvironmentCdf(std::shared_ptr<vkEngineCommandPool> commandPool,
        const float* pixels, int srcWidth, int srcHeight);

    /// @brief 双线性采样 luminance
    /// @param pixels 像素数据
    /// @param width 宽度
    /// @param height 高度
    /// @param u u 坐标
    /// @param v v 坐标
    /// @return luminance
    static float sampleBilinearLuminance(const float* pixels, int width, int height, float u, float v);

private:
    std::shared_ptr<vkEngineLogicalDevice> _device;

    glm::ivec2 _resolution{};
    glm::ivec2 _envCdfResolution{ 512, 256 };

    VkImage _image = VK_NULL_HANDLE;
    VkDeviceMemory _memory = VK_NULL_HANDLE;
    VkImageView _imageView = VK_NULL_HANDLE;
    VkSampler _sampler = VK_NULL_HANDLE;

    VkImage _envCdfImage = VK_NULL_HANDLE;
    VkDeviceMemory _envCdfMemory = VK_NULL_HANDLE;
    VkImageView _envCdfImageView = VK_NULL_HANDLE;
    VkSampler _envCdfSampler = VK_NULL_HANDLE;
};
