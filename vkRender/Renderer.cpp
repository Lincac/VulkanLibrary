#include "Renderer.h"

#include <stdexcept>

void Renderer::initialize(GLFWwindow* inWindow) {
    if (inWindow == nullptr) {
        throw std::runtime_error("Renderer::initialize: window is null");
    }
    if (initialized) {
        cleanup();
    }

    window = inWindow;
    context.initialize(window);

    createSwapchainImageViews();

    commandPool.initialize(
        context.getDevice(),
        context.getGraphicsQueueFamily(),
        context.getGraphicsQueue());

    renderPass.initialize(
        context.getDevice(),
        context.getSwapchainFormat(),
        context.getSwapchainExtent(),
        swapchainImageViews);

    createCommandBuffers();
    createSyncObjects();

    initialized = true;
}

void Renderer::run() {
    if (!initialized) {
        throw std::runtime_error("Renderer::run: renderer not initialized");
    }

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
    }

    vkDeviceWaitIdle(context.getDevice());
}

void Renderer::drawFrame() {
    if (!initialized) {
        throw std::runtime_error("Renderer::drawFrame: renderer not initialized");
    }

    VkDevice device = context.getDevice();
    VkSwapchainKHR swapchain = context.getSwapchain();
    VkQueue graphicsQueue = context.getGraphicsQueue();

    vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &inFlightFence);

    uint32_t imageIndex = 0;
    VkResult acquireResult = vkAcquireNextImageKHR(
        device,
        swapchain,
        UINT64_MAX,
        imageAvailableSemaphore,
        VK_NULL_HANDLE,
        &imageIndex);

    if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return;
    }
    if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swapchain image");
    }

    vkResetCommandBuffer(commandBuffers[imageIndex], 0);
    recordCommandBuffer(commandBuffers[imageIndex], imageIndex);

    VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore signalSemaphore = renderFinishedSemaphores[imageIndex];
    VkSemaphore signalSemaphores[] = { signalSemaphore };

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    VkResult presentResult = vkQueuePresentKHR(graphicsQueue, &presentInfo);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return;
    }
    // VK_SUBOPTIMAL_KHR: present 闂備胶鎳撻悺銊╁礉閺囩喐鍙忔繛鎴欏灪閺咁剟鎮橀悙鑸殿棄閻犱浇灏欑槐鎾存媴閸繃鍣介柡浣哄亾缁绘盯骞橀崘鑼獓闂佺粯鎸搁澶愬极瀹ュ洣娌柣鎰靛墾妤犲繘姊婚崒姘闁稿鎹囧娲箵閹烘枬銏㈢磽閸岋附瀚� swapchain
    if (presentResult != VK_SUCCESS && presentResult != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to present swapchain image");
    }
}

void Renderer::recreateSwapchain() {
    if (!initialized) {
        throw std::runtime_error("Renderer::recreateSwapchain: renderer not initialized");
    }

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwWaitEvents();
        glfwGetFramebufferSize(window, &width, &height);
    }

    VkDevice device = context.getDevice();
    vkDeviceWaitIdle(device);

    if (!commandBuffers.empty()) {
        commandPool.freeCommandBuffers(commandBuffers);
        commandBuffers.clear();
    }

    // Graphics pipeline depends on render pass/extent; caller should recreate it with new shaders.
    if (graphicsPipelineReady) {
        pipeline.cleanupGraphics();
        graphicsPipelineReady = false;
    }

    renderPass.cleanup();
    destroySwapchainImageViews();
    context.recreateSwapchain();
    createSwapchainImageViews();

    renderPass.initialize(
        context.getDevice(),
        context.getSwapchainFormat(),
        context.getSwapchainExtent(),
        swapchainImageViews);

    createCommandBuffers();
    destroySyncObjects();
    createSyncObjects();

    if (storedVertModule != VK_NULL_HANDLE && storedFragModule != VK_NULL_HANDLE) {
        Pipeline::GraphicsCreateInfo info{};
        info.device = context.getDevice();
        info.renderPass = renderPass.getRenderPass();
        info.extent = context.getSwapchainExtent();
        info.vertShaderModule = storedVertModule;
        info.fragShaderModule = storedFragModule;
        info.enableDepthTest = false;
        pipeline.initializeGraphics(info);
        graphicsPipelineReady = true;
    }
}

