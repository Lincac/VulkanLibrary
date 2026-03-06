#pragma once

#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>

#include "Instance.h"
#include "Device.h"

class Render
{
public:

	Render(GLFWwindow* window);
	~Render();

private:

	Instance* _instance;
	PhysicalDevice* _physicalDevice;
	Device* _logicalDevice;

	VkSurfaceKHR _surface;

};

