#include <stdio.h>
#include <stdlib.h>
#include "vulkan_renderer.h"
#include "log_assert.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

GLFWwindow *window;

void init_window(const char *name = "Test Window", const int width = 800, const int height = 600)
{
    if (!glfwInit())
        ERR_EXIT("Cannot initialize GLFW.\nExiting ...\n", "init_window");

    if (!glfwVulkanSupported())
        ERR_EXIT("GLFW failed to find the Vulkan loader.\nExiting... \n", "init_window");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // disable resize for the moment

    window = glfwCreateWindow(width, height, name, NULL, NULL);
    if (!window)
    {
        // It didn't work, so try to give a useful error:
        ERR_EXIT("Cannot create a window in which to draw!\n", "init_window");
    }
}

int main()
{
    init_window();

    if (init_renderer(window) == EXIT_FAILURE)
        return EXIT_FAILURE;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    cleanup_renderer();

    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}