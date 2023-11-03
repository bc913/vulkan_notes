#define GLAD_VULKAN_IMPLEMENTATION
#include <glad/vulkan.h>
// No need to include <vulkan/vulkan.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ----------------
// MACROS
// ----------------
#if _MSC_VER
// TODO: This works for now
// #include <intrin.h>
#define DEBUG_BREAK() __debugbreak()
#else
// #define DEBUG_BREAK() __builtin_trap()
#define DEBUG_BREAK() raise(SIGTRAP)
#endif

#define DEMO_TEXTURE_COUNT 1
#define APP_SHORT_NAME "tri_glad"
#define APP_LONG_NAME "The glad powered Vulkan Triangle Demo Program"
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#if defined(NDEBUG) && defined(__GNUC__)
#define U_ASSERT_ONLY __attribute__((unused))
#else
#define U_ASSERT_ONLY
#endif

#define ERR_EXIT(err_msg, err_class) \
    do                               \
    {                                \
        printf(err_msg);             \
        fflush(stdout);              \
        exit(1);                     \
    } while (0)

// ----------------
// Debug
// ----------------
VKAPI_ATTR VkBool32 VKAPI_CALL
BreakCallback(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType,
              uint64_t srcObject, size_t location, int32_t msgCode,
              const char *pLayerPrefix, const char *pMsg, void *pUserData)
{
    DEBUG_BREAK();
    return false;
}
static int validation_error = 0;
VKAPI_ATTR VkBool32 VKAPI_CALL dbgFunc(VkFlags msgFlags,
                                       VkDebugReportObjectTypeEXT objType,
                                       uint64_t srcObject, size_t location,
                                       int32_t msgCode,
                                       const char *pLayerPrefix,
                                       const char *pMsg, void *pUserData)
{
    char *message = (char *)malloc(strlen(pMsg) + 100);

    assert(message);

    validation_error = 1;

    if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
    {
        sprintf(message, "ERROR: [%s] Code %d : %s", pLayerPrefix, msgCode,
                pMsg);
    }
    else if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
    {
        sprintf(message, "WARNING: [%s] Code %d : %s", pLayerPrefix, msgCode,
                pMsg);
    }
    else
    {
        return false;
    }

    printf("%s\n", message);
    fflush(stdout);
    free(message);

    /*
     * false indicates that layer should not bail-out of an
     * API call that had validation failures. This may mean that the
     * app dies inside the driver due to invalid parameter(s).
     * That's what would happen without validation layers, so we'll
     * keep that behavior here.
     */
    return false;
}

// ----------------
// DEFINITIONS
// ----------------
struct texture_object
{
    VkSampler sampler;

    VkImage image;
    VkImageLayout imageLayout;

    VkDeviceMemory mem;
    VkImageView view;
    int32_t tex_width, tex_height;
};

typedef struct
{
    VkImage image;
    VkCommandBuffer cmd;
    VkImageView view;
} SwapchainBuffers;

struct demo
{
    GLFWwindow *window;
    VkSurfaceKHR surface;
    bool use_staging_buffer;

    VkInstance inst;
    VkPhysicalDevice gpu;
    VkDevice device;
    VkQueue queue;
    VkPhysicalDeviceProperties gpu_props;
    VkPhysicalDeviceFeatures gpu_features;
    VkQueueFamilyProperties *queue_props;
    uint32_t graphics_queue_node_index;

    uint32_t enabled_extension_count;
    uint32_t enabled_layer_count;
    const char *extension_names[64];
    const char *enabled_layers[64];

    int width, height;
    VkFormat format;
    VkColorSpaceKHR color_space;

    uint32_t swapchainImageCount;
    VkSwapchainKHR swapchain;
    SwapchainBuffers *buffers;

    VkCommandPool cmd_pool;

    struct
    {
        VkFormat format;

        VkImage image;
        VkDeviceMemory mem;
        VkImageView view;
    } depth;

    struct texture_object textures[DEMO_TEXTURE_COUNT];

    struct
    {
        VkBuffer buf;
        VkDeviceMemory mem;

        VkPipelineVertexInputStateCreateInfo vi;
        VkVertexInputBindingDescription vi_bindings[1];
        VkVertexInputAttributeDescription vi_attrs[2];
    } vertices;

