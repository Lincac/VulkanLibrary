#pragma once

#include "Render_Opaque.h"
#include "Camera.h"

class Render
{
public:

    Render(const std::string& appName, GLFWwindow* window, bool enableMSAA = true);
    ~Render();

    void addActor(Actor* actor);

    void setMSAAEnabled(bool enableMSAA);

    Camera* getCamera();

    void draw();

private:
    
    void initSceneDescriptorResources();

    void initPostProcessResources();
    
    void updateSceneUniformBuffer(uint32_t frameIndex);

    void initSyncObjects();

    void recordQuadPass(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    void recreateSwapChainResources();

    void cleanupSyncObjects();

    void cleanupSceneDescriptorResources();

    void cleanupPostProcessResources();

private:

    vkEngine* _engine = nullptr;

    Render_Opaque* _opaqueRender;

    Camera* _camera;

    VkDescriptorSetLayout _sceneDescSetLayout = VK_NULL_HANDLE;

    VkDescriptorPool _sceneDescPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> _sceneDescSets;

    VkSampler _quadSampler = VK_NULL_HANDLE;
    VkRenderPass _quadRenderPass = VK_NULL_HANDLE;
    VkDescriptorSetLayout _quadDescSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout _quadPipelineLayout = VK_NULL_HANDLE;
    VkPipeline _quadPipeline = VK_NULL_HANDLE;
    VkDescriptorPool _quadDescPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> _quadDescSets;
    std::vector<VkFramebuffer> _quadFramebuffers;

    std::vector<VkBuffer> _sceneUniformBuffers;
    std::vector<VkDeviceMemory> _sceneUniformBuffersMemory;
    std::vector<void*> _sceneUniformBuffersMapped;

    std::vector<VkSemaphore> _imageAvailableSemaphores;
    std::vector<VkSemaphore> _renderFinishedSemaphores;
    std::vector<VkFence> _inFlightFences;

    uint32_t _currentFrame = 0;
    bool _framebufferResized = false;
    bool _enableMSAA = true;
};
