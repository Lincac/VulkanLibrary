#include "Render.h"

#include <fstream>
#include <stdexcept>

Render::Render(const std::string& appName, GLFWwindow* window)
{
    _engine = new vkEngine(appName, window);
    _engine->init();

    initOpaqueRenderPass();
    initOpaquePipelineLayout();
    recreateOpaqueFramebuffers();
    initSyncObjects();
}

Render::~Render()
{
    if (_engine == nullptr) {
        return;
    }

    VkDevice device = _engine->getLogicalDevice();
    if (device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device);
    }

    cleanupOpaqueResources();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        if (_imageAvailableSemaphores.size() > i && _imageAvailableSemaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, _imageAvailableSemaphores[i], nullptr);
        }
        if (_renderFinishedSemaphores.size() > i && _renderFinishedSemaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, _renderFinishedSemaphores[i], nullptr);
        }
        if (_inFlightFences.size() > i && _inFlightFences[i] != VK_NULL_HANDLE) {
            vkDestroyFence(device, _inFlightFences[i], nullptr);
        }
    }

    delete _engine;
    _engine = nullptr;
}

void Render::drawFrame()
{
    vkWaitForFences(_engine->getLogicalDevice(), 1, &_inFlightFences[_currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(
        _engine->getLogicalDevice(),
        _engine->getSwapChain(),
        UINT64_MAX,
        _imageAvailableSemaphores[_currentFrame],
        VK_NULL_HANDLE,
        &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    vkResetFences(_engine->getLogicalDevice(), 1, &_inFlightFences[_currentFrame]);

    VkCommandBuffer commandBuffer = _engine->getCommandBuffer(_currentFrame);
    vkResetCommandBuffer(commandBuffer, 0);
    recordCommandBuffer(commandBuffer, imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { _imageAvailableSemaphores[_currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VkSemaphore signalSemaphores[] = { _renderFinishedSemaphores[_currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(_engine->getGraphicsQueue(), 1, &submitInfo, _inFlightFences[_currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { _engine->getSwapChain() };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(_engine->getPresentQueue(), &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _framebufferResized) {
        _framebufferResized = false;
        recreateSwapChain();
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    _currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Render::addOpaqueActor(const Actor& actor)
{
    if (actor.getVertexShader().empty() || actor.getFragmentShader().empty()) {
        throw std::runtime_error("opaque actor must provide both vertex and fragment shaders");
    }

    OpaqueActorRuntime runtime{};
    runtime.actor = actor;
    runtime.pipeline = createOpaquePipeline(actor);
    _opaqueActors.push_back(runtime);
}

void Render::clearOpaqueActors()
{
    VkDevice device = _engine->getLogicalDevice();
    for (auto& actor : _opaqueActors) {
        if (actor.pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, actor.pipeline, nullptr);
            actor.pipeline = VK_NULL_HANDLE;
        }
    }
    _opaqueActors.clear();
}

void Render::initOpaqueRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = _engine->getSwapChainImageFormat();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(_engine->getLogicalDevice(), &renderPassInfo, nullptr, &_opaqueRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create opaque render pass!");
    }
}

void Render::initOpaquePipelineLayout()
{
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    if (vkCreatePipelineLayout(_engine->getLogicalDevice(), &pipelineLayoutInfo, nullptr, &_opaquePipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create opaque pipeline layout!");
    }
}

VkPipeline Render::createOpaquePipeline(const Actor& actor) const
{
    VkShaderModule vertModule = createShaderModule(actor.getVertexShader());
    VkShaderModule fragModule = createShaderModule(actor.getFragmentShader());

    VkPipelineShaderStageCreateInfo shaderStages[2]{};
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertModule;
    shaderStages[0].pName = "main";
    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragModule;
    shaderStages[1].pName = "main";

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(actor.getBindings().size());
    vertexInputInfo.pVertexBindingDescriptions = actor.getBindings().empty() ? nullptr : actor.getBindings().data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(actor.getAttributes().size());
    vertexInputInfo.pVertexAttributeDescriptions = actor.getAttributes().empty() ? nullptr : actor.getAttributes().data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

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
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = _opaquePipelineLayout;
    pipelineInfo.renderPass = _opaqueRenderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    VkPipeline pipeline = VK_NULL_HANDLE;
    if (vkCreateGraphicsPipelines(_engine->getLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
        vkDestroyShaderModule(_engine->getLogicalDevice(), fragModule, nullptr);
        vkDestroyShaderModule(_engine->getLogicalDevice(), vertModule, nullptr);
        throw std::runtime_error("failed to create opaque graphics pipeline!");
    }

    vkDestroyShaderModule(_engine->getLogicalDevice(), fragModule, nullptr);
    vkDestroyShaderModule(_engine->getLogicalDevice(), vertModule, nullptr);
    return pipeline;
}

void Render::rebuildOpaquePipelines()
{
    VkDevice device = _engine->getLogicalDevice();
    for (auto& actor : _opaqueActors) {
        if (actor.pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, actor.pipeline, nullptr);
            actor.pipeline = VK_NULL_HANDLE;
        }
        actor.pipeline = createOpaquePipeline(actor.actor);
    }
}

void Render::recreateOpaqueFramebuffers()
{
    cleanupOpaqueFramebuffers();

    const uint32_t imageCount = _engine->getSwapChainImageCount();
    _opaqueFramebuffers.resize(imageCount, VK_NULL_HANDLE);

    for (uint32_t i = 0; i < imageCount; ++i) {
        VkImageView attachments[] = { _engine->getSwapChainImageView(i) };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = _opaqueRenderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = _engine->getSwapChainExtent().width;
        framebufferInfo.height = _engine->getSwapChainExtent().height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(_engine->getLogicalDevice(), &framebufferInfo, nullptr, &_opaqueFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create opaque framebuffer!");
        }
    }
}

void Render::cleanupOpaqueFramebuffers()
{
    VkDevice device = _engine->getLogicalDevice();
    for (VkFramebuffer framebuffer : _opaqueFramebuffers) {
        if (framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
    }
    _opaqueFramebuffers.clear();
}

void Render::cleanupOpaqueResources()
{
    clearOpaqueActors();
    cleanupOpaqueFramebuffers();

    VkDevice device = _engine->getLogicalDevice();
    if (_opaquePipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, _opaquePipelineLayout, nullptr);
        _opaquePipelineLayout = VK_NULL_HANDLE;
    }
    if (_opaqueRenderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, _opaqueRenderPass, nullptr);
        _opaqueRenderPass = VK_NULL_HANDLE;
    }
}

VkShaderModule Render::createShaderModule(const std::string& spirvPath) const
{
    std::ifstream file(spirvPath, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open SPIR-V file: " + spirvPath);
    }

    const std::streamsize fileSize = file.tellg();
    if (fileSize <= 0) {
        throw std::runtime_error("invalid SPIR-V file size: " + spirvPath);
    }

    std::vector<char> code(static_cast<size_t>(fileSize));
    file.seekg(0);
    file.read(code.data(), fileSize);
    if (!file.good() && !file.eof()) {
        throw std::runtime_error("failed to read SPIR-V file: " + spirvPath);
    }

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule = VK_NULL_HANDLE;
    if (vkCreateShaderModule(_engine->getLogicalDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module: " + spirvPath);
    }

    return shaderModule;
}

void Render::initSyncObjects()
{
    _imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    _renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    _inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(_engine->getLogicalDevice(), &semaphoreInfo, nullptr, &_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(_engine->getLogicalDevice(), &semaphoreInfo, nullptr, &_renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(_engine->getLogicalDevice(), &fenceInfo, nullptr, &_inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

void Render::recreateSwapChain()
{
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(_engine->getWindow(), &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(_engine->getWindow(), &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(_engine->getLogicalDevice());

    _engine->resetSwapChain();

    cleanupOpaqueFramebuffers();
    if (_opaqueRenderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(_engine->getLogicalDevice(), _opaqueRenderPass, nullptr);
        _opaqueRenderPass = VK_NULL_HANDLE;
    }

    initOpaqueRenderPass();
    recreateOpaqueFramebuffers();
    rebuildOpaquePipelines();
}

void Render::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkExtent2D renderExtent = _engine->getSwapChainExtent();

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = _opaqueRenderPass;
    renderPassInfo.framebuffer = _opaqueFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = renderExtent;

    VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(renderExtent.width);
    viewport.height = static_cast<float>(renderExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = renderExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    for (const auto& actor : _opaqueActors) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, actor.pipeline);

        if (actor.actor.getVertexBuffer() != VK_NULL_HANDLE) {
            VkBuffer vertexBuffers[] = { actor.actor.getVertexBuffer() };
            VkDeviceSize offsets[] = { actor.actor.getVertexOffset() };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        }

        if (actor.actor.useIndexedDraw()) {
            vkCmdBindIndexBuffer(commandBuffer, actor.actor.getIndexBuffer(), 0, actor.actor.getIndexType());
            vkCmdDrawIndexed(commandBuffer, actor.actor.getIndexCount(), actor.actor.getInstanceCount(), 0, 0, 0);
        }
        else {
            vkCmdDraw(commandBuffer, actor.actor.getVertexCount(), actor.actor.getInstanceCount(), 0, 0);
        }
    }

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}