#pragma once

#include "Swapchain.h"

class RenderPass
{
public:

	RenderPass(const Device& logicalDevice, const Swapchain& swapchain);

public:

	VkRenderPass getRenderPass() const;

private:

	VkRenderPass _renderPass;

};

