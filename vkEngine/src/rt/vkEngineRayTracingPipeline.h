#pragma once

#include "resource/vkEngineBuffer.h"
#include "device/vkEngineLogicalDevice.h"

/// @brief 光线追踪管线类
class vkEngineRayTracingPipeline
{
public:
    /// @brief 构造函数
    /// @param device 逻辑设备
    vkEngineRayTracingPipeline(std::shared_ptr<vkEngineLogicalDevice> device);
    ~vkEngineRayTracingPipeline();

    /// @brief 设置着色器 SPIR-V 路径
    /// @param raygen 光线生成着色器
    /// @param miss 未命中着色器
    /// @param closestHit 最近命中着色器
    void setShaders(const std::string& raygen, const std::string& miss, const std::string& closestHit);

    /// @brief 设置描述符集布局
    /// @param layout 描述符集布局
    void setDescriptorSetLayout(VkDescriptorSetLayout layout);

    /// @brief 创建 RT 管线
    void create();

    /// @brief 创建 SBT buffer 与 raygen / miss / hit region
    void createSBT();

    /// @brief 录制 trace rays 命令
    /// @param cmd 命令缓冲区
    /// @param descriptorSet 描述符集
    /// @param width 宽度
    /// @param height 高度
    /// @param depth 深度
    void recordTrace(VkCommandBuffer cmd, VkDescriptorSet descriptorSet,
        uint32_t width, uint32_t height, uint32_t depth = 1);

    /// @brief 获取管线
    /// @return 管线
    VkPipeline& getPipeline();

    /// @brief 获取管线布局
    /// @return 管线布局
    VkPipelineLayout& getPipelineLayout();

    /// @brief 获取 raygen region
    /// @return raygen region
    const VkStridedDeviceAddressRegionKHR& getRaygenRegion() const;

    /// @brief 获取 miss region
    /// @return miss region
    const VkStridedDeviceAddressRegionKHR& getMissRegion() const;

    /// @brief 获取 hit region
    /// @return hit region
    const VkStridedDeviceAddressRegionKHR& getHitRegion() const;

    /// @brief 获取 callable region
    /// @return callable region
    const VkStridedDeviceAddressRegionKHR& getCallableRegion() const;

private:
    VkShaderModule createShaderModule(const std::vector<char>& code);

private:
    std::shared_ptr<vkEngineLogicalDevice> _device; // 逻辑设备

    std::string _raygenPath; // 光线生成着色器路径
    std::string _missPath; // 未命中着色器路径
    std::string _closestHitPath; // 最近命中着色器路径
    VkDescriptorSetLayout _descriptorSetLayout = VK_NULL_HANDLE; // 描述符集布局

    std::vector<VkShaderModule> _shaderModules; // 着色器模块   vector<shaderModule>

    VkPipeline _pipeline = VK_NULL_HANDLE; // 管线
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE; // 管线布局

    /* SBT 
     *  Shader Binding Table（着色器绑定表）
     * 一个 GPU Buffer，里面按固定格式存放 RayGen / Miss / HitGroup 等 shader 的句柄和参数，用来驱动光线追踪管线
     * 
     * RayGen → 发射光线
     *  ↓
     * TLAS → 找到击中的 instance
     *  ↓
     * SBT → 找到对应的 hit shader
     *  ↓
     * 执行 closest-hit / any-hit / intersection
     */
    std::vector<std::shared_ptr<vkEngineBuffer>> _sbtBuffers; // 各 shader group 独立 SBT buffer

    VkStridedDeviceAddressRegionKHR _raygenRegion{}; // raygen region
    VkStridedDeviceAddressRegionKHR _missRegion{}; // miss region
    VkStridedDeviceAddressRegionKHR _hitRegion{}; // hit region
    VkStridedDeviceAddressRegionKHR _callableRegion{}; // callable region

    uint32_t _shaderGroupHandleSize = 0; // shader group handle size
    uint32_t _shaderGroupHandleAlignment = 0; // shader group handle alignment  
    static constexpr uint32_t _shaderGroupCount = 3; // shader group count
};
