#pragma once

#include <string>
#include <vector>
#include <volk/volk.h>

#include "Device.h"

enum class ShaderType
{
	Vertex,
	Fragment,
	Geometry,
	Compute
};

class ShaderModule
{
public:

	ShaderModule(const Device& logicalDevice, const std::string& spirvPath, ShaderType shaderType);

	ShaderModule(const ShaderModule&) = delete;
	ShaderModule& operator=(const ShaderModule&) = delete;
	ShaderModule(ShaderModule&& other) noexcept;
	ShaderModule& operator=(ShaderModule&& other) noexcept;
	~ShaderModule();

	VkShaderModule getModule() const;
	VkShaderStageFlagBits getStage() const;
	VkPipelineShaderStageCreateInfo getStageCreateInfo(const char* entryPoint = "main") const;

private:

	static std::vector<char> readSpirvFile(const std::string& spirvPath);
	static VkShaderStageFlagBits toVkShaderStage(ShaderType shaderType);
	void destroy();

	VkDevice _device;
	VkShaderModule _module;
	VkShaderStageFlagBits _stage;
};

