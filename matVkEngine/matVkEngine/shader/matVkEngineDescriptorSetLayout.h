#pragma once

#include "shader/matVkEngineShaderModule.h"

namespace mat {

    enum class DescriptorType {
        None,
        UBO,
        Sampler2D,
        StorageBuffer
    };

    class VkEngineDescriptorSetLayout {
    public:
        VkEngineDescriptorSetLayout();
        ~VkEngineDescriptorSetLayout();

        void addBinding(uint32_t binding, DescriptorType type, uint32_t count, ShaderType shaderFlags);

        void create(std::shared_ptr<VkEngineLogicalDevice> logicalDevice);

        void release(std::shared_ptr<VkEngineLogicalDevice> logicalDevice);

        VkDescriptorSetLayout getVkDescriptorSetLayout() const;

        std::vector<VkDescriptorSetLayoutBinding> getBindings() const;

    private:
        VkEngineDescriptorSetLayout(const VkEngineDescriptorSetLayout&) = delete;
        VkEngineDescriptorSetLayout(VkEngineDescriptorSetLayout&&) = delete;
        VkEngineDescriptorSetLayout& operator=(const VkEngineDescriptorSetLayout&) = delete;
        VkEngineDescriptorSetLayout& operator=(VkEngineDescriptorSetLayout&&) = delete;

        VkDescriptorSetLayout _layout = VK_NULL_HANDLE;
        std::vector<VkDescriptorSetLayoutBinding> _bindings;
    };

};  // namespace mat