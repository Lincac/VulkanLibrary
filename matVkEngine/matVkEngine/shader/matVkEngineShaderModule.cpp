#include "matVkEngineShaderModule.h"

#include <fstream>
#include <stdexcept>

namespace mat {

    VkEngineShaderModule::VkEngineShaderModule() {
        _type = ShaderType::None;
        _shaderStageinfo = {};
    }

    VkEngineShaderModule::~VkEngineShaderModule() {}

    void VkEngineShaderModule::setShaderType(ShaderType type) {
        _type = type;
    }

    void VkEngineShaderModule::load(const std::string& path) {
        std::ifstream file(path, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t)file.tellg();
        _buffer = std::vector<char>(fileSize);

        file.seekg(0);
        file.read(_buffer.data(), fileSize);

        file.close();
    }

    void VkEngineShaderModule::create(std::shared_ptr<VkEngineLogicalDevice> logicalDevice) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = _buffer.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(_buffer.data());

        if (vkCreateShaderModule(logicalDevice->getVkDevice(), &createInfo, nullptr, &_shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }

        _shaderStageinfo = {};
        _shaderStageinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;

        switch (_type) {
            case mat::ShaderType::None:
                throw std::runtime_error("Error shader type!");
            case mat::ShaderType::Vertex:
                _shaderStageinfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
                break;
            case mat::ShaderType::Fragment:
                _shaderStageinfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
            case mat::ShaderType::Compute:
                _shaderStageinfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
                break;
            default:
                break;
        }

        _shaderStageinfo.module = _shaderModule;
        _shaderStageinfo.pName = "main";
    }

    void VkEngineShaderModule::release(std::shared_ptr<VkEngineLogicalDevice> logicalDevice) {
        if (_shaderModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(logicalDevice->getVkDevice(), _shaderModule, nullptr);
            _shaderModule = VK_NULL_HANDLE;
            _shaderStageinfo = {};
        }
    }

    VkShaderModule VkEngineShaderModule::getVkShaderModule() const {
        return _shaderModule;
    }

    const VkPipelineShaderStageCreateInfo& VkEngineShaderModule::getShaderStageCreateInfo() const {
        return _shaderStageinfo;
    }

}  // namespace mat
