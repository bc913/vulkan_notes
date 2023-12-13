#include <stdio.h>
#include <stdlib.h>
#include "vulkan_renderer.h"
#include "log_assert.h"
#include "defines.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

GLFWwindow *window;
typedef struct application_state
{
    b8 is_running;
    b8 is_suspended;
    i16 width, height;
} application_state;
static application_state *app_state;

void init_window(const char *name, const int width, const int height)
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

int init()
{
    app_state = (application_state *)(malloc(sizeof(application_state)));
    app_state->width = 800;
    app_state->height = 600;
    init_window("Test Window", app_state->width, app_state->height);
    if (init_renderer(window, app_state->width, app_state->height) == EXIT_FAILURE)
        return EXIT_FAILURE;
}

void run()
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }
}

void shutdown()
{
    cleanup_renderer();
    glfwDestroyWindow(window);
    glfwTerminate();

    free(app_state);
}

int main()
{
    init();
    run();
    shutdown();
    return EXIT_SUCCESS;
}