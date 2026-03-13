#pragma once

#include "Actor.h"
#include "vkEngine.h"

class Render
{
public:
    using Actor = ::Actor;

    Render(const std::string& appName, GLFWwindow* window);
    ~Render();

public:
    void drawFrame();
    void addOpaqueActor(const Actor& actor);
    void clearOpaqueActors();

private:
    struct OpaqueActorRuntime
    {
        Actor actor;
        VkPipeline pipeline = VK_NULL_HANDLE;
    };

    void initOpaqueRenderPass();
    void initOpaquePipelineLayout();
    VkPipeline createOpaquePipeline(const Actor& actor) const;
    void rebuildOpaquePipelines();
    void recreateOpaqueFramebuffers();
    void cleanupOpaqueFramebuffers();
    void cleanupOpaqueResources();
    VkShaderModule createShaderModule(const std::string& spirvPath) const;
    void initSyncObjects();
    void recreateSwapChain();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

private:
    vkEngine* _engine = nullptr;

    VkRenderPass _opaqueRenderPass = VK_NULL_HANDLE;
    VkPipelineLayout _opaquePipelineLayout = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> _opaqueFramebuffers;
    std::vector<OpaqueActorRuntime> _opaqueActors;

    std::vector<VkSemaphore> _imageAvailableSemaphores;
    std::vector<VkSemaphore> _renderFinishedSemaphores;
    std::vector<VkFence> _inFlightFences;

    uint32_t _currentFrame = 0;
    bool _framebufferResized = false;
};
