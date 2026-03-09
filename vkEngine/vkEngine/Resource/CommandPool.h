#pragma once

#include "RenderPass.h"

const int MAX_FRAMES_IN_FLIGHT = 2;

class CommandPool
{
public:

	CommandPool(const PhysicalDevice& physicalDevice, const Device& logicalDevice);

public:

	VkCommandBuffer getCommandBuffer(uint32_t index);

	VkCommandBuffer* getCommandBuffers();

private:

	VkCommandPool _commandPool;
	std::vector<VkCommandBuffer> _commandBuffers;

};

