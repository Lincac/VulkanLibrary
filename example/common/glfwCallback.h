#include <iostream>

#include <glfw/glfw3.h>

void WindowSizeCallback(GLFWwindow* window, int width, int height)
{
    glfwSetWindowSize(window, width, height); 
}