#include "vkContext.h"

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Vulkan", nullptr, nullptr);

    vkContext context;
    context.initialize(window);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    context.cleanup();
    glfwDestroyWindow(window);
    glfwTerminate();
}
