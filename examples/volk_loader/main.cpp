/* Set platform defines at build time for volk to pick up. */
#if defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined(__linux__) || defined(__unix__)
#define VK_USE_PLATFORM_XLIB_KHR
#elif defined(__APPLE__)
#define VK_USE_PLATFORM_MACOS_MVK
#else
#error "Platform not supported by this example."
#endif

#define VOLK_IMPLEMENTATION
#include "volk.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>

#include "stdio.h"
#include "stdlib.h"

int main()
{
    //===============
    // Volk
    //===============

    /* This won't compile if the appropriate Vulkan platform define isn't set. */
    void *ptr =
#if defined(_WIN32)
        &vkCreateWin32SurfaceKHR;
#elif defined(__linux__) || defined(__unix__)
        &vkCreateXlibSurfaceKHR;
#elif defined(__APPLE__)
        &vkCreateMacOSSurfaceMVK;
#else
        /* Platform not recogized for testing. */
        NULL;
#endif

    /* Try to initialize volk. This might not work on CI builds, but the
     * above should have compiled at least. */
    VkResult r = volkInitialize();
    if (r != VK_SUCCESS)
    {
        printf("volkInitialize failed!\n");
        return EXIT_FAILURE;
    }

    uint32_t version = volkGetInstanceVersion();
    printf("Vulkan version %d.%d.%d initialized.\n",
           VK_VERSION_MAJOR(version),
           VK_VERSION_MINOR(version),
           VK_VERSION_PATCH(version));

    //===============
    // GLFW
    //===============
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow *window = glfwCreateWindow(800, 600, "Test Window", nullptr, nullptr);

    // vulkan test
    uint32_t extensionCount = 0; // Vulkan specifically uses this typedef
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    printf("Extension count: %i\n", extensionCount);

    // glm test
    glm::mat4 testMatrix(1.0f);
    glm::vec4 testVector(1.0f);
    auto test_result = testMatrix * testVector;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}