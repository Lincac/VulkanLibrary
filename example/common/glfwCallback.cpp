#include "glfwCallback.h"

void WindowSizeCallback(GLFWwindow *window, int width, int height)
{
    glfwSetWindowSize(window, width, height); 
}