    VkCommandBuffer setup_cmd; // Command Buffer for initialization commands
    VkCommandBuffer draw_cmd;  // Command Buffer for drawing commands
    VkPipelineLayout pipeline_layout;
    VkDescriptorSetLayout desc_layout;
    VkPipelineCache pipelineCache;
    VkRenderPass render_pass;
    VkPipeline pipeline;

    VkShaderModule vert_shader_module;
    VkShaderModule frag_shader_module;

    VkDescriptorPool desc_pool;
    VkDescriptorSet desc_set;

    VkFramebuffer *framebuffers;

    VkPhysicalDeviceMemoryProperties memory_properties;

    int32_t curFrame;
    int32_t frameCount;
    bool validate;
    bool use_break;
    PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback;
    PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback;
    VkDebugReportCallbackEXT msg_callback;
    PFN_vkDebugReportMessageEXT DebugReportMessage;

    float depthStencil;
    float depthIncrement;

    uint32_t current_buffer;
    uint32_t queue_count;
};

// ----------------
// CALLBACKS
// ----------------
static void error_callback(int error, const char *desc)
{
    printf("GLFW error: %s\n", desc);
    fflush(stdout);
}

static void init_connection()
{
    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
    {
        printf("Cannot initialize GLFW.\nExiting ...\n");
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    if (!glfwVulkanSupported())
    {
        printf("GLFW failed to find the Vulkan loader.\nExiting... \n");
        fflush(stdout);
        exit(EXIT_FAILURE);
    }
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action,
                         int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

static void demo_draw(struct demo *demo);
static void refresh_callback(GLFWwindow *window)
{
    struct demo *demo = (struct demo *)(glfwGetWindowUserPointer(window));
    demo_draw(demo);
}
static void demo_resize(struct demo *demo);
static void resize_callback(GLFWwindow *window, int width, int height)
{
    struct demo *demo = (struct demo *)(glfwGetWindowUserPointer(window));
    demo->width = width;
    demo->height = height;
    demo_resize(demo);
}

// ----------------
// CHECKS
// ----------------
static VkBool32 demo_check_layers(uint32_t check_count,
                                  const char **check_names,
                                  uint32_t layer_count,
                                  VkLayerProperties *layers)
{
    uint32_t i, j;
    for (i = 0; i < check_count; i++)
    {
        VkBool32 found = 0;
        for (j = 0; j < layer_count; j++)
        {
            if (!strcmp(check_names[i], layers[j].layerName))
            {
                found = 1;
                break;
            }
        }
        if (!found)
        {
            fprintf(stderr, "Cannot find layer: %s\n", check_names[i]);
            return 0;
        }
    }
    return 1;
}

// ----------------
// FUNCTIONS
// ----------------

static void demo_flush_init_cmd(struct demo *demo)
{
    VkResult U_ASSERT_ONLY err;

    if (demo->setup_cmd == VK_NULL_HANDLE)
        return;

    err = vkEndCommandBuffer(demo->setup_cmd);
    assert(!err);

    const VkCommandBuffer cmd_bufs[] = {demo->setup_cmd};
    VkFence nullFence = {VK_NULL_HANDLE};
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = NULL;
    submit_info.pWaitDstStageMask = NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = cmd_bufs;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = NULL;

    err = vkQueueSubmit(demo->queue, 1, &submit_info, nullFence);
    assert(!err);

    err = vkQueueWaitIdle(demo->queue);
    assert(!err);

    vkFreeCommandBuffers(demo->device, demo->cmd_pool, 1, cmd_bufs);
    demo->setup_cmd = VK_NULL_HANDLE;
}

typedef union my_u
{
    int x[3];
    float y[3];
} my_u;

static void demo_draw_build_cmd(struct demo *demo)
{
    VkCommandBufferBeginInfo cmd_buf_info = {};
    cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_buf_info.pNext = NULL;
    cmd_buf_info.flags = 0;
    cmd_buf_info.pInheritanceInfo = NULL;

    // const VkRenderPassBeginInfo rp_begin = {
    //     .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    //     .pNext = NULL,
    //     .renderPass = demo->render_pass,
    //     .framebuffer = demo->framebuffers[demo->current_buffer],
    //     .renderArea.offset.x = 0,
    //     .renderArea.offset.y = 0,
    //     .renderArea.extent.width = demo->width,
    //     .renderArea.extent.height = demo->height,
    //     .clearValueCount = 2,
    //     .pClearValues = clear_values,
    // };
    // VkResult U_ASSERT_ONLY err;

    // err = vkBeginCommandBuffer(demo->draw_cmd, &cmd_buf_info);
    // assert(!err);

    // // We can use LAYOUT_UNDEFINED as a wildcard here because we don't care
    // what
    // // happens to the previous contents of the image
    // VkImageMemoryBarrier image_memory_barrier = {
    //     .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    //     .pNext = NULL,
    //     .srcAccessMask = 0,
    //     .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    //     .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    //     .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    //     .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    //     .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    //     .image = demo->buffers[demo->current_buffer].image,
    //     .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    // vkCmdPipelineBarrier(demo->draw_cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    //                      VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0,
    //                      NULL, 1, &image_memory_barrier);
    // vkCmdBeginRenderPass(demo->draw_cmd, &rp_begin,
    // VK_SUBPASS_CONTENTS_INLINE); vkCmdBindPipeline(demo->draw_cmd,
    // VK_PIPELINE_BIND_POINT_GRAPHICS,
    //                   demo->pipeline);
    // vkCmdBindDescriptorSets(demo->draw_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
    //                         demo->pipeline_layout, 0, 1, &demo->desc_set, 0,
    //                         NULL);

    // VkViewport viewport;
    // memset(&viewport, 0, sizeof(viewport));
    // viewport.height = (float)demo->height;
    // viewport.width = (float)demo->width;
    // viewport.minDepth = (float)0.0f;
    // viewport.maxDepth = (float)1.0f;
    // vkCmdSetViewport(demo->draw_cmd, 0, 1, &viewport);

    // VkRect2D scissor;
    // memset(&scissor, 0, sizeof(scissor));
    // scissor.extent.width = demo->width;
    // scissor.extent.height = demo->height;
    // scissor.offset.x = 0;
    // scissor.offset.y = 0;
    // vkCmdSetScissor(demo->draw_cmd, 0, 1, &scissor);

    // VkDeviceSize offsets[1] = {0};
    // vkCmdBindVertexBuffers(demo->draw_cmd, VERTEX_BUFFER_BIND_ID, 1,
    //                        &demo->vertices.buf, offsets);

    // vkCmdDraw(demo->draw_cmd, 3, 1, 0, 0);
    // vkCmdEndRenderPass(demo->draw_cmd);

    // VkImageMemoryBarrier prePresentBarrier = {
    //     .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    //     .pNext = NULL,
    //     .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    //     .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
    //     .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    //     .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    //     .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    //     .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    //     .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    // prePresentBarrier.image = demo->buffers[demo->current_buffer].image;
    // VkImageMemoryBarrier* pmemory_barrier = &prePresentBarrier;
    // vkCmdPipelineBarrier(demo->draw_cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    //                      VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0,
    //                      NULL, 1, pmemory_barrier);

    // err = vkEndCommandBuffer(demo->draw_cmd);
    // assert(!err);
}

static void demo_draw(struct demo *demo)
{
    VkResult U_ASSERT_ONLY err;
    VkSemaphore imageAcquiredSemaphore, drawCompleteSemaphore;
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = NULL;
    semaphoreCreateInfo.flags = 0;

    err = vkCreateSemaphore(demo->device, &semaphoreCreateInfo, NULL,
                            &imageAcquiredSemaphore);
    assert(!err);

    err = vkCreateSemaphore(demo->device, &semaphoreCreateInfo, NULL,
                            &drawCompleteSemaphore);
    assert(!err);

    // Get the index of the next available swapchain image:
    err = vkAcquireNextImageKHR(demo->device, demo->swapchain, UINT64_MAX,
                                imageAcquiredSemaphore,
                                (VkFence)0, // TODO: Show use of fence
                                &demo->current_buffer);
    if (err == VK_ERROR_OUT_OF_DATE_KHR)
    {
        // demo->swapchain is out of date (e.g. the window was resized) and
        // must be recreated:
        demo_resize(demo);
        demo_draw(demo);
        vkDestroySemaphore(demo->device, imageAcquiredSemaphore, NULL);
        vkDestroySemaphore(demo->device, drawCompleteSemaphore, NULL);
        return;
    }
    else if (err == VK_SUBOPTIMAL_KHR)
    {
        // demo->swapchain is not as optimal as it could be, but the platform's
        // presentation engine will still present the image correctly.
    }
    else
    {
        assert(!err);
    }

    demo_flush_init_cmd(demo);

    // Wait for the present complete semaphore to be signaled to ensure
    // that the image won't be rendered to until the presentation
    // engine has fully released ownership to the application, and it is
    // okay to render to the image.

    demo_draw_build_cmd(demo);
    VkFence nullFence = VK_NULL_HANDLE;
    VkPipelineStageFlags pipe_stage_flags =
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &imageAcquiredSemaphore;
    submit_info.pWaitDstStageMask = &pipe_stage_flags;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &demo->draw_cmd;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &drawCompleteSemaphore;

    err = vkQueueSubmit(demo->queue, 1, &submit_info, nullFence);
    assert(!err);

    VkPresentInfoKHR present = {};
    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.pNext = NULL;
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &drawCompleteSemaphore;
    present.swapchainCount = 1;
    present.pSwapchains = &demo->swapchain;
    present.pImageIndices = &demo->current_buffer;

    err = vkQueuePresentKHR(demo->queue, &present);
    if (err == VK_ERROR_OUT_OF_DATE_KHR)
    {
        // demo->swapchain is out of date (e.g. the window was resized) and
        // must be recreated:
        demo_resize(demo);
    }
    else if (err == VK_SUBOPTIMAL_KHR)
    {
        // demo->swapchain is not as optimal as it could be, but the platform's
        // presentation engine will still present the image correctly.
    }
    else
    {
        assert(!err);
    }

    err = vkQueueWaitIdle(demo->queue);
    assert(err == VK_SUCCESS);

    vkDestroySemaphore(demo->device, imageAcquiredSemaphore, NULL);
    vkDestroySemaphore(demo->device, drawCompleteSemaphore, NULL);
}

static void demo_resize(struct demo *demo)
{
    uint32_t i;

    // In order to properly resize the window, we must re-create the swapchain
    // AND redo the command buffers, etc.
    //
    // First, perform part of the demo_cleanup() function:

    for (i = 0; i < demo->swapchainImageCount; i++)
    {
        vkDestroyFramebuffer(demo->device, demo->framebuffers[i], NULL);
    }
    free(demo->framebuffers);
    vkDestroyDescriptorPool(demo->device, demo->desc_pool, NULL);

    if (demo->setup_cmd)
    {
        vkFreeCommandBuffers(demo->device, demo->cmd_pool, 1, &demo->setup_cmd);
    }
    vkFreeCommandBuffers(demo->device, demo->cmd_pool, 1, &demo->draw_cmd);
    vkDestroyCommandPool(demo->device, demo->cmd_pool, NULL);

    vkDestroyPipeline(demo->device, demo->pipeline, NULL);
    vkDestroyRenderPass(demo->device, demo->render_pass, NULL);
    vkDestroyPipelineLayout(demo->device, demo->pipeline_layout, NULL);
    vkDestroyDescriptorSetLayout(demo->device, demo->desc_layout, NULL);

    vkDestroyBuffer(demo->device, demo->vertices.buf, NULL);
    vkFreeMemory(demo->device, demo->vertices.mem, NULL);

    for (i = 0; i < DEMO_TEXTURE_COUNT; i++)
    {
        vkDestroyImageView(demo->device, demo->textures[i].view, NULL);
        vkDestroyImage(demo->device, demo->textures[i].image, NULL);
        vkFreeMemory(demo->device, demo->textures[i].mem, NULL);
        vkDestroySampler(demo->device, demo->textures[i].sampler, NULL);
    }

    for (i = 0; i < demo->swapchainImageCount; i++)
    {
        vkDestroyImageView(demo->device, demo->buffers[i].view, NULL);
    }

    vkDestroyImageView(demo->device, demo->depth.view, NULL);
    vkDestroyImage(demo->device, demo->depth.image, NULL);
    vkFreeMemory(demo->device, demo->depth.mem, NULL);

    free(demo->buffers);

    // Second, re-perform the demo_prepare() function, which will re-create the
    // swapchain:
    // TODO: demo_prepare(demo);
}

static void init_vk(struct demo *demo)
{
    VkResult err;
    int glad_vk_version = 0;
    uint32_t i = 0;
    uint32_t required_extension_count = 0;
    uint32_t instance_layer_count = 0;
    uint32_t validation_layer_count = 0;

    const char **required_extensions = NULL;
    const char **instance_validation_layers = NULL;

    char *instance_validation_layers_alt1[] = {
        "VK_LAYER_LUNARG_standard_validation"};

    char *instance_validation_layers_alt2[] = {
        "VK_LAYER_GOOGLE_threading",
        "VK_LAYER_LUNARG_parameter_validation",
        "VK_LAYER_LUNARG_object_tracker",
        "VK_LAYER_LUNARG_image",
        "VK_LAYER_LUNARG_core_validation",
        "VK_LAYER_LUNARG_swapchain",
        "VK_LAYER_GOOGLE_unique_objects"};

    glad_vk_version = gladLoaderLoadVulkan(NULL, NULL, NULL);
    if (!glad_vk_version)
    {
        ERR_EXIT("Unable to load Vulkan symbols!\n", "gladLoad Failure");
    }

    /* Look for validation layers */
    VkBool32 validation_found = 0;
    if (demo->validate)
    {
        err = vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL);
        assert(!err);

        instance_validation_layers =
            (const char **)instance_validation_layers_alt1;
        if (instance_layer_count > 0)
        {
            VkLayerProperties *instance_layers = (VkLayerProperties *)(malloc(
                sizeof(VkLayerProperties) * instance_layer_count));
            err = vkEnumerateInstanceLayerProperties(&instance_layer_count,
                                                     instance_layers);
            assert(!err);

            validation_found =
                demo_check_layers(ARRAY_SIZE(instance_validation_layers_alt1),
                                  instance_validation_layers,
                                  instance_layer_count, instance_layers);
            if (validation_found)
            {
                demo->enabled_layer_count =
                    ARRAY_SIZE(instance_validation_layers_alt1);
                demo->enabled_layers[0] = "VK_LAYER_LUNARG_standard_validation";
                validation_layer_count = 1;
            }
            else
            {
                // use alternative set of validation layers
                instance_validation_layers =
                    (const char **)instance_validation_layers_alt2;
                demo->enabled_layer_count =
                    ARRAY_SIZE(instance_validation_layers_alt2);
                validation_found = demo_check_layers(
                    ARRAY_SIZE(instance_validation_layers_alt2),
                    instance_validation_layers, instance_layer_count,
                    instance_layers);
                validation_layer_count =
                    ARRAY_SIZE(instance_validation_layers_alt2);
                for (i = 0; i < validation_layer_count; i++)
                {
                    demo->enabled_layers[i] = instance_validation_layers[i];
                }
            }
            free(instance_layers);
        }

        if (!validation_found)
        {
            ERR_EXIT(
                "vkEnumerateInstanceLayerProperties failed to find "
                "required validation layer.\n\n"
                "Please look at the Getting Started guide for additional "
                "information.\n",
                "vkCreateInstance Failure");
        }
    }

    /* Look for instance extensions */
    required_extensions =
        glfwGetRequiredInstanceExtensions(&required_extension_count);
    if (!required_extensions)
    {
        ERR_EXIT(
            "glfwGetRequiredInstanceExtensions failed to find the "
            "platform surface extensions.\n\nDo you have a compatible "
            "Vulkan installable client driver (ICD) installed?\nPlease "
            "look at the Getting Started guide for additional "
            "information.\n",
            "vkCreateInstance Failure");
    }

    for (i = 0; i < required_extension_count; i++)
    {
        demo->extension_names[demo->enabled_extension_count++] =
            required_extensions[i];
        assert(demo->enabled_extension_count < 64);
    }

    if (GLAD_VK_EXT_debug_report && demo->validate)
    {
        demo->extension_names[demo->enabled_extension_count++] =
            VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
    }
    assert(demo->enabled_extension_count < 64);

    VkApplicationInfo app = {};
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pNext = NULL;
    app.pApplicationName = APP_SHORT_NAME;
    app.applicationVersion = 0;
    app.pEngineName = APP_SHORT_NAME;
    app.engineVersion = 0;
    app.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo inst_info = {};
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pNext = NULL;
    inst_info.pApplicationInfo = &app;
    inst_info.enabledLayerCount = demo->enabled_layer_count;
    inst_info.ppEnabledLayerNames =
        (const char *const *)instance_validation_layers;
    inst_info.enabledExtensionCount = demo->enabled_extension_count;
    inst_info.ppEnabledExtensionNames =
        (const char *const *)demo->extension_names;

    uint32_t gpu_count;

    err = vkCreateInstance(&inst_info, NULL, &demo->inst);
    if (err == VK_ERROR_INCOMPATIBLE_DRIVER)
    {
        ERR_EXIT(
            "Cannot find a compatible Vulkan installable client driver "
            "(ICD).\n\nPlease look at the Getting Started guide for "
            "additional information.\n",
            "vkCreateInstance Failure");
    }
    else if (err == VK_ERROR_EXTENSION_NOT_PRESENT)
    {
        ERR_EXIT(
            "Cannot find a specified extension library"
            ".\nMake sure your layers path is set appropriately\n",
            "vkCreateInstance Failure");
    }
    else if (err)
    {
        ERR_EXIT(
            "vkCreateInstance failed.\n\nDo you have a compatible Vulkan "
            "installable client driver (ICD) installed?\nPlease look at "
            "the Getting Started guide for additional information.\n",
            "vkCreateInstance Failure");
    }

    /* Make initial call to query gpu_count, then second call for gpu info*/
    err = vkEnumeratePhysicalDevices(demo->inst, &gpu_count, NULL);
    assert(!err && gpu_count > 0);

    if (gpu_count > 0)
    {
        VkPhysicalDevice *physical_devices =
            (VkPhysicalDevice *)(malloc(sizeof(VkPhysicalDevice) * gpu_count));
        err = vkEnumeratePhysicalDevices(demo->inst, &gpu_count,
                                         physical_devices);
        assert(!err);
        /* For tri demo we just grab the first physical device */
        demo->gpu = physical_devices[0];
        free(physical_devices);
    }
    else
    {
        ERR_EXIT(
            "vkEnumeratePhysicalDevices reported zero accessible devices."
            "\n\nDo you have a compatible Vulkan installable client"
            " driver (ICD) installed?\nPlease look at the Getting Started"
            " guide for additional information.\n",
            "vkEnumeratePhysicalDevices Failure");
    }

    /* Look for device extensions */
    /* Re-Load glad here to load instance pointers and to populate available
     * extensions */
    glad_vk_version = gladLoaderLoadVulkan(demo->inst, demo->gpu, NULL);
    if (!glad_vk_version)
    {
        ERR_EXIT("Unable to re-load Vulkan symbols with instance!\n",
                 "gladLoad Failure");
    }

    VkBool32 swapchainExtFound = 0;
    demo->enabled_extension_count = 0;

    if (GLAD_VK_KHR_swapchain)
    {
        swapchainExtFound = 1;
        demo->extension_names[demo->enabled_extension_count++] =
            VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    }
    assert(demo->enabled_extension_count < 64);

    if (!swapchainExtFound)
    {
        ERR_EXIT(
            "vkEnumerateDeviceExtensionProperties failed to find "
            "the " VK_KHR_SWAPCHAIN_EXTENSION_NAME
            " extension.\n\nDo you have a compatible "
            "Vulkan installable client driver (ICD) installed?\nPlease "
            "look at the Getting Started guide for additional "
            "information.\n",
            "vkCreateInstance Failure");
    }

    if (demo->validate)
    {
        demo->CreateDebugReportCallback =
            (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
                demo->inst, "vkCreateDebugReportCallbackEXT");
        demo->DestroyDebugReportCallback =
            (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(
                demo->inst, "vkDestroyDebugReportCallbackEXT");
        if (!demo->CreateDebugReportCallback)
        {
            ERR_EXIT(
                "GetProcAddr: Unable to find vkCreateDebugReportCallbackEXT\n",
                "vkGetProcAddr Failure");
        }
        if (!demo->DestroyDebugReportCallback)
        {
            ERR_EXIT(
                "GetProcAddr: Unable to find vkDestroyDebugReportCallbackEXT\n",
                "vkGetProcAddr Failure");
        }
        demo->DebugReportMessage =
            (PFN_vkDebugReportMessageEXT)vkGetInstanceProcAddr(
                demo->inst, "vkDebugReportMessageEXT");
        if (!demo->DebugReportMessage)
        {
            ERR_EXIT("GetProcAddr: Unable to find vkDebugReportMessageEXT\n",
                     "vkGetProcAddr Failure");
        }

        VkDebugReportCallbackCreateInfoEXT dbgCreateInfo;
        dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        dbgCreateInfo.flags =
            VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
        dbgCreateInfo.pfnCallback = demo->use_break ? BreakCallback : dbgFunc;
        dbgCreateInfo.pUserData = demo;
        dbgCreateInfo.pNext = NULL;
        err = demo->CreateDebugReportCallback(demo->inst, &dbgCreateInfo, NULL,
                                              &demo->msg_callback);
        switch (err)
        {
        case VK_SUCCESS:
            break;
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            ERR_EXIT("CreateDebugReportCallback: out of host memory\n",
                     "CreateDebugReportCallback Failure");
            break;
        default:
            ERR_EXIT("CreateDebugReportCallback: unknown failure\n",
                     "CreateDebugReportCallback Failure");
            break;
        }
    }

    vkGetPhysicalDeviceProperties(demo->gpu, &demo->gpu_props);

    // Query with NULL data to get count
    vkGetPhysicalDeviceQueueFamilyProperties(demo->gpu, &demo->queue_count,
                                             NULL);

    demo->queue_props = (VkQueueFamilyProperties *)malloc(
        demo->queue_count * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(demo->gpu, &demo->queue_count,
                                             demo->queue_props);
    assert(demo->queue_count >= 1);

    vkGetPhysicalDeviceFeatures(demo->gpu, &demo->gpu_features);

    // Graphics queue and MemMgr queue can be separate.
    // TODO: Add support for separate queues, including synchronization,
    //       and appropriate tracking for QueueSubmit
}

static void init(struct demo *demo)
{
    int i;
    memset(demo, 0, sizeof(*demo));
    demo->frameCount = INT32_MAX;
    demo->use_staging_buffer = true;
    demo->use_break = true;
    demo->validate = true;
    init_connection();
    init_vk(demo);
}

static void create_window(struct demo *demo)
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    demo->window =
        glfwCreateWindow(demo->width, demo->height, APP_LONG_NAME, NULL, NULL);
    if (!demo->window)
    {
        // It didn't work, so try to give a useful error:
        printf("Cannot create a window in which to draw!\n");
        fflush(stdout);
        exit(1);
    }

    glfwSetWindowUserPointer(demo->window, demo);
    glfwSetWindowRefreshCallback(demo->window, refresh_callback);
    glfwSetFramebufferSizeCallback(demo->window, resize_callback);
    glfwSetKeyCallback(demo->window, key_callback);
}
int main()
{
    struct demo demo;
    init(&demo);
    return EXIT_SUCCESS;
}

// int main() {
//     glfwInit();

//     glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
//     GLFWwindow* window =
//         glfwCreateWindow(800, 600, "Test Window", nullptr, nullptr);

//     // vulkan test
//     uint32_t extensionCount = 0;  // Vulkan specifically uses this typedef
//     vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
//     nullptr); printf("Extension count: %i\n", extensionCount);

//     // glm test
//     glm::mat4 testMatrix(1.0f);
//     glm::vec4 testVector(1.0f);
//     auto test_result = testMatrix * testVector;

//     while (!glfwWindowShouldClose(window)) {
//         glfwPollEvents();
//     }

//     glfwDestroyWindow(window);
//     glfwTerminate();
//     std::cout << "Hello Vulkan!\n";
//     return 0;
// }