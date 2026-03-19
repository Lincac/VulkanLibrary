#pragma once

#include "Render_Opaque.h"
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
    
    void initSceneDescriptorResources();

    void initSyncObjects();

    void recreateSwapChainResources();

    void cleanupSyncObjects();

    void cleanupSceneDescriptorResources();

private:

    vkEngine* _engine = nullptr;

    Render_Opaque* _opaqueRender;

    Camera* _camera;

    VkDescriptorSetLayout _sceneDescSetLayout = VK_NULL_HANDLE;

    VkDescriptorPool _sceneDescPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> _sceneDescSets;

    std::vector<VkBuffer> _sceneUniformBuffers;
    std::vector<VkDeviceMemory> _sceneUniformBuffersMemory;
    std::vector<void*> _sceneUniformBuffersMapped;

    std::vector<VkSemaphore> _imageAvailableSemaphores;
    std::vector<VkSemaphore> _renderFinishedSemaphores;
    std::vector<VkFence> _inFlightFences;

    uint32_t _currentFrame = 0;
    bool _framebufferResized = false;
};
