#pragma once

#include "vkEngineLogicalDevice.h"
#include "vkEngineCommandPool.h"

/// @brief Sampled texture (HDR environment maps, etc.)
class vkEngineTexture
{
public:
    vkEngineTexture(std::shared_ptr<vkEngineLogicalDevice> device);
    ~vkEngineTexture();

    /// @brief 从 Radiance HDR 加载并上传到 GPU（R32G32B32A32_SFLOAT）
    /// @param commandPool 用于一次性上传命令
    /// @param relativePath 相对 exe 的路径
    void loadHdr(std::shared_ptr<vkEngineCommandPool> commandPool, const std::string& relativePath);

    /// @brief 获取图像视图
    /// @return 图像视图    
    VkImageView& getImageView();

    /// @brief 获取采样器
    /// @return 采样器
    VkSampler& getSampler();

    /// @brief 获取分辨率
    /// @return 分辨率
    glm::ivec2 getResolution() const;

private:
    std::shared_ptr<vkEngineLogicalDevice> _device; // 逻辑设备

    glm::ivec2 _resolution{}; // 分辨率

    VkImage _image = VK_NULL_HANDLE; // 图像
    VkDeviceMemory _memory = VK_NULL_HANDLE; // 设备内存    
    VkImageView _imageView = VK_NULL_HANDLE; // 图像视图
    VkSampler _sampler = VK_NULL_HANDLE; // 采样器
};
