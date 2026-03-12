#include "ShaderModule.h"

#include <fstream>
#include <stdexcept>

ShaderModule::ShaderModule()
{
	_logicalDevice = nullptr;
	_path = "";

	_module = VK_NULL_HANDLE;
}

void ShaderModule::setDependice(Device* logicalDevice)
{
	if (logicalDevice == nullptr)
	{
		return;
	}

	_logicalDevice = logicalDevice;
}

void ShaderModule::setShaderType(ShaderType type)
{
	_stage = toVkShaderStage(type);
}

void ShaderModule::setFilePath(const std::string& path)
{
	_path = path;
}

int ShaderModule::create()
{
	if (_logicalDevice == nullptr || _path == "")
	{
		return -1;
	}

	const std::vector<char> code = readSpirvFile(_path);
	if (code.empty()) {
		throw std::runtime_error("SPIR-V file is empty: " + _path);
	}

	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	if (vkCreateShaderModule(_logicalDevice->getDevice(), &createInfo, nullptr, &_module) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module: " + _path);
	}

	return 0;
}

VkShaderModule ShaderModule::getModule() const
{
	return _module;
}

VkShaderStageFlagBits ShaderModule::getStage() const
{
	return _stage;
}

VkPipelineShaderStageCreateInfo ShaderModule::getStageCreateInfo(const char* entryPoint) const
{
	VkPipelineShaderStageCreateInfo stageInfo{};
	stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageInfo.stage = _stage;
	stageInfo.module = _module;
	stageInfo.pName = entryPoint;
	return stageInfo;
}

std::vector<char> ShaderModule::readSpirvFile(const std::string& spirvPath)
{
	std::ifstream file(spirvPath, std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("failed to open SPIR-V file: " + spirvPath);
	}

	const std::streamsize fileSize = file.tellg();
	if (fileSize < 0) {
		throw std::runtime_error("failed to query SPIR-V file size: " + spirvPath);
	}

	std::vector<char> buffer(static_cast<size_t>(fileSize));
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	if (!file.good() && !file.eof()) {
		throw std::runtime_error("failed to read SPIR-V file: " + spirvPath);
	}

	return buffer;
}

VkShaderStageFlagBits ShaderModule::toVkShaderStage(ShaderType shaderType)
{
	switch (shaderType) {
	case ShaderType::Vertex:
		return VK_SHADER_STAGE_VERTEX_BIT;
	case ShaderType::Fragment:
		return VK_SHADER_STAGE_FRAGMENT_BIT;
	case ShaderType::Geometry:
		return VK_SHADER_STAGE_GEOMETRY_BIT;
	case ShaderType::Compute:
		return VK_SHADER_STAGE_COMPUTE_BIT;
	default:
		throw std::runtime_error("unsupported shader type");
	}
}