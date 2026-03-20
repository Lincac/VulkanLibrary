#include "Render.h"

#include <stdexcept>
#include <cstring>
#include "Render_Opaque.h"

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::vec4 cameraPos;

    alignas(16) glm::vec4 lightDir;
    alignas(16) glm::vec4 lightColor;
};

Render::Render(const std::string& appName, GLFWwindow* window, bool enableMSAA)
{
    _enableMSAA = enableMSAA;
    _engine = new vkEngine(appName, window);
    _engine->init();

    auto extent = _engine->getSwapChainExtent();

    _camera = new Camera(glm::vec3(0, 0, 3));
    _camera->SetResolution(extent.width, extent.height);

    VkDescriptorSetLayoutBinding uboBinding{};
    uboBinding.binding = 0;
    uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboBinding.descriptorCount = 1;
    uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo uboLayoutInfo{};
    uboLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    uboLayoutInfo.bindingCount = 1;
    uboLayoutInfo.pBindings = &uboBinding;

    if (vkCreateDescriptorSetLayout(_engine->getLogicalDevice(), &uboLayoutInfo, nullptr, &_sceneDescSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create camera descriptor set layout!");
    }

    initSyncObjects();
    initSceneDescriptorResources();

    _opaqueRender = new Render_Opaque(_engine, _sceneDescSetLayout, _enableMSAA);

    initPostProcessResources();
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

    delete _opaqueRender;
    _opaqueRender = nullptr;

    cleanupPostProcessResources();
    cleanupSceneDescriptorResources();
    cleanupSyncObjects();

    if (_sceneDescSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, _sceneDescSetLayout, nullptr);
        _sceneDescSetLayout = VK_NULL_HANDLE;
    }

    delete _camera;
    _camera = nullptr;

    delete _engine;
    _engine = nullptr;
}

void Render::addActor(Actor* actor)
{
    _opaqueRender->addActor(actor);
}

Camera* Render::getCamera()
{
    return _camera;
}

void Render::setMSAAEnabled(bool enableMSAA)
{
    if (_enableMSAA == enableMSAA) {
        return;
    }
    if (_engine == nullptr) {
        _enableMSAA = enableMSAA;
        return;
    }

    VkDevice device = _engine->getLogicalDevice();
    if (device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device);
    }

    _enableMSAA = enableMSAA;

    delete _opaqueRender;
    _opaqueRender = nullptr;
    cleanupPostProcessResources();

    _opaqueRender = new Render_Opaque(_engine, _sceneDescSetLayout, _enableMSAA);
    initPostProcessResources();
}

void Render::draw()
{
    vkWaitForFences(_engine->getLogicalDevice(), 1, &_inFlightFences[_currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(_engine->getLogicalDevice(), _engine->getSwapChain(), 
        UINT64_MAX, _imageAvailableSemaphores[_currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChainResources();
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    auto commandBuffer = _engine->getCommandBuffer(_currentFrame);

    vkResetFences(_engine->getLogicalDevice(), 1, &_inFlightFences[_currentFrame]);
    vkResetCommandBuffer(commandBuffer, /*VkCommandBufferResetFlagBits*/ 0);
    
    updateSceneUniformBuffer(_currentFrame);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    _opaqueRender->draw(commandBuffer, _sceneDescSets[_currentFrame], imageIndex);
    recordQuadPass(commandBuffer, imageIndex);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }

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

    VkSwapchainKHR swapChains[] = { _engine->getSwapChain()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(_engine->getPresentQueue(), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _framebufferResized) {
        _framebufferResized = false;
        recreateSwapChainResources();
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    _currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Render::updateSceneUniformBuffer(uint32_t frameIndex)
{
    if (frameIndex >= _sceneUniformBuffersMapped.size() || _sceneUniformBuffersMapped[frameIndex] == nullptr) {
        return;
    }

    UniformBufferObject ubo{};
    ubo.model = glm::mat4(1.0f);
    ubo.view = _camera->GetViewMatrix();
    ubo.proj = _camera->GetProjectionMatrix(CameraType::Perspective);
    ubo.proj[1][1] *= -1.0f; // Vulkan clip space has inverted Y

    glm::vec3 camPos = _camera->GetPosition();
    ubo.cameraPos = glm::vec4(camPos, 1.0f);
    ubo.lightDir = glm::vec4(glm::normalize(glm::vec3(0.4f, -1.0f, 0.3f)), 0.0f);
    ubo.lightColor = glm::vec4(1.0f);

    memcpy(_sceneUniformBuffersMapped[frameIndex], &ubo, sizeof(ubo));
}

void Render::initSceneDescriptorResources()
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    _sceneUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    _sceneUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    _sceneUniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);
    _sceneDescSets.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        _engine->createVKBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            _sceneUniformBuffers[i], _sceneUniformBuffersMemory[i]);

        vkMapMemory(_engine->getLogicalDevice(), _sceneUniformBuffersMemory[i],
            0, bufferSize, 0, &_sceneUniformBuffersMapped[i]);
    }

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;

    if (vkCreateDescriptorPool(_engine->getLogicalDevice(), &poolInfo, nullptr, &_sceneDescPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create camera descriptor pool!");
    }

    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, _sceneDescSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = _sceneDescPool;
    allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts = layouts.data();

    if (vkAllocateDescriptorSets(_engine->getLogicalDevice(), &allocInfo, _sceneDescSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate camera descriptor sets!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = _sceneUniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = _sceneDescSets[i];
        write.dstBinding = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(_engine->getLogicalDevice(), 1, &write, 0, nullptr);
    }
}

void Render::initPostProcessResources()
{
    auto extent = _engine->getSwapChainExtent();
    const uint32_t imageCount = _engine->getSwapChainImageCount();
    const VkFormat colorFormat = _engine->getSwapChainImageFormat();
    VkDevice device = _engine->getLogicalDevice();

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    if (vkCreateSampler(device, &samplerInfo, nullptr, &_quadSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create quad sampler!");
    }

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = colorFormat;
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
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &_quadRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create quad render pass!");
    }

    VkDescriptorSetLayoutBinding texBinding{};
    texBinding.binding = 0;
    texBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texBinding.descriptorCount = 1;
    texBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &texBinding;
    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &_quadDescSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create quad descriptor set layout!");
    }

    VkShaderModule vertShaderModule = _engine->createShader("../Dependence/shader/bin/quad.vert.spv", Vertex);
    VkShaderModule fragShaderModule = _engine->createShader("../Dependence/shader/bin/quad.frag.spv", Fragment);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
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
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    std::array<VkDynamicState, 2> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &_quadDescSetLayout;
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &_quadPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create quad pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = _quadPipelineLayout;
    pipelineInfo.renderPass = _quadRenderPass;
    pipelineInfo.subpass = 0;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_quadPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create quad pipeline!");
    }

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = imageCount;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = imageCount;
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &_quadDescPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create quad descriptor pool!");
    }

    _quadDescSets.resize(imageCount, VK_NULL_HANDLE);
    std::vector<VkDescriptorSetLayout> setLayouts(imageCount, _quadDescSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = _quadDescPool;
    allocInfo.descriptorSetCount = imageCount;
    allocInfo.pSetLayouts = setLayouts.data();
    if (vkAllocateDescriptorSets(device, &allocInfo, _quadDescSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate quad descriptor sets!");
    }

    for (uint32_t i = 0; i < imageCount; ++i) {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = _opaqueRender->getMainColorImageView(i);
        imageInfo.sampler = _quadSampler;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = _quadDescSets[i];
        write.dstBinding = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = 1;
        write.pImageInfo = &imageInfo;
        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }

    _quadFramebuffers.resize(imageCount, VK_NULL_HANDLE);
    for (uint32_t i = 0; i < imageCount; ++i) {
        VkImageView attachment = _engine->getSwapChainImageView(i);

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = _quadRenderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &attachment;
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;
        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &_quadFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create quad framebuffer!");
        }
    }
}

void Render::recordQuadPass(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    if (imageIndex >= _quadFramebuffers.size() || imageIndex >= _quadDescSets.size()) {
        throw std::runtime_error("swapchain image index out of quad pass range");
    }

    auto extent = _engine->getSwapChainExtent();

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = _quadRenderPass;
    renderPassInfo.framebuffer = _quadFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = extent;

    VkClearValue clearColor{};
    clearColor.color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _quadPipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = extent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        _quadPipelineLayout,
        0,
        1,
        &_quadDescSets[imageIndex],
        0,
        nullptr);

    vkCmdDraw(commandBuffer, 4, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);
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

void Render::cleanupSyncObjects()
{
    VkDevice device = _engine->getLogicalDevice();
    if (device == VK_NULL_HANDLE) {
        return;
    }

    for (auto fence : _inFlightFences) {
        if (fence != VK_NULL_HANDLE) {
            vkDestroyFence(device, fence, nullptr);
        }
    }

    for (auto semaphore : _renderFinishedSemaphores) {
        if (semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, semaphore, nullptr);
        }
    }

    for (auto semaphore : _imageAvailableSemaphores) {
        if (semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, semaphore, nullptr);
        }
    }

    _inFlightFences.clear();
    _renderFinishedSemaphores.clear();
    _imageAvailableSemaphores.clear();
}

void Render::cleanupSceneDescriptorResources()
{
    if (_engine == nullptr) {
        return;
    }

    VkDevice device = _engine->getLogicalDevice();
    if (device == VK_NULL_HANDLE) {
        return;
    }

    for (size_t i = 0; i < _sceneUniformBuffers.size(); ++i) {
        if (_sceneUniformBuffersMapped[i] != nullptr) {
            vkUnmapMemory(device, _sceneUniformBuffersMemory[i]);
            _sceneUniformBuffersMapped[i] = nullptr;
        }
        if (_sceneUniformBuffers[i] != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, _sceneUniformBuffers[i], nullptr);
        }
        if (_sceneUniformBuffersMemory[i] != VK_NULL_HANDLE) {
            vkFreeMemory(device, _sceneUniformBuffersMemory[i], nullptr);
        }
    }
    _sceneUniformBuffers.clear();
    _sceneUniformBuffersMemory.clear();
    _sceneUniformBuffersMapped.clear();
    _sceneDescSets.clear();

    if (_sceneDescPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, _sceneDescPool, nullptr);
        _sceneDescPool = VK_NULL_HANDLE;
    }
}

void Render::cleanupPostProcessResources()
{
    if (_engine == nullptr) {
        return;
    }

    VkDevice device = _engine->getLogicalDevice();
    if (device == VK_NULL_HANDLE) {
        return;
    }

    for (auto framebuffer : _quadFramebuffers) {
        if (framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
    }
    _quadFramebuffers.clear();
    _quadDescSets.clear();

    if (_quadDescPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, _quadDescPool, nullptr);
        _quadDescPool = VK_NULL_HANDLE;
    }

    if (_quadPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, _quadPipeline, nullptr);
        _quadPipeline = VK_NULL_HANDLE;
    }

    if (_quadPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, _quadPipelineLayout, nullptr);
        _quadPipelineLayout = VK_NULL_HANDLE;
    }

    if (_quadDescSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, _quadDescSetLayout, nullptr);
        _quadDescSetLayout = VK_NULL_HANDLE;
    }

    if (_quadRenderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, _quadRenderPass, nullptr);
        _quadRenderPass = VK_NULL_HANDLE;
    }

    if (_quadSampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, _quadSampler, nullptr);
        _quadSampler = VK_NULL_HANDLE;
    }
}

void Render::recreateSwapChainResources()
{
     _opaqueRender->cleanup();
    cleanupPostProcessResources();

    _engine->resetSwapChain();
    auto extent = _engine->getSwapChainExtent();
    _camera->SetResolution(extent.width, extent.height);
    _opaqueRender->init();
    initPostProcessResources();
}