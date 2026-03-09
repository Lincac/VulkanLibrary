#include "RenderPipeline.h"

#include <stdexcept>
#include <utility>

RenderPipeline::RenderPipeline()
	: _pipeline(VK_NULL_HANDLE)
	, _device(VK_NULL_HANDLE)
	, _pipelineCache(VK_NULL_HANDLE)
	, _useDepthStencilState(false)
	, _useDynamicState(false)
{
	_pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	_pipelineInfo.layout = VK_NULL_HANDLE;
	_pipelineInfo.renderPass = VK_NULL_HANDLE;
	_pipelineInfo.subpass = 0;
	_pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	_pipelineInfo.basePipelineIndex = -1;
	_pipelineInfo.flags = 0;

	_vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	_inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	_viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	_rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	_multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	_depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	_colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	_dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;

	// Set commonly expected defaults to avoid invalid unset fields.
	_inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	_inputAssemblyState.primitiveRestartEnable = VK_FALSE;
	_rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	_rasterizationState.cullMode = VK_CULL_MODE_NONE;
	_rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	_rasterizationState.lineWidth = 1.0f;
	_multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	_colorBlendState.logicOpEnable = VK_FALSE;

	refreshPipelineInfoPointers();
}

RenderPipeline::RenderPipeline(RenderPipeline&& other) noexcept
	: _pipelineInfo(other._pipelineInfo)
	, _pipeline(other._pipeline)
	, _device(other._device)
	, _pipelineCache(other._pipelineCache)
	, _shaderStages(std::move(other._shaderStages))
	, _vertexInputState(other._vertexInputState)
	, _inputAssemblyState(other._inputAssemblyState)
	, _viewportState(other._viewportState)
	, _rasterizationState(other._rasterizationState)
	, _multisampleState(other._multisampleState)
	, _depthStencilState(other._depthStencilState)
	, _colorBlendState(other._colorBlendState)
	, _dynamicState(other._dynamicState)
	, _useDepthStencilState(other._useDepthStencilState)
	, _useDynamicState(other._useDynamicState)
{
	other._pipeline = VK_NULL_HANDLE;
	other._device = VK_NULL_HANDLE;
	other._pipelineCache = VK_NULL_HANDLE;
	other._useDepthStencilState = false;
	other._useDynamicState = false;
	other.refreshPipelineInfoPointers();
	refreshPipelineInfoPointers();
}

RenderPipeline& RenderPipeline::operator=(RenderPipeline&& other) noexcept
{
	if (this != &other) {
		destroy();

		_pipelineInfo = other._pipelineInfo;
		_pipeline = other._pipeline;
		_device = other._device;
		_pipelineCache = other._pipelineCache;
		_shaderStages = std::move(other._shaderStages);
		_vertexInputState = other._vertexInputState;
		_inputAssemblyState = other._inputAssemblyState;
		_viewportState = other._viewportState;
		_rasterizationState = other._rasterizationState;
		_multisampleState = other._multisampleState;
		_depthStencilState = other._depthStencilState;
		_colorBlendState = other._colorBlendState;
		_dynamicState = other._dynamicState;
		_useDepthStencilState = other._useDepthStencilState;
		_useDynamicState = other._useDynamicState;

		other._pipeline = VK_NULL_HANDLE;
		other._device = VK_NULL_HANDLE;
		other._pipelineCache = VK_NULL_HANDLE;
		other._useDepthStencilState = false;
		other._useDynamicState = false;

		other.refreshPipelineInfoPointers();
		refreshPipelineInfoPointers();
	}
	return *this;
}

RenderPipeline::~RenderPipeline()
{
	destroy();
}

void RenderPipeline::setShaderStages(const std::vector<VkPipelineShaderStageCreateInfo>& stages)
{
	_shaderStages = stages;
	refreshPipelineInfoPointers();
}

void RenderPipeline::setVertexInputState(const VkPipelineVertexInputStateCreateInfo& state)
{
	_vertexInputState = state;
	_vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
}

void RenderPipeline::setInputAssemblyState(const VkPipelineInputAssemblyStateCreateInfo& state)
{
	_inputAssemblyState = state;
	_inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
}

void RenderPipeline::setViewportState(const VkPipelineViewportStateCreateInfo& state)
{
	_viewportState = state;
	_viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
}

