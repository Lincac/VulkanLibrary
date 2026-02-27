#pragma once

#include "FrameGraph.h"
#include "Pipeline.h"
#include "RenderPass.h"
#include "VertexBuffer.h"
#include "CommandPool.h"
#include "Context.h"

#include <vector>

class Renderer {
public:
    void initialize(GLFWwindow* inWindow);

    void run();

    void drawFrame();

    void cleanup();

    void initializeGraphicsPipeline(
        VkShaderModule vertShaderModule,
        VkShaderModule fragShaderModule);
    void initializeVertexBuffer(const std::vector<Vertex>& vertices);

    Context& getContext() { return context; }
    const Context& getContext() const { return context; }
    RenderPass& getRenderPass() { return renderPass; }
    const RenderPass& getRenderPass() const { return renderPass; }
    Pipeline& getPipeline() { return pipeline; }
    const Pipeline& getPipeline() const { return pipeline; }

private:
    void recreateSwapchain();
    void createSwapchainImageViews();
    void destroySwapchainImageViews();
    void createCommandBuffers();
    void createSyncObjects();
    void destroySyncObjects();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

private:
    GLFWwindow* window = nullptr;

    Context context;
    CommandPool commandPool;
    RenderPass renderPass;
    Pipeline pipeline;
    VertexBuffer vertexBuffer;
    FrameGraph frameGraph;

    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkCommandBuffer> commandBuffers;

    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    VkFence inFlightFence = VK_NULL_HANDLE;

    bool initialized = false;
    bool graphicsPipelineReady = false;
    bool vertexBufferReady = false;

    VkShaderModule storedVertModule = VK_NULL_HANDLE;
    VkShaderModule storedFragModule = VK_NULL_HANDLE;
};
