#pragma once

#include "vkEngineLogicalDevice.h"
#include "vkEngineCommandPool.h"

/// @brief 图像类
class vkEngineImage {
public:
    /// @brief 构造函数
    /// @param device 逻辑设备
    /// @param resolution 分辨率
    vkEngineImage(std::shared_ptr<vkEngineLogicalDevice> device, glm::ivec2 resolution = glm::ivec2(1));
    ~vkEngineImage(); 

    /// @brief 设置分辨率
    /// @param resolution 分辨率
    void setResolution(glm::ivec2 resolution);

    /// @brief 设置格式
    /// @param format 格式
    void setFormat(VkFormat format);

    /// @brief 设置图像使用标志
    /// @param usage 使用标志
    void setImageUsageFlags(VkImageUsageFlags usage);

    /// @brief 生成图像
    void generate();

    /// @brief 获取图像
    /// @return 图像
    VkImage& getImage();

    /// @brief 获取图像视图
    /// @return 图像视图
    VkImageView& getImageView();

    /// @brief 获取设备内存
    /// @return 设备内存
    VkDeviceMemory& getMemory();

    /// @brief 获取分辨率
    /// @return 分辨率
    glm::ivec2 getResolution() const;

    /// @brief 读回 GPU 图像并保存为 PNG
    /// @param commandPool 命令池
    /// @param filepath 输出路径
    /// @param beforeCopy 复制前在同一 command buffer 中执行的回调（如 trace rays）
    void saveToPng(std::shared_ptr<vkEngineCommandPool> commandPool, const std::string& filepath,
        std::function<void(VkCommandBuffer)> beforeCopy = nullptr);

private:
    std::shared_ptr<vkEngineLogicalDevice> _device; // 逻辑设备

    glm::ivec2 _resolution; // 分辨率
    VkFormat _format; // 格式
    VkImageUsageFlags _usage; // 图像使用标志

    VkImage _image = VK_NULL_HANDLE; // 图像
    VkDeviceMemory _memory = VK_NULL_HANDLE; // 设备内存
    VkImageView _imageView = VK_NULL_HANDLE; // 图像视图
};