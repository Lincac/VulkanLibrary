#pragma once

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

	ShaderModule();

public:

	void setDependice(Device* logicalDevice);

	void setShaderType(ShaderType type);

	void setFilePath(const std::string& path);

	int create();

	VkShaderModule getModule() const;

	VkShaderStageFlagBits getStage() const;

	VkPipelineShaderStageCreateInfo getStageCreateInfo(const char* entryPoint = "main") const;

private:

	static std::vector<char> readSpirvFile(const std::string& spirvPath);

	static VkShaderStageFlagBits toVkShaderStage(ShaderType shaderType);

private:

	Device* _logicalDevice;

private:

	std::string _path;

	VkShaderModule _module;
	VkShaderStageFlagBits _stage;
};

