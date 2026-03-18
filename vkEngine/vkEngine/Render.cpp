#include "Render.h"

#include <stdexcept>
#include <cstring>

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::vec4 cameraPos;
    alignas(16) glm::vec4 lightDir;
    alignas(16) glm::vec4 lightColor;
};

Render::Render(const std::string& appName, GLFWwindow* window)
{
    _engine = new vkEngine(appName, window);
    _engine->init();

    auto extent = _engine->getSwapChainExtent();

    _camera = new Camera(glm::vec3(0, 0, 3));
    _camera->SetResolution(extent.width, extent.height);

    initOpaqueRenderPass();
    initOpaquePipeline();
    initOpaqueFramebuffers();
    initSyncObjects();
    initCameraDescriptorResources();
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

    cleanupCameraDescriptorResources();
    cleanupSyncObjects();
    cleanupOpaquePipelineResources();
    cleanupActorRefs();

    delete _camera;
    _camera = nullptr;

    delete _engine;
    _engine = nullptr;
}

void Render::addActor(Actor* actor)
{
    if (actor == nullptr)
    {
        return;
    }

    actor->init(_engine);

    _opaqueActors.push_back(actor);
}

Camera* Render::getCamera()
{
    return _camera;
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

    drawOpaqueCommandBuffer(commandBuffer, imageIndex);

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

void Render::drawOpaqueCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    auto swapChainExtent = _engine->getSwapChainExtent();
    if (imageIndex >= _opaqueFramebuffers.size()) {
        throw std::runtime_error("swapchain image index out of opaque framebuffer range");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = _opaqueRenderPass;
    renderPassInfo.framebuffer = _opaqueFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapChainExtent;

    VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _opaquePipeline);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapChainExtent.width;
    viewport.height = (float)swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    for (const auto& actor : _opaqueActors)
    {
        UniformBufferObject ubo{};
        ubo.model = actor->getTransformMatrix();
        ubo.view = _camera->GetViewMatrix();
        ubo.proj = _camera->GetProjectionMatrix(CameraType::PerspectiveAsym);
        ubo.cameraPos = glm::vec4(_camera->GetPosition(), 1.0f);
        ubo.lightDir = glm::vec4(glm::normalize(glm::vec3(0.4f, 0.7f, 0.2f)), 0.0f);
        ubo.lightColor = glm::vec4(1.6f, 1.6f, 1.6f, 1.0f);
        memcpy(_cameraUniformBuffersMapped[_currentFrame], &ubo, sizeof(ubo));

        actor->draw(commandBuffer, _opaquePipelineLayout, _cameraDescriptorSets[_currentFrame]);
    }

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
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

    if (vkCreateRenderPass(_engine->getLogicalDevice(), &renderPassInfo, nullptr, &_opaqueRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

void Render::initOpaquePipeline()
{
    VkShaderModule vertShaderModule = _engine->createShader("../Dependence/shader/bin/OpaqueShader.vert.spv", Vertex);
    VkShaderModule fragShaderModule = _engine->createShader("../Dependence/shader/bin/OpaqueShader.frag.spv", Fragment);

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

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

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
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    if (_cameraDescriptorSetLayout == VK_NULL_HANDLE) {
        VkDescriptorSetLayoutBinding cameraUboBinding{};
        cameraUboBinding.binding = 0;
        cameraUboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        cameraUboBinding.descriptorCount = 1;
        cameraUboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo cameraLayoutInfo{};
        cameraLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        cameraLayoutInfo.bindingCount = 1;
        cameraLayoutInfo.pBindings = &cameraUboBinding;

        if (vkCreateDescriptorSetLayout(_engine->getLogicalDevice(), &cameraLayoutInfo, nullptr, &_cameraDescriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create camera descriptor set layout!");
        }
    }

    if (_materialDescriptorSetLayout == VK_NULL_HANDLE) {
        VkDescriptorSetLayoutBinding materialUboBinding{};
        materialUboBinding.binding = 0;
        materialUboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        materialUboBinding.descriptorCount = 1;
        materialUboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo materialLayoutInfo{};
        materialLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        materialLayoutInfo.bindingCount = 1;
        materialLayoutInfo.pBindings = &materialUboBinding;

        if (vkCreateDescriptorSetLayout(_engine->getLogicalDevice(), &materialLayoutInfo, nullptr, &_materialDescriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create material descriptor set layout!");
        }
    }

    VkDescriptorSetLayout setLayouts[] = { _cameraDescriptorSetLayout, _materialDescriptorSetLayout };
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 2;
    pipelineLayoutInfo.pSetLayouts = setLayouts;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    if (vkCreatePipelineLayout(_engine->getLogicalDevice(), &pipelineLayoutInfo, nullptr, &_opaquePipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
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
    pipelineInfo.layout = _opaquePipelineLayout;
    pipelineInfo.renderPass = _opaqueRenderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(_engine->getLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_opaquePipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(_engine->getLogicalDevice(), fragShaderModule, nullptr);
    vkDestroyShaderModule(_engine->getLogicalDevice(), vertShaderModule, nullptr);
}

void Render::initCameraDescriptorResources()
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    _cameraUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    _cameraUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    _cameraUniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);
    _cameraDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        _engine->createVKBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            _cameraUniformBuffers[i], _cameraUniformBuffersMemory[i]);

        vkMapMemory(_engine->getLogicalDevice(), _cameraUniformBuffersMemory[i],
            0, bufferSize, 0, &_cameraUniformBuffersMapped[i]);
    }

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;

    if (vkCreateDescriptorPool(_engine->getLogicalDevice(), &poolInfo, nullptr, &_cameraDescriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create camera descriptor pool!");
    }

    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, _cameraDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = _cameraDescriptorPool;
    allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts = layouts.data();

    if (vkAllocateDescriptorSets(_engine->getLogicalDevice(), &allocInfo, _cameraDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate camera descriptor sets!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = _cameraUniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = _cameraDescriptorSets[i];
        write.dstBinding = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(_engine->getLogicalDevice(), 1, &write, 0, nullptr);
    }
}

void Render::initOpaqueFramebuffers()
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

void Render::cleanupOpaquePipelineResources()
{
    VkDevice device = _engine->getLogicalDevice();
    if (device == VK_NULL_HANDLE) {
        return;
    }

    cleanupOpaqueFramebuffers();

    if (_opaquePipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, _opaquePipeline, nullptr);
        _opaquePipeline = VK_NULL_HANDLE;
    }

    if (_opaquePipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, _opaquePipelineLayout, nullptr);
        _opaquePipelineLayout = VK_NULL_HANDLE;
    }

    if (_opaqueRenderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, _opaqueRenderPass, nullptr);
        _opaqueRenderPass = VK_NULL_HANDLE;
    }

    if (_materialDescriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, _materialDescriptorSetLayout, nullptr);
        _materialDescriptorSetLayout = VK_NULL_HANDLE;
    }

    if (_cameraDescriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, _cameraDescriptorSetLayout, nullptr);
        _cameraDescriptorSetLayout = VK_NULL_HANDLE;
    }
}

void Render::cleanupCameraDescriptorResources()
{
    if (_engine == nullptr) {
        return;
    }

    VkDevice device = _engine->getLogicalDevice();
    if (device == VK_NULL_HANDLE) {
        return;
    }

    for (size_t i = 0; i < _cameraUniformBuffers.size(); ++i) {
        if (_cameraUniformBuffersMapped[i] != nullptr) {
            vkUnmapMemory(device, _cameraUniformBuffersMemory[i]);
            _cameraUniformBuffersMapped[i] = nullptr;
        }
        if (_cameraUniformBuffers[i] != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, _cameraUniformBuffers[i], nullptr);
        }
        if (_cameraUniformBuffersMemory[i] != VK_NULL_HANDLE) {
            vkFreeMemory(device, _cameraUniformBuffersMemory[i], nullptr);
        }
    }
    _cameraUniformBuffers.clear();
    _cameraUniformBuffersMemory.clear();
    _cameraUniformBuffersMapped.clear();
    _cameraDescriptorSets.clear();

    if (_cameraDescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, _cameraDescriptorPool, nullptr);
        _cameraDescriptorPool = VK_NULL_HANDLE;
    }
}

void Render::cleanupOpaqueFramebuffers()
{
    VkDevice device = _engine->getLogicalDevice();
    if (device == VK_NULL_HANDLE) {
        return;
    }

    for (auto framebuffer : _opaqueFramebuffers) {
        if (framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
    }
    _opaqueFramebuffers.clear();
}

void Render::recreateSwapChainResources()
{
    cleanupOpaquePipelineResources();
    _engine->resetSwapChain();
    initOpaqueRenderPass();
    initOpaquePipeline();
    initOpaqueFramebuffers();
}

void Render::cleanupActorRefs()
{
    // Actor ownership is external; Render only stores non-owning pointers.
    _opaqueActors.clear();
}