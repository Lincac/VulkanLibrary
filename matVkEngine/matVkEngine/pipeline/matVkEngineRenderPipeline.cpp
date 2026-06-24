#include "matVkEngineRenderPipeline.h"

#include <stdexcept>

namespace mat {

    VkEngineRenderPipeline::VkEngineRenderPipeline() {
        _info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    }

    VkEngineRenderPipeline::~VkEngineRenderPipeline() {}

    void VkEngineRenderPipeline::setShaderStages(std::vector<VkPipelineShaderStageCreateInfo> stages) {
        _info.stageCount = stages.size();
        _info.pStages = stages.data();
    }

    void VkEngineRenderPipeline::setVertexInputInfo(const VkPipelineVertexInputStateCreateInfo& info) {
        _info.pVertexInputState = &info;
    }

    void VkEngineRenderPipeline::setInputAssembly(const VkPipelineInputAssemblyStateCreateInfo& info) {
        _info.pInputAssemblyState = &info;
    }

    void VkEngineRenderPipeline::setViewportState(const VkPipelineViewportStateCreateInfo& info) {
        _info.pViewportState = &info;
    }

    void VkEngineRenderPipeline::setRasterizer(const VkPipelineRasterizationStateCreateInfo& info) {
        _info.pRasterizationState = &info;
    }

    void VkEngineRenderPipeline::setMultisampling(const VkPipelineMultisampleStateCreateInfo& info) {
        _info.pMultisampleState = &info;
    }

    void VkEngineRenderPipeline::setDepthStencil(const VkPipelineDepthStencilStateCreateInfo& info) {
        _info.pDepthStencilState = &info;
    }

    void VkEngineRenderPipeline::setColorBlending(const VkPipelineColorBlendStateCreateInfo& info) {
        _info.pColorBlendState = &info;
    }

    void VkEngineRenderPipeline::setDynamicState(const VkPipelineDynamicStateCreateInfo& info) {
        _info.pDynamicState = &info;
    }

    void VkEngineRenderPipeline::setPipelineLayout(const VkPipelineLayout& layout) {
        _info.layout = layout;
    }

    void VkEngineRenderPipeline::setRenderPass(VkRenderPass pass, uint32_t passId) {
        _info.renderPass = pass;
        _info.subpass = passId;
    }

    void VkEngineRenderPipeline::create(VkDevice device) {
        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &_info, nullptr, &_pipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }
    }

};  // namespace mat