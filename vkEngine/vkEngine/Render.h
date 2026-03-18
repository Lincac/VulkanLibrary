#pragma once

#include "Actor.h"
#include "Camera.h"

class Render
{
public:

    Render(const std::string& appName, GLFWwindow* window);
    ~Render();

    void addActor(Actor* actor);

    Camera* getCamera();

    void draw();

private:

    void drawOpaqueCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

private:
    
    void initOpaqueRenderPass();

    void initOpaquePipeline();
    void initCameraDescriptorResources();
    void initOpaqueFramebuffers();
    void cleanupOpaqueFramebuffers();
    void recreateSwapChainResources();

    void initSyncObjects();

    void cleanupSyncObjects();

    void cleanupOpaquePipelineResources();
    void cleanupCameraDescriptorResources();

    void cleanupActorRefs();

private:

    vkEngine* _engine = nullptr;

    Camera* _camera;

    VkPipelineLayout _opaquePipelineLayout = VK_NULL_HANDLE;
    VkRenderPass _opaqueRenderPass = VK_NULL_HANDLE;
    VkPipeline _opaquePipeline = VK_NULL_HANDLE;
    VkDescriptorSetLayout _cameraDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout _materialDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool _cameraDescriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> _cameraDescriptorSets;
    std::vector<VkBuffer> _cameraUniformBuffers;
    std::vector<VkDeviceMemory> _cameraUniformBuffersMemory;
    std::vector<void*> _cameraUniformBuffersMapped;
    std::vector<VkFramebuffer> _opaqueFramebuffers;

    std::vector<Actor*> _opaqueActors;

    std::vector<VkSemaphore> _imageAvailableSemaphores;
    std::vector<VkSemaphore> _renderFinishedSemaphores;
    std::vector<VkFence> _inFlightFences;

    uint32_t _currentFrame = 0;
    bool _framebufferResized = false;
};