void Renderer::cleanup() {
    if (!initialized) {
        return;
    }

    VkDevice device = context.getDevice();
    if (device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device);
    }

    pipeline.cleanup();
    graphicsPipelineReady = false;
    if (storedVertModule != VK_NULL_HANDLE) {
        Pipeline::destroyShaderModule(device, storedVertModule);
        storedVertModule = VK_NULL_HANDLE;
    }
    if (storedFragModule != VK_NULL_HANDLE) {
        Pipeline::destroyShaderModule(device, storedFragModule);
        storedFragModule = VK_NULL_HANDLE;
    }
    vertexBuffer.cleanup();
    vertexBufferReady = false;

    if (!commandBuffers.empty()) {
        commandPool.freeCommandBuffers(commandBuffers);
        commandBuffers.clear();
    }

    destroySyncObjects();
    renderPass.cleanup();
    commandPool.cleanup();
    destroySwapchainImageViews();
    context.cleanup();

    window = nullptr;
    initialized = false;
}

void Renderer::initializeGraphicsPipeline(
    VkShaderModule vertShaderModule,
    VkShaderModule fragShaderModule) {
    if (!initialized) {
        throw std::runtime_error("Renderer::initializeGraphicsPipeline: renderer not initialized");
    }

    VkDevice device = context.getDevice();
    if (storedVertModule != VK_NULL_HANDLE) {
        Pipeline::destroyShaderModule(device, storedVertModule);
    }
    if (storedFragModule != VK_NULL_HANDLE) {
        Pipeline::destroyShaderModule(device, storedFragModule);
    }
    storedVertModule = vertShaderModule;
    storedFragModule = fragShaderModule;

    Pipeline::GraphicsCreateInfo info{};
    info.device = device;
    info.renderPass = renderPass.getRenderPass();
    info.extent = context.getSwapchainExtent();
    info.vertShaderModule = vertShaderModule;
    info.fragShaderModule = fragShaderModule;
    info.enableDepthTest = false;

    pipeline.initializeGraphics(info);
    graphicsPipelineReady = true;
}

void Renderer::initializeVertexBuffer(const std::vector<Vertex>& vertices) {
    if (!initialized) {
        throw std::runtime_error("Renderer::initializeVertexBuffer: renderer not initialized");
    }

    vertexBuffer.initialize(
        context.getDevice(),
        context.getPhysicalDevice(),
        vertices);
    vertexBufferReady = true;
}

void Renderer::createSwapchainImageViews() {
    VkDevice device = context.getDevice();
    VkSwapchainKHR swapchain = context.getSwapchain();
    VkFormat format = context.getSwapchainFormat();

    uint32_t imageCount = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
    if (imageCount == 0) {
        throw std::runtime_error("No swapchain images found");
    }

    swapchainImages.resize(imageCount, VK_NULL_HANDLE);
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());

    swapchainImageViews.resize(imageCount, VK_NULL_HANDLE);
    for (uint32_t i = 0; i < imageCount; ++i) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = swapchainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &viewInfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS) {
            destroySwapchainImageViews();
            throw std::runtime_error("Failed to create swapchain image view");
        }
    }
}

void Renderer::destroySwapchainImageViews() {
    VkDevice device = context.getDevice();
    if (device != VK_NULL_HANDLE) {
        for (VkImageView imageView : swapchainImageViews) {
            if (imageView != VK_NULL_HANDLE) {
                vkDestroyImageView(device, imageView, nullptr);
            }
        }
    }

    swapchainImageViews.clear();
    swapchainImages.clear();
}

void Renderer::createCommandBuffers() {
    commandBuffers = commandPool.allocateCommandBuffers(
        static_cast<uint32_t>(swapchainImageViews.size()),
        VK_COMMAND_BUFFER_LEVEL_PRIMARY);
}

