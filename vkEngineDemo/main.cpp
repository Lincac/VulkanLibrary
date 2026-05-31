#include "vkEngineSwapChain.h"
#include "vkEngineCommandPool.h"

#include <iostream>

int main(){
    glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	auto window = glfwCreateWindow(800, 600, "Vulkan Demo", nullptr, nullptr);

    vkEngine engine("Vulkan Demo", true);

    if (glfwCreateWindowSurface(engine.getInstance(), window, nullptr, &engine.getSurfaceKHR()) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }

    vkEnginePhysicalDevice physicalDevice(engine);
    vkEngineLogicalDevice logicalDevice(physicalDevice);
    vkEngineSwapChain swapChain(logicalDevice, window);
    vkEngineCommandPool commandPool(logicalDevice);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}