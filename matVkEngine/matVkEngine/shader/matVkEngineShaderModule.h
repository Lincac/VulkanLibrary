#pragma once

#include "device/matVkEngineLogicalDevice.h"

namespace mat {

    enum class ShaderType : VkShaderStageFlags {
        None = 0,
        Vertex = VK_SHADER_STAGE_VERTEX_BIT,
        Fragment = VK_SHADER_STAGE_FRAGMENT_BIT,
        Compute = VK_SHADER_STAGE_COMPUTE_BIT,
    };

    inline constexpr ShaderType operator|(ShaderType lhs, ShaderType rhs) noexcept {
        return static_cast<ShaderType>(static_cast<VkShaderStageFlags>(lhs) | static_cast<VkShaderStageFlags>(rhs));
    }

    inline constexpr ShaderType& operator|=(ShaderType& lhs, ShaderType rhs) noexcept {
        lhs = lhs | rhs;
        return lhs;
    }

    class VkEngineShaderModule {
    public:
        VkEngineShaderModule();
        ~VkEngineShaderModule();

        void setShaderType(ShaderType type);

        void load(const std::string& path);

        void create(std::shared_ptr<VkEngineLogicalDevice> logicalDevice);

        void release(std::shared_ptr<VkEngineLogicalDevice> logicalDevice);

        VkShaderModule getVkShaderModule() const;

        const VkPipelineShaderStageCreateInfo& getShaderStageCreateInfo() const;

    private:
        VkEngineShaderModule(const VkEngineShaderModule&) = delete;
        VkEngineShaderModule(VkEngineShaderModule&&) = delete;
        VkEngineShaderModule& operator=(const VkEngineShaderModule&) = delete;
        VkEngineShaderModule& operator=(VkEngineShaderModule&&) = delete;

        ShaderType _type;
        std::vector<char> _buffer;

        VkShaderModule _shaderModule = VK_NULL_HANDLE;
        VkPipelineShaderStageCreateInfo _shaderStageinfo;
    };

};  // namespace mat