#include "Render.h"

#include <stdexcept>

Render::Render(GLFWwindow* window)
{
	_instance = new Instance("Demo");

	if (glfwCreateWindowSurface(_instance->getInstance(), window, nullptr, &_surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}

	_physicalDevice = new PhysicalDevice(_instance->getInstance(), _surface);
	_logicalDevice = new Device(*_physicalDevice);
}

Render::~Render()
{
}
