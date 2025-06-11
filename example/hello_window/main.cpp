#include <iostream>

#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <glfwCallback.h>

#include <VulkanAPI/VulkanAPI.h>

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

	glfwSetWindowSizeCallback(window, WindowSizeCallback);

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::cout << "glfw extensions" << std::endl;
	for (uint32_t i = 0; i < glfwExtensionCount; i++)
	{
		std::cout << glfwExtensions[i] << std::endl;
	}

	std::cout << std::endl;

	auto extensions = VulkanAPI::getVKExtensionProperties();
	std::cout << "vulkan extensions" << std::endl;
	for (auto extension : extensions)
	{
		std::cout << extension.extensionName << std::endl;
	}

	VulkanAPI vulkan;
	vulkan.setApplicationInfo();
	vulkan.createInstance(glfwExtensions, glfwExtensionCount);

	glm::mat4 matrix;
	glm::vec4 vec;
	auto test = matrix * vec;

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
	}

	glfwDestroyWindow(window);

	glfwTerminate();

	return 0;
}