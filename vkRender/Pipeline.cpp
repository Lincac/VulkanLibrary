#include "Pipeline.h"
#include "VertexBuffer.h"

#include <stdexcept>

void Pipeline::ensureDevice(VkDevice inDevice) {
    if (inDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("Pipeline: device is null");
    }

    if (device == VK_NULL_HANDLE) {
        device = inDevice;
        return;
    }

    if (device != inDevice) {
        throw std::runtime_error("Pipeline: device mismatch");
    }
}

VkPipelineLayout Pipeline::createPipelineLayout(
    const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
    const std::vector<VkPushConstantRange>& pushConstantRanges) const {
    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    layoutInfo.pSetLayouts = descriptorSetLayouts.empty() ? nullptr : descriptorSetLayouts.data();
    layoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
    layoutInfo.pPushConstantRanges = pushConstantRanges.empty() ? nullptr : pushConstantRanges.data();

    VkPipelineLayout layout = VK_NULL_HANDLE;
    if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &layout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout");
    }
    return layout;
}

void Pipeline::initializeGraphics(const GraphicsCreateInfo& info) {
    // Clean old graphics pipeline first, then bind/validate device for new creation.
    cleanupGraphics();
    ensureDevice(info.device);

    if (info.renderPass == VK_NULL_HANDLE) {
        throw std::runtime_error("Pipeline::initializeGraphics: render pass is null");
    }
    if (info.vertShaderModule == VK_NULL_HANDLE || info.fragShaderModule == VK_NULL_HANDLE) {
        throw std::runtime_error("Pipeline::initializeGraphics: shader modules are null");
    }
    if (info.extent.width == 0 || info.extent.height == 0) {
        throw std::runtime_error("Pipeline::initializeGraphics: invalid extent");
    }

    graphicsPipelineLayout = createPipelineLayout(
        info.descriptorSetLayouts,
        info.pushConstantRanges);

    VkPipelineShaderStageCreateInfo shaderStages[2]{};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = info.vertShaderModule;
    shaderStages[0].pName = "main";
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = info.fragShaderModule;
    shaderStages[1].pName = "main";

    const VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
    const auto attributeDescriptions = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &bindingDescription;
    vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInput.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = info.topology;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(info.extent.width);
    viewport.height = static_cast<float>(info.extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = info.extent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = info.polygonMode;
    rasterizer.cullMode = info.cullMode;
    rasterizer.frontFace = info.frontFace;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = info.enableDepthTest ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = info.enableDepthTest ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp = info.depthCompareOp;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = info.enableDepthTest ? &depthStencil : nullptr;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr;
    pipelineInfo.layout = graphicsPipelineLayout;
    pipelineInfo.renderPass = info.renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(
            device,
            VK_NULL_HANDLE,
            1,
            &pipelineInfo,
            nullptr,
            &graphicsPipeline) != VK_SUCCESS) {
        vkDestroyPipelineLayout(device, graphicsPipelineLayout, nullptr);
        graphicsPipelineLayout = VK_NULL_HANDLE;
        throw std::runtime_error("Failed to create graphics pipeline");
    }
}

void Pipeline::initializeCompute(const ComputeCreateInfo& info) {
    // Clean old compute pipeline first, then bind/validate device for new creation.
    cleanupCompute();
    ensureDevice(info.device);

    if (info.computeShaderModule == VK_NULL_HANDLE) {
        throw std::runtime_error("Pipeline::initializeCompute: shader module is null");
    }

    computePipelineLayout = createPipelineLayout(
        info.descriptorSetLayouts,
        info.pushConstantRanges);

    VkPipelineShaderStageCreateInfo stage{};
    stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stage.module = info.computeShaderModule;
    stage.pName = "main";

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.stage = stage;
    pipelineInfo.layout = computePipelineLayout;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    if (vkCreateComputePipelines(
            device,
            VK_NULL_HANDLE,
            1,
            &pipelineInfo,
            nullptr,
            &computePipeline) != VK_SUCCESS) {
        vkDestroyPipelineLayout(device, computePipelineLayout, nullptr);
        computePipelineLayout = VK_NULL_HANDLE;
        throw std::runtime_error("Failed to create compute pipeline");
    }
}

void Pipeline::cleanupGraphics() {
    if (device != VK_NULL_HANDLE && graphicsPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, graphicsPipeline, nullptr);
    }
    if (device != VK_NULL_HANDLE && graphicsPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, graphicsPipelineLayout, nullptr);
    }

    graphicsPipeline = VK_NULL_HANDLE;
    graphicsPipelineLayout = VK_NULL_HANDLE;

    if (computePipeline == VK_NULL_HANDLE && computePipelineLayout == VK_NULL_HANDLE) {
        device = VK_NULL_HANDLE;
    }
}

void Pipeline::cleanupCompute() {
    if (device != VK_NULL_HANDLE && computePipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, computePipeline, nullptr);
    }
    if (device != VK_NULL_HANDLE && computePipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, computePipelineLayout, nullptr);
    }

    computePipeline = VK_NULL_HANDLE;
    computePipelineLayout = VK_NULL_HANDLE;

    if (graphicsPipeline == VK_NULL_HANDLE && graphicsPipelineLayout == VK_NULL_HANDLE) {
        device = VK_NULL_HANDLE;
    }
}

void Pipeline::cleanup() {
    cleanupGraphics();
    cleanupCompute();
}

void Pipeline::bindGraphics(VkCommandBuffer commandBuffer) const {
    if (graphicsPipeline == VK_NULL_HANDLE) {
        throw std::runtime_error("Pipeline::bindGraphics: graphics pipeline not created");
    }
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
}

void Pipeline::bindCompute(VkCommandBuffer commandBuffer) const {
    if (computePipeline == VK_NULL_HANDLE) {
        throw std::runtime_error("Pipeline::bindCompute: compute pipeline not created");
    }
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
}

VkShaderModule Pipeline::createShaderModule(VkDevice device, const std::vector<uint32_t>& spirvCode) {
    if (device == VK_NULL_HANDLE) {
        throw std::runtime_error("Pipeline::createShaderModule: device is null");
    }
    if (spirvCode.empty()) {
        throw std::runtime_error("Pipeline::createShaderModule: SPIR-V code is empty");
    }

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spirvCode.size() * sizeof(uint32_t);
    createInfo.pCode = spirvCode.data();

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module");
    }
    return shaderModule;
}

void Pipeline::destroyShaderModule(VkDevice device, VkShaderModule shaderModule) {
    if (device == VK_NULL_HANDLE || shaderModule == VK_NULL_HANDLE) {
        return;
    }
    vkDestroyShaderModule(device, shaderModule, nullptr);
}
