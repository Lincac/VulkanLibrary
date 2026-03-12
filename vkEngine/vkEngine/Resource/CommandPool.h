#pragma once

#include "RenderPass.h"

const int MAX_FRAMES_IN_FLIGHT = 2;

class CommandPool
{
public:

	CommandPool();

	void setDependice(Device* logicalDevice);

	int create();

public:

	VkCommandBuffer getCommandBuffer(uint32_t index);

	VkCommandBuffer* getCommandBuffers();

private:

	Device* _logicalDevice;

private:

	VkCommandPool _commandPool;
	std::vector<VkCommandBuffer> _commandBuffers;

};