void RenderPipeline::setRasterizationState(const VkPipelineRasterizationStateCreateInfo& state)
{
	_rasterizationState = state;
	_rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
}

void RenderPipeline::setMultisampleState(const VkPipelineMultisampleStateCreateInfo& state)
{
	_multisampleState = state;
	_multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
}

void RenderPipeline::setDepthStencilState(const VkPipelineDepthStencilStateCreateInfo& state)
{
	_depthStencilState = state;
	_depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	_useDepthStencilState = true;
	refreshPipelineInfoPointers();
}

void RenderPipeline::disableDepthStencilState()
{
	_useDepthStencilState = false;
	refreshPipelineInfoPointers();
}

void RenderPipeline::setColorBlendState(const VkPipelineColorBlendStateCreateInfo& state)
{
	_colorBlendState = state;
	_colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
}

void RenderPipeline::setDynamicState(const VkPipelineDynamicStateCreateInfo& state)
{
	_dynamicState = state;
	_dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	_useDynamicState = true;
	refreshPipelineInfoPointers();
}

void RenderPipeline::disableDynamicState()
{
	_useDynamicState = false;
	refreshPipelineInfoPointers();
}

void RenderPipeline::setLayout(VkPipelineLayout layout)
{
	_pipelineInfo.layout = layout;
}

void RenderPipeline::setRenderPass(VkRenderPass renderPass, uint32_t subpass)
{
	_pipelineInfo.renderPass = renderPass;
	_pipelineInfo.subpass = subpass;
}

void RenderPipeline::setBasePipeline(VkPipeline basePipelineHandle, int32_t basePipelineIndex)
{
	_pipelineInfo.basePipelineHandle = basePipelineHandle;
	_pipelineInfo.basePipelineIndex = basePipelineIndex;
}

void RenderPipeline::setPipelineFlags(VkPipelineCreateFlags flags)
{
	_pipelineInfo.flags = flags;
}

void RenderPipeline::setPipelineCache(VkPipelineCache cache)
{
	_pipelineCache = cache;
}

void RenderPipeline::create(const Device& logicalDevice)
{
	if (_shaderStages.empty()) {
		throw std::runtime_error("graphics pipeline requires at least one shader stage");
	}
	if (_pipelineInfo.layout == VK_NULL_HANDLE) {
		throw std::runtime_error("graphics pipeline layout is not set");
	}
	if (_pipelineInfo.renderPass == VK_NULL_HANDLE) {
		throw std::runtime_error("graphics pipeline render pass is not set");
	}

	destroy();
	_device = logicalDevice.getDevice();

	refreshPipelineInfoPointers();
	if (vkCreateGraphicsPipelines(_device, _pipelineCache, 1, &_pipelineInfo, nullptr, &_pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline");
	}
}

void RenderPipeline::destroy()
{
	if (_pipeline != VK_NULL_HANDLE && _device != VK_NULL_HANDLE) {
		vkDestroyPipeline(_device, _pipeline, nullptr);
		_pipeline = VK_NULL_HANDLE;
	}
}

VkPipeline RenderPipeline::getPipeline() const
{
	return _pipeline;
}

const VkGraphicsPipelineCreateInfo& RenderPipeline::getCreateInfo() const
{
	return _pipelineInfo;
}

void RenderPipeline::refreshPipelineInfoPointers()
{
	_pipelineInfo.stageCount = static_cast<uint32_t>(_shaderStages.size());
	_pipelineInfo.pStages = _shaderStages.empty() ? nullptr : _shaderStages.data();

	_pipelineInfo.pVertexInputState = &_vertexInputState;
	_pipelineInfo.pInputAssemblyState = &_inputAssemblyState;
	_pipelineInfo.pViewportState = &_viewportState;
	_pipelineInfo.pRasterizationState = &_rasterizationState;
	_pipelineInfo.pMultisampleState = &_multisampleState;
	_pipelineInfo.pColorBlendState = &_colorBlendState;
	_pipelineInfo.pDepthStencilState = _useDepthStencilState ? &_depthStencilState : nullptr;
	_pipelineInfo.pDynamicState = _useDynamicState ? &_dynamicState : nullptr;
}
