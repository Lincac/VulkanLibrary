#include "vkEngine.h"

#include <fstream>

VkShaderModule vkEngine::createShader(const std::string& path, ShaderType type)
{
	(void)type;
	VkShaderModule shader = VK_NULL_HANDLE;

	const std::vector<char> code = readSpirvFile(path);
	if (code.empty()) {
		throw std::runtime_error("SPIR-V file is empty: " + path);
	}

	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	if (vkCreateShaderModule(_logicalDevice, &createInfo, nullptr, &shader) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module: " + path);
	}

	return shader;
}

std::vector<char> vkEngine::readSpirvFile(const std::string& spirvPath)
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

VkShaderStageFlagBits vkEngine::toVkShaderStage(ShaderType shaderType)
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