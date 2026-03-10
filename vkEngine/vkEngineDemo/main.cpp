#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>	

#include "Render.h"
#include "ShaderModule.h"

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	auto window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);

	Render render(window);

	ShaderModule vShader(*render.getDevice(), "../Dependence/shader/bin/triangle.vert.spv", ShaderType::Vertex);
	ShaderModule fShader(*render.getDevice(), "../Dependence/shader/bin/triangle.frag.spv", ShaderType::Fragment);

	render.getRenderPipeline()->addShader(vShader);
	render.getRenderPipeline()->addShader(fShader);

	render.getRenderPipeline()->create(*render.getDevice());

	while (!glfwWindowShouldClose(window)) {
		render.drawFrame();
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
}