#pragma once

#include "Actor.h"

class Render
{
public:

    Render(const std::string& appName, GLFWwindow* window);

    void addActor(Actor* actor);

    void draw();

private:
    
    void initOpaqueRenderPass();
    void initOpaquePipeline();

private:

    vkEngine* _engine = nullptr;

    VkPipelineLayout _opaquePipelineLayout;
    VkRenderPass _opaqueRenderPass;
    VkPipeline _opaquePipeline;

    std::vector<Actor*> _opaqueActors;

    uint32_t _currentFrame = 0;
    bool _framebufferResized = false;
};
