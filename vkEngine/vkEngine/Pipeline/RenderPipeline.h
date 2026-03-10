#pragma once

#include <cstdint>
#include <volk/volk.h>
#include <vector>

#include "Device.h"
#include "ShaderModule.h"

class RenderPipeline
{
public:

	RenderPipeline();

	RenderPipeline(const RenderPipeline&) = delete;
	RenderPipeline& operator=(const RenderPipeline&) = delete;
	RenderPipeline(RenderPipeline&& other) noexcept;
	RenderPipeline& operator=(RenderPipeline&& other) noexcept;
	~RenderPipeline();

	void addShader(ShaderModule& shader);

	void setVertexInputState(const VkPipelineVertexInputStateCreateInfo& state);
	void setInputAssemblyState(const VkPipelineInputAssemblyStateCreateInfo& state);
	void setViewportState(const VkPipelineViewportStateCreateInfo& state);
	void setRasterizationState(const VkPipelineRasterizationStateCreateInfo& state);
	void setMultisampleState(const VkPipelineMultisampleStateCreateInfo& state);
	void setDepthStencilState(const VkPipelineDepthStencilStateCreateInfo& state);
	void disableDepthStencilState();
	void setColorBlendState(const VkPipelineColorBlendStateCreateInfo& state);
	void setDynamicState(const VkPipelineDynamicStateCreateInfo& state);
	void disableDynamicState();

	void setLayout(VkPipelineLayout layout);
	void setRenderPass(VkRenderPass renderPass, uint32_t subpass = 0);
	void setBasePipeline(VkPipeline basePipelineHandle, int32_t basePipelineIndex = -1);
	void setPipelineFlags(VkPipelineCreateFlags flags);
	void setPipelineCache(VkPipelineCache cache);

	void create(const Device& logicalDevice);
	void destroy();

	VkPipeline getPipeline() const;
	const VkGraphicsPipelineCreateInfo& getCreateInfo() const;

private:

	void refreshPipelineInfoPointers();

	VkGraphicsPipelineCreateInfo _pipelineInfo;
	VkPipeline _pipeline;
	VkPipelineLayout _pipelineLayout;
	VkDevice _device;
	VkPipelineCache _pipelineCache;

	std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;

	VkPipelineVertexInputStateCreateInfo _vertexInputState;
	VkPipelineInputAssemblyStateCreateInfo _inputAssemblyState;
	VkPipelineViewportStateCreateInfo _viewportState;
	VkViewport _viewport;
	VkRect2D _scissor;
	VkPipelineRasterizationStateCreateInfo _rasterizationState;
	VkPipelineMultisampleStateCreateInfo _multisampleState;
	VkPipelineDepthStencilStateCreateInfo _depthStencilState;
	VkPipelineColorBlendAttachmentState _colorBlendAttachment;
	VkPipelineColorBlendStateCreateInfo _colorBlendState;
	std::vector<VkDynamicState> _dynamicStates;
	VkPipelineDynamicStateCreateInfo _dynamicState;

	bool _useDepthStencilState;
	bool _useDynamicState;

};

