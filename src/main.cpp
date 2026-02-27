#include "Renderer.h"
#include "vkHelper.h"

#include <iostream>

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Vulkan", nullptr, nullptr);

    Renderer renderer;

    try
    {
        renderer.initialize(window);

        const std::vector<Vertex> vertices = {
            { { 0.0f, -0.5f } },
            { { 0.5f, 0.5f } },
            { { -0.5f, 0.5f } },
        };
        renderer.initializeVertexBuffer(vertices);

        const auto vertCode = readSpvFile("../shaders/bin/triangle.vert.spv");
        const auto fragCode = readSpvFile("../shaders/bin/triangle.frag.spv");

        VkDevice device = renderer.getContext().getDevice();
        VkShaderModule vertModule = Pipeline::createShaderModule(device, vertCode);
        VkShaderModule fragModule = Pipeline::createShaderModule(device, fragCode);

        renderer.initializeGraphicsPipeline(vertModule, fragModule);

        renderer.run();
        renderer.cleanup();
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << '\n';
        renderer.cleanup();
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}
