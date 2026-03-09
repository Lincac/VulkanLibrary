#pragma once

#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>

#include "Instance.h"
#include "Device.h"
#include "CommandPool.h"
#include "RenderPipeline.h"
#include "RenderPass.h"

class Render
{
public:

	Render(GLFWwindow* window);
	~Render();

public:

	void drawFrame(GLFWwindow* window);

private:

	void initSyncObjects();

	void recreateSwapChain(GLFWwindow* window);

	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

private:

	Instance* _instance;

	PhysicalDevice* _physicalDevice;

	Device* _logicalDevice;

	Swapchain* _swapChain;

	RenderPass* _renderPass;

	RenderPipeline* _renderPipeline;

	CommandPool* _commandPool;

	VkSurfaceKHR _surface;

	std::vector<VkSemaphore> _imageAvailableSemaphores;
	std::vector<VkSemaphore> _renderFinishedSemaphores;
	std::vector<VkFence> _inFlightFences;

	uint32_t _currentFrame = 0;
	bool _framebufferResized = false;

};