void Renderer::createSyncObjects() {
    VkDevice device = context.getDevice();

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS) {
        destroySyncObjects();
        throw std::runtime_error("Failed to create synchronization objects");
    }

    const uint32_t imageCount = static_cast<uint32_t>(swapchainImageViews.size());
    renderFinishedSemaphores.resize(imageCount, VK_NULL_HANDLE);
    for (uint32_t i = 0; i < imageCount; ++i) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS) {
            destroySyncObjects();
            throw std::runtime_error("Failed to create render finished semaphore");
        }
    }
}

void Renderer::destroySyncObjects() {
    VkDevice device = context.getDevice();
    if (device == VK_NULL_HANDLE) {
        imageAvailableSemaphore = VK_NULL_HANDLE;
        renderFinishedSemaphores.clear();
        inFlightFence = VK_NULL_HANDLE;
        return;
    }

    if (inFlightFence != VK_NULL_HANDLE) {
        vkDestroyFence(device, inFlightFence, nullptr);
    }
    for (VkSemaphore sem : renderFinishedSemaphores) {
        if (sem != VK_NULL_HANDLE) {
            vkDestroySemaphore(device, sem, nullptr);
        }
    }
    renderFinishedSemaphores.clear();
    if (imageAvailableSemaphore != VK_NULL_HANDLE) {
        vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
    }

    imageAvailableSemaphore = VK_NULL_HANDLE;
    inFlightFence = VK_NULL_HANDLE;
}

void Renderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer");
    }

    frameGraph.reset();

    FrameGraph::ImageDesc swapchainDesc{};
    swapchainDesc.width = context.getSwapchainExtent().width;
    swapchainDesc.height = context.getSwapchainExtent().height;
    swapchainDesc.depth = 1;
    swapchainDesc.layers = 1;
    swapchainDesc.mipLevels = 1;
    swapchainDesc.format = context.getSwapchainFormat();
    swapchainDesc.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainDesc.imported = true;
    swapchainDesc.exported = true;
    swapchainDesc.transient = false;

    const FrameGraph::ResourceId swapchainImageResource =
        frameGraph.createImage("swapchain_image", swapchainDesc);

    VkImageSubresourceRange range{};
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;
    range.levelCount = 1;
    range.baseArrayLayer = 0;
    range.layerCount = 1;
    frameGraph.bindImage(swapchainImageResource, swapchainImages[imageIndex], range);

    const FrameGraph::PassId clearPass = frameGraph.addPass("clear_and_draw", true);
    FrameGraph::ResourceUsage clearWrite{};
    clearWrite.resource = swapchainImageResource;
    clearWrite.access = FrameGraph::AccessType::Write;
    clearWrite.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    clearWrite.accessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    // Render pass ends in PRESENT layout; we track pass output state.
    clearWrite.imageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    frameGraph.writeResource(clearPass, clearWrite);

    frameGraph.setPassCallback(clearPass, [&](VkCommandBuffer cmd) {
        VkClearValue clearColor{};
        clearColor.color = { { 0.08f, 0.08f, 0.12f, 1.0f } };

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass.getRenderPass();
        renderPassInfo.framebuffer = renderPass.getFramebuffer(imageIndex);
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = context.getSwapchainExtent();
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        if (graphicsPipelineReady && vertexBufferReady) {
            pipeline.bindGraphics(cmd);
            vertexBuffer.bind(cmd);
            vkCmdDraw(cmd, vertexBuffer.getVertexCount(), 1, 0, 0);
        }

        vkCmdEndRenderPass(cmd);
    });

    // This pass represents present dependency edge, allowing FrameGraph to emit sync.
    const FrameGraph::PassId presentPass = frameGraph.addPass("present", true);
    FrameGraph::ResourceUsage presentRead{};
    presentRead.resource = swapchainImageResource;
    presentRead.access = FrameGraph::AccessType::Read;
    presentRead.stageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
    presentRead.accessMask = 0;
    presentRead.imageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    frameGraph.readResource(presentPass, presentRead);

    frameGraph.compile();
    frameGraph.execute(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record command buffer");
    }
}
