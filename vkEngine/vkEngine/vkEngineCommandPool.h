#pragma once

#include "vkEngineLogicalDevice.h"

class vkEngineCommandPool
{
public:
    vkEngineCommandPool(vkEngineLogicalDevice& device);
    ~vkEngineCommandPool();

private:
    vkEngineLogicalDevice& _device;

    VkCommandPool _commandPool;
    std::vector<VkCommandBuffer> _commandBuffers;

};