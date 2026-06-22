#include "matVkEngineDescriptorSetLayout.h"

#include <stdexcept>

namespace mat {

    VkEngineDescriptorSetLayout::VkEngineDescriptorSetLayout() {}

    VkEngineDescriptorSetLayout::~VkEngineDescriptorSetLayout() {}

    void VkEngineDescriptorSetLayout::addBinding(uint32_t binding, DescriptorType type, uint32_t count,
                                                 ShaderType shaderFlags) {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = binding;
        uboLayoutBinding.descriptorCount = count;

        switch (type) {
            case mat::DescriptorType::None:
                throw std::runtime_error("Error descriptor type!");
                break;
            case mat::DescriptorType::UBO:
                uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                break;
            case mat::DescriptorType::Sampler2D:
                uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                break;
            case mat::DescriptorType::StorageBuffer:
                uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                break;
            default:
                throw std::runtime_error("Error descriptor!");
                break;
        }

        uboLayoutBinding.stageFlags = static_cast<VkShaderStageFlags>(shaderFlags);

        _bindings.push_back(uboLayoutBinding);
    }

    void VkEngineDescriptorSetLayout::create(std::shared_ptr<VkEngineLogicalDevice> logicalDevice) {
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(_bindings.size());
        layoutInfo.pBindings = _bindings.data();

        if (vkCreateDescriptorSetLayout(logicalDevice->getVkDevice(), &layoutInfo, nullptr, &_layout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    void VkEngineDescriptorSetLayout::release(std::shared_ptr<VkEngineLogicalDevice> logicalDevice) {
        if (_layout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(logicalDevice->getVkDevice(), _layout, nullptr);
            _layout = VK_NULL_HANDLE;
        }
    }

    VkDescriptorSetLayout VkEngineDescriptorSetLayout::getVkDescriptorSetLayout() const {
        return _layout;
    }

    std::vector<VkDescriptorSetLayoutBinding> VkEngineDescriptorSetLayout::getBindings() const {
        return _bindings;
    }

};  // namespace mat