#pragma once

#include "Swapchain.h"

class RenderPass
{
public:

	RenderPass();

public:

	void setDependice(Swapchain* swapchain);

	int create();

	VkRenderPass getRenderPass() const;

private:

	Swapchain* _swapchain;

private:

	VkRenderPass _renderPass;

};

