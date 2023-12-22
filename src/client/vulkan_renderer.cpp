#include "vulkan_renderer.h"
#include "log_assert.h"
#include "utils.h"
#include "file_system.h"
#include "vulkan_types.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>

#define OBJECT_SHADER_STAGE_COUNT 2
char stage_type_strs[OBJECT_SHADER_STAGE_COUNT][5] = {"vert", "frag"};
VkShaderStageFlagBits stage_types[OBJECT_SHADER_STAGE_COUNT] = {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};

const char *vulkan_result_string(VkResult result, b8 get_extended)
{
    // From: https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkResult.html
    // Success Codes
    switch (result)
    {
    default:
    case VK_SUCCESS:
        return !get_extended ? "VK_SUCCESS" : "VK_SUCCESS Command successfully completed";
    case VK_NOT_READY:
        return !get_extended ? "VK_NOT_READY" : "VK_NOT_READY A fence or query has not yet completed";
    case VK_TIMEOUT:
        return !get_extended ? "VK_TIMEOUT" : "VK_TIMEOUT A wait operation has not completed in the specified time";
    case VK_EVENT_SET:
        return !get_extended ? "VK_EVENT_SET" : "VK_EVENT_SET An event is signaled";
    case VK_EVENT_RESET:
        return !get_extended ? "VK_EVENT_RESET" : "VK_EVENT_RESET An event is unsignaled";
    case VK_INCOMPLETE:
        return !get_extended ? "VK_INCOMPLETE" : "VK_INCOMPLETE A return array was too small for the result";
    case VK_SUBOPTIMAL_KHR:
        return !get_extended ? "VK_SUBOPTIMAL_KHR" : "VK_SUBOPTIMAL_KHR A swapchain no longer matches the surface properties exactly, but can still be used to present to the surface successfully.";
    case VK_THREAD_IDLE_KHR:
        return !get_extended ? "VK_THREAD_IDLE_KHR" : "VK_THREAD_IDLE_KHR A deferred operation is not complete but there is currently no work for this thread to do at the time of this call.";
    case VK_THREAD_DONE_KHR:
        return !get_extended ? "VK_THREAD_DONE_KHR" : "VK_THREAD_DONE_KHR A deferred operation is not complete but there is no work remaining to assign to additional threads.";
    case VK_OPERATION_DEFERRED_KHR:
        return !get_extended ? "VK_OPERATION_DEFERRED_KHR" : "VK_OPERATION_DEFERRED_KHR A deferred operation was requested and at least some of the work was deferred.";
    case VK_OPERATION_NOT_DEFERRED_KHR:
        return !get_extended ? "VK_OPERATION_NOT_DEFERRED_KHR" : "VK_OPERATION_NOT_DEFERRED_KHR A deferred operation was requested and no operations were deferred.";
    case VK_PIPELINE_COMPILE_REQUIRED_EXT:
        return !get_extended ? "VK_PIPELINE_COMPILE_REQUIRED_EXT" : "VK_PIPELINE_COMPILE_REQUIRED_EXT A requested pipeline creation would have required compilation, but the application requested compilation to not be performed.";

    // Error codes
    case VK_ERROR_OUT_OF_HOST_MEMORY:
        return !get_extended ? "VK_ERROR_OUT_OF_HOST_MEMORY" : "VK_ERROR_OUT_OF_HOST_MEMORY A host memory allocation has failed.";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        return !get_extended ? "VK_ERROR_OUT_OF_DEVICE_MEMORY" : "VK_ERROR_OUT_OF_DEVICE_MEMORY A device memory allocation has failed.";
    case VK_ERROR_INITIALIZATION_FAILED:
        return !get_extended ? "VK_ERROR_INITIALIZATION_FAILED" : "VK_ERROR_INITIALIZATION_FAILED Initialization of an object could not be completed for implementation-specific reasons.";
    case VK_ERROR_DEVICE_LOST:
        return !get_extended ? "VK_ERROR_DEVICE_LOST" : "VK_ERROR_DEVICE_LOST The logical or physical device has been lost. See Lost Device";
    case VK_ERROR_MEMORY_MAP_FAILED:
        return !get_extended ? "VK_ERROR_MEMORY_MAP_FAILED" : "VK_ERROR_MEMORY_MAP_FAILED Mapping of a memory object has failed.";
    case VK_ERROR_LAYER_NOT_PRESENT:
        return !get_extended ? "VK_ERROR_LAYER_NOT_PRESENT" : "VK_ERROR_LAYER_NOT_PRESENT A requested layer is not present or could not be loaded.";
    case VK_ERROR_EXTENSION_NOT_PRESENT:
        return !get_extended ? "VK_ERROR_EXTENSION_NOT_PRESENT" : "VK_ERROR_EXTENSION_NOT_PRESENT A requested extension is not supported.";
    case VK_ERROR_FEATURE_NOT_PRESENT:
        return !get_extended ? "VK_ERROR_FEATURE_NOT_PRESENT" : "VK_ERROR_FEATURE_NOT_PRESENT A requested feature is not supported.";
    case VK_ERROR_INCOMPATIBLE_DRIVER:
        return !get_extended ? "VK_ERROR_INCOMPATIBLE_DRIVER" : "VK_ERROR_INCOMPATIBLE_DRIVER The requested version of Vulkan is not supported by the driver or is otherwise incompatible for implementation-specific reasons.";
    case VK_ERROR_TOO_MANY_OBJECTS:
        return !get_extended ? "VK_ERROR_TOO_MANY_OBJECTS" : "VK_ERROR_TOO_MANY_OBJECTS Too many objects of the type have already been created.";
    case VK_ERROR_FORMAT_NOT_SUPPORTED:
        return !get_extended ? "VK_ERROR_FORMAT_NOT_SUPPORTED" : "VK_ERROR_FORMAT_NOT_SUPPORTED A requested format is not supported on this device.";
    case VK_ERROR_FRAGMENTED_POOL:
        return !get_extended ? "VK_ERROR_FRAGMENTED_POOL" : "VK_ERROR_FRAGMENTED_POOL A pool allocation has failed due to fragmentation of the pool’s memory. This must only be returned if no attempt to allocate host or device memory was made to accommodate the new allocation. This should be returned in preference to VK_ERROR_OUT_OF_POOL_MEMORY, but only if the implementation is certain that the pool allocation failure was due to fragmentation.";
    case VK_ERROR_SURFACE_LOST_KHR:
        return !get_extended ? "VK_ERROR_SURFACE_LOST_KHR" : "VK_ERROR_SURFACE_LOST_KHR A surface is no longer available.";
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
        return !get_extended ? "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR" : "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR The requested window is already in use by Vulkan or another API in a manner which prevents it from being used again.";
    case VK_ERROR_OUT_OF_DATE_KHR:
        return !get_extended ? "VK_ERROR_OUT_OF_DATE_KHR" : "VK_ERROR_OUT_OF_DATE_KHR A surface has changed in such a way that it is no longer compatible with the swapchain, and further presentation requests using the swapchain will fail. Applications must query the new surface properties and recreate their swapchain if they wish to continue presenting to the surface.";
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
        return !get_extended ? "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR" : "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR The display used by a swapchain does not use the same presentable image layout, or is incompatible in a way that prevents sharing an image.";
    case VK_ERROR_INVALID_SHADER_NV:
        return !get_extended ? "VK_ERROR_INVALID_SHADER_NV" : "VK_ERROR_INVALID_SHADER_NV One or more shaders failed to compile or link. More details are reported back to the application via VK_EXT_debug_report if enabled.";
    case VK_ERROR_OUT_OF_POOL_MEMORY:
        return !get_extended ? "VK_ERROR_OUT_OF_POOL_MEMORY" : "VK_ERROR_OUT_OF_POOL_MEMORY A pool memory allocation has failed. This must only be returned if no attempt to allocate host or device memory was made to accommodate the new allocation. If the failure was definitely due to fragmentation of the pool, VK_ERROR_FRAGMENTED_POOL should be returned instead.";
    case VK_ERROR_INVALID_EXTERNAL_HANDLE:
        return !get_extended ? "VK_ERROR_INVALID_EXTERNAL_HANDLE" : "VK_ERROR_INVALID_EXTERNAL_HANDLE An external handle is not a valid handle of the specified type.";
    case VK_ERROR_FRAGMENTATION:
        return !get_extended ? "VK_ERROR_FRAGMENTATION" : "VK_ERROR_FRAGMENTATION A descriptor pool creation has failed due to fragmentation.";
    case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT:
        return !get_extended ? "VK_ERROR_INVALID_DEVICE_ADDRESS_EXT" : "VK_ERROR_INVALID_DEVICE_ADDRESS_EXT A buffer creation failed because the requested address is not available.";
    // NOTE: Same as above
    // case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
    //    return !get_extended ? "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS" :"VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS A buffer creation or memory allocation failed because the requested address is not available. A shader group handle assignment failed because the requested shader group handle information is no longer valid.";
    case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
        return !get_extended ? "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT" : "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT An operation on a swapchain created with VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT failed as it did not have exlusive full-screen access. This may occur due to implementation-dependent reasons, outside of the application’s control.";
    case VK_ERROR_UNKNOWN:
        return !get_extended ? "VK_ERROR_UNKNOWN" : "VK_ERROR_UNKNOWN An unknown error has occurred; either the application has provided invalid input, or an implementation failure has occurred.";
    }
}

b8 vulkan_result_is_success(VkResult result)
{
    // From: https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkResult.html
    switch (result)
    {
        // Success Codes
    default:
    case VK_SUCCESS:
    case VK_NOT_READY:
    case VK_TIMEOUT:
    case VK_EVENT_SET:
    case VK_EVENT_RESET:
    case VK_INCOMPLETE:
    case VK_SUBOPTIMAL_KHR:
    case VK_THREAD_IDLE_KHR:
    case VK_THREAD_DONE_KHR:
    case VK_OPERATION_DEFERRED_KHR:
    case VK_OPERATION_NOT_DEFERRED_KHR:
    case VK_PIPELINE_COMPILE_REQUIRED_EXT:
        return true;

    // Error codes
    case VK_ERROR_OUT_OF_HOST_MEMORY:
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
    case VK_ERROR_INITIALIZATION_FAILED:
    case VK_ERROR_DEVICE_LOST:
    case VK_ERROR_MEMORY_MAP_FAILED:
    case VK_ERROR_LAYER_NOT_PRESENT:
    case VK_ERROR_EXTENSION_NOT_PRESENT:
    case VK_ERROR_FEATURE_NOT_PRESENT:
    case VK_ERROR_INCOMPATIBLE_DRIVER:
    case VK_ERROR_TOO_MANY_OBJECTS:
    case VK_ERROR_FORMAT_NOT_SUPPORTED:
    case VK_ERROR_FRAGMENTED_POOL:
    case VK_ERROR_SURFACE_LOST_KHR:
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
    case VK_ERROR_OUT_OF_DATE_KHR:
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
    case VK_ERROR_INVALID_SHADER_NV:
    case VK_ERROR_OUT_OF_POOL_MEMORY:
    case VK_ERROR_INVALID_EXTERNAL_HANDLE:
    case VK_ERROR_FRAGMENTATION:
    case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT:
    // NOTE: Same as above
    // case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
    case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
    case VK_ERROR_UNKNOWN:
        return false;
    }
}

//--------------
// Definitions
//--------------

static vulkan_context context;
static u32 cached_framebuffer_width = 0;
static u32 cached_framebuffer_height = 0;
static VkPipeline graphicsPipeline;
static VkPipelineLayout pipelineLayout;

// Extensions
static const char *requested_device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
static uint32_t requested_device_ext_count = 1;

// Debug
static VkDebugUtilsMessengerEXT debugMessenger;
static PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallback;
static PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallback;

#ifdef DEBUG_MODE
static VkBool32 enable_validation_layers = VK_TRUE;
#else
static VkBool32 enable_validation_layers = VK_FALSE;
#endif

//--------------
// Utils
//--------------
const char **append_strs_to_str_arr(const char **const source, size_t size, const char **const append, size_t append_size)
{
    if (source == NULL || !size || append == NULL || !append_size)
        return NULL;

    size_t total_size = size + append_size;
    const char **dest = (const char **)(malloc(sizeof(char *) * total_size));
    size_t i;
    for (i = 0; i < total_size; ++i)
    {
        if (i < size)
        {
            dest[i] = strdup(source[i]);
            // or
            // size_t len = strlen(source[i]);
            // dest[i] = (char *)(malloc(sizeof(char) * len + 1));
            // dest[i] = strcpy(dest[i], source[i]);
        }
        else
        {
            dest[i] = strdup(append[i - size]);
            // or
            // size_t len = strlen(append[i]);
            // dest[i] = (char *)(malloc(sizeof(char) * len + 1));
            // dest[i] = strcpy(dest[i], append[i]);
        }

        // printf("dest[%zu]= %s\n", i, dest[i]);
    }

    return dest;
}

//--------------
// Helpers
//--------------

// Extension
// ###############
VkBool32 check_extensions(uint32_t check_count, const char **check_names,
                          uint32_t extension_count, VkExtensionProperties *extensions)
{
    uint32_t i = 0, j;
    for (; i < check_count; ++i)
    {
        VkBool32 found = VK_FALSE;
        for (j = 0; j < extension_count; ++j)
        {
            if (!strcmp(check_names[i], extensions[j].extensionName))
            {
                found = VK_TRUE;
                break;
            }
        }

        if (found != VK_TRUE)
        {
            fprintf(stderr, "Can not find extension: %s\n", check_names[i]);
            return VK_FALSE;
        }
    }

    return VK_TRUE;
}

VkBool32 check_instance_extension_support(uint32_t check_count, const char **check_names)
{
    VkResult res;
    // Need to get number of extensions to create array of correct size to hold extensions
    uint32_t extensionCount = 0;
    res = vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);
    assert(!res);

    VkExtensionProperties *extensions = (VkExtensionProperties *)(malloc(sizeof(VkExtensionProperties) * extensionCount));
    res = vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, extensions);
    assert(!res);

    VkBool32 b_res = check_extensions(check_count, check_names, extensionCount, extensions);
    free(extensions);
    return b_res;
}

VkBool32 check_device_extension_support(VkPhysicalDevice device)
{
    // Get the count
    uint32_t extension_count;
    VkResult res = vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count, NULL);
    if (res != VK_SUCCESS || !extension_count)
        return VK_FALSE;

    // Get extensions
    VkExtensionProperties *extensions = (VkExtensionProperties *)(malloc(sizeof(VkExtensionProperties) * extension_count));
    res = vkEnumerateDeviceExtensionProperties(device, NULL, &extension_count, extensions);
    // assert(res == VK_SUCCESS)

    // Specify required device extensions here
    VkBool32 result = check_extensions(requested_device_ext_count, requested_device_extensions, extension_count, extensions);

    free(extensions);
    return result;
}

// Validation
// ###############

VkBool32 check_validation_layers(uint32_t check_count, const char **check_names,
                                 uint32_t layer_count, VkLayerProperties *layers)
{
    uint32_t i = 0, j;
    for (; i < check_count; ++i)
    {
        VkBool32 found = VK_FALSE;
        for (j = 0; j < layer_count; ++j)
        {
            if (!strcmp(check_names[i], layers[j].layerName))
            {
                found = VK_TRUE;
                break;
            }
        }

        if (!found)
        {
            fprintf(stderr, "Can not find validation layer: %s\n", check_names[i]);
            return VK_FALSE;
        }
    }

    return 1;
}

VkBool32 check_validation_support(uint32_t check_count, const char **check_names)
{
    VkResult res;
    // Set the counter
    uint32_t validation_layer_count;
    res = vkEnumerateInstanceLayerProperties(&validation_layer_count, NULL);
    assert(!res);

    VkLayerProperties *validation_layers = (VkLayerProperties *)(malloc(sizeof(VkLayerProperties) * validation_layer_count));
    res = vkEnumerateInstanceLayerProperties(&validation_layer_count, validation_layers);
    assert(!res);

    VkBool32 b_res = check_validation_layers(check_count, check_names, validation_layer_count, validation_layers);
    free(validation_layers);
    return b_res;
}

const char **get_required_validation_layers(uint32_t *count)
{
    const char **required_validation_layers = (const char **)(malloc(sizeof(char *) * 1));
    required_validation_layers[0] = "VK_LAYER_KHRONOS_validation";
    if (enable_validation_layers && !check_validation_support(1, required_validation_layers))
        ERR_EXIT(
            "Validation layers requested but not available!\n",
            "vkCreateInstance Failure");

    // WARNING: Count should be set manually
    *count = 1;
    return required_validation_layers;
}

// Queue families
// ###############
VkBool32 is_valid_queue_family_indices(vulkan_physical_device_queue_family_info qfi)
{
    return qfi.graphics_family_index >= 0 && qfi.presentation_family_index >= 0;
}
vulkan_physical_device_queue_family_info get_queue_families(VkPhysicalDevice device)
{
    vulkan_physical_device_queue_family_info indices;

    // Get all queue family for given device
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);
    assert(queue_family_count > 0);

    VkQueueFamilyProperties *queue_families = (VkQueueFamilyProperties *)(malloc(sizeof(VkQueueFamilyProperties) * queue_family_count));
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);
    /*
        Queue family can contain one or more queues and each queue have type:
        - Graphics
        - Transfer data
        - Compute queue
    */

    // Go through each queue family and check if it has at least 1 of the required types of queue
    int i = 0;
    for (; i < queue_family_count; i++)
    {
        // First check if queue family has at least 1 queue in that family (could have no queues)
        // Queue can be multiple types defined through bitfield. Need to bitwise AND with VK_QUEUE_*_BIT to check if has required type
        if (queue_families[i].queueCount > 0 && queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            indices.graphics_family_index = i;

        // Check if Queue Family supports presentation
        VkBool32 presentation_support = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, context.surface, &presentation_support);
        // Check if queue is presentation type (can be both graphics and presentation)
        if (queue_families[i].queueCount > 0 && presentation_support)
            indices.presentation_family_index = i;

        // Check if queue family indices are in a valid state, stop searching if so
        if (is_valid_queue_family_indices(indices))
            break;
    }

    return indices;
}

// Swap-chain - surface
// ###############
SwapChainDetails get_swap_chain_details(VkPhysicalDevice device) // vulkan_device_query_swapchain_support
{
    SwapChainDetails details;

    // Get the surface capabilities for the given surface on the given physical device
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, context.surface, &details.surfaceCapabilities);

    // formats
    uint32_t formats_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, context.surface, &formats_count, 0);
    if (formats_count)
    {
        details.format_count = formats_count;
        // if (!details.formats)
        details.formats = (VkSurfaceFormatKHR *)(malloc(sizeof(VkSurfaceFormatKHR) * formats_count));
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, context.surface, &formats_count, details.formats);
    }

    // Presentation modes
    uint32_t presentation_modes_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, context.surface, &presentation_modes_count, NULL);
    if (presentation_modes_count)
    {
        details.presentation_mode_count = presentation_modes_count;
        // if (!details.presentationModes)
        details.presentationModes = (VkPresentModeKHR *)(malloc(sizeof(VkSurfaceFormatKHR) * presentation_modes_count));
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, context.surface, &presentation_modes_count, details.presentationModes);
    }

    return details;
}

// Best format is subjective, but ours will be:
// format		:	VK_FORMAT_R8G8B8A8_UNORM (VK_FORMAT_B8G8R8A8_UNORM as backup)
// colorSpace	:	VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
VkSurfaceFormatKHR choose_best_surface_format(VkSurfaceFormatKHR *formats, uint32_t formats_count)
{
    // If only 1 format available and is undefined, then this means ALL formats are available (no restrictions)
    if (formats == NULL || !formats_count || (formats_count == 1 && formats[0].format == VK_FORMAT_UNDEFINED))
    {
        return {VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    }

    // If restricted, search for optimal format
    uint32_t i;
    for (i = 0; i < formats_count; ++i)
    {
        if ((formats[i].format == VK_FORMAT_R8G8B8A8_UNORM || formats[i].format == VK_FORMAT_B8G8R8A8_UNORM || formats[i].format == VK_FORMAT_B8G8R8A8_SRGB) && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return formats[i];
    }

    // If can't find optimal format, then just return first format
    return formats[0];
}

VkPresentModeKHR choose_best_presentation_mode(VkPresentModeKHR *presentation_modes, uint32_t presentation_modes_count)
{
    if (presentation_modes == NULL || !presentation_modes_count)
        return VK_PRESENT_MODE_FIFO_KHR;

    uint32_t i;
    for (i = 0; i < presentation_modes_count; ++i)
    {
        if (presentation_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
            return presentation_modes[i];
    }

    // If can't find, use FIFO as Vulkan spec says it must be present
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D choose_swap_extent(GLFWwindow *window, VkSurfaceCapabilitiesKHR *surface_capabilities)
{
    if (surface_capabilities->currentExtent.width != UINT32_MAX)
        return surface_capabilities->currentExtent;

    // TODO: You might use context.framebuffer_width, height
    // Vulkan works with pixels so we need resolution in pixels
    int width, height;
    glfwGetFramebufferSize(window, &width, &height); // in pixels

    VkExtent2D actual_extent = {};
    actual_extent.width = (uint32_t)width;
    actual_extent.height = (uint32_t)height;

    // Surface also defines max and min, so make sure within boundaries by clamping value
    actual_extent.width = clamp(actual_extent.width, surface_capabilities->minImageExtent.width, surface_capabilities->maxImageExtent.width);
    actual_extent.height = clamp(actual_extent.height, surface_capabilities->minImageExtent.height, surface_capabilities->maxImageExtent.height);

    return actual_extent;
}

VkExtent2D choose_swap_extent2(u32 width, u32 height, VkSurfaceCapabilitiesKHR *surface_capabilities)
{
    if (surface_capabilities->currentExtent.width != UINT32_MAX)
        return surface_capabilities->currentExtent;

    VkExtent2D actual_extent = {};
    actual_extent.width = (uint32_t)width;
    actual_extent.height = (uint32_t)height;

    // Surface also defines max and min, so make sure within boundaries by clamping value
    actual_extent.width = clamp(actual_extent.width, surface_capabilities->minImageExtent.width, surface_capabilities->maxImageExtent.width);
    actual_extent.height = clamp(actual_extent.height, surface_capabilities->minImageExtent.height, surface_capabilities->maxImageExtent.height);

    return actual_extent;
}

// DEvice
// ###############

VkBool32 check_device_suitable(VkPhysicalDevice device, SwapChainDetails *details)
{
    /*
    // Information about the device itself (ID, name, type, vendor, etc)
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    // Information about what the device can do (geo shader, tess shader, wide lines, etc)
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
    */

    // Check extensions
    VkBool32 extensions_supported = check_device_extension_support(device);

    // Check swapchain capabilities
    VkBool32 swap_chain_adequate = VK_FALSE;
    if (extensions_supported)
    {
        *details = get_swap_chain_details(device);
        swap_chain_adequate = details->format_count && details->presentation_mode_count;
    }

    return is_valid_queue_family_indices(get_queue_families(device)) && extensions_supported && swap_chain_adequate;
}

//--------------
// Debug
//--------------
// The call is aborted if returns true;
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData)
{

    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        fprintf(stderr, pCallbackData->pMessage);

    return VK_FALSE;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != NULL)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != NULL)
    {
        func(instance, debugMessenger, pAllocator);
    }
}

void populate_debug_messenger(VkDebugUtilsMessengerCreateInfoEXT *createInfo)
{
    *createInfo = {};
    createInfo->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo->pfnUserCallback = debug_callback;
    createInfo->pUserData = NULL;
}

void setup_debug_messenger()
{
    if (!enable_validation_layers)
        return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populate_debug_messenger(&createInfo);

    if (CreateDebugUtilsMessengerEXT(context.instance, &createInfo, context.allocator, &debugMessenger) != VK_SUCCESS)
        ERR_EXIT("Failed to setup debug messenger.\n", "setup_debug_messenger");
}

//--------------
// Create
//--------------

int init_volk()
{
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

    return EXIT_SUCCESS;
}

void create_surface(GLFWwindow *window)
{
    if (glfwCreateWindowSurface(context.instance, window, context.allocator, &context.surface) != VK_SUCCESS)
        ERR_EXIT(
            "Failed to create window surface.\n",
            "glfwCreateWindowSurface() Failure");
}

void create_instance()
{
    uint32_t required_validation_count;
    const char **required_validation_layers = get_required_validation_layers(&required_validation_count);

    context.framebuffer_width = (cached_framebuffer_width != 0) ? cached_framebuffer_width : 800;
    context.framebuffer_height = (cached_framebuffer_height != 0) ? cached_framebuffer_height : 600;
    cached_framebuffer_width = 0;
    cached_framebuffer_height = 0;

    // Information about the application itself
    // Most data here doesn't affect the program and is for developer convenience
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan App";               // Custom name of the application
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0); // Custom version of the application
    appInfo.pEngineName = "No Engine";                     // Custom engine name
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);      // Custom engine version
    appInfo.apiVersion = VK_API_VERSION_1_3;               // The Vulkan Version

    // Check the extensions first
    // glfw may require multiple extensions
    uint32_t required_extension_count = 0;
    const char **required_extensions = NULL;
    required_extensions = glfwGetRequiredInstanceExtensions(&required_extension_count);
    // This list already includes VK_KHR_surface extension for surface

    if (enable_validation_layers)
    {
        // doing c tricks to append string to array
        const char *extra_extensions[] = {VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
        required_extensions = append_strs_to_str_arr(required_extensions, required_extension_count, extra_extensions, 1);
        ++required_extension_count;
    }

// MoltenVK VK_ERROR_INCOMPATIBLE_DRIVER error:
// Beginning with the 1.3.216 Vulkan SDK, the VK_KHR_PORTABILITY_subset extension is mandatory.
#ifdef __APPLE__
    requiredExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

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

    if (!check_instance_extension_support(required_extension_count, required_extensions))
    {
        ERR_EXIT(
            "VkInstance does not support the required extensions.\n",
            "vkCreateInstance Failure");
    }

    // Creation information for a VkInstance (Vulkan Instance)
    // tells the Vulkan driver which global extensions and validation layers we want to use
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    // Extension info
    createInfo.enabledExtensionCount = required_extension_count;
    createInfo.ppEnabledExtensionNames = required_extensions;

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (!enable_validation_layers)
    {
        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledLayerNames = NULL;
        createInfo.pNext = NULL;
    }
    else
    {
        createInfo.enabledLayerCount = required_validation_count;
        createInfo.ppEnabledLayerNames = required_validation_layers;

        populate_debug_messenger(&debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
    }

#ifdef __APPLE__
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    // Create instance
    // TODO: custom allocator.
    context.allocator = 0;
    VkResult res = vkCreateInstance(&createInfo, context.allocator, &context.instance);
    if (res == VK_ERROR_INCOMPATIBLE_DRIVER)
    {
        ERR_EXIT(
            "Cannot find a compatible Vulkan installable client driver "
            "(ICD).\n\nPlease look at the Getting Started guide for "
            "additional information.\n",
            "vkCreateInstance Failure");
    }
    else if (res == VK_ERROR_EXTENSION_NOT_PRESENT)
    {
        ERR_EXIT(
            "Cannot find a specified extension library"
            ".\nMake sure your layers path is set appropriately\n",
            "vkCreateInstance Failure");
    }
    else if (res)
    {
        ERR_EXIT(
            "vkCreateInstance failed.\n\nDo you have a compatible Vulkan "
            "installable client driver (ICD) installed?\nPlease look at "
            "the Getting Started guide for additional information.\n",
            "vkCreateInstance Failure");
    }

    volkLoadInstance(context.instance);
}

void get_physical_device()
{
    VkResult res;
    // Enumerate physical devices
    // Get the counter
    uint32_t device_count = 0;
    res = vkEnumeratePhysicalDevices(context.instance, &device_count, NULL);
    assert(res == VK_SUCCESS);

    if (!device_count)
        ERR_EXIT("Can't find GPUs that support Vulkan instance.\n", "get_physical_device");

    // Get the physical devices
    VkPhysicalDevice *physical_devices = (VkPhysicalDevice *)(malloc(sizeof(VkPhysicalDevice) * device_count));
    res = vkEnumeratePhysicalDevices(context.instance, &device_count, physical_devices);
    assert(res == VK_SUCCESS);

    for (uint32_t i = 0; i < device_count; i++)
    {
        if (check_device_suitable(physical_devices[i], &context.device.swapchain_support))
        {
            context.device.physical_device = physical_devices[i];
            break;
        }
    }

    if (context.device.physical_device == VK_NULL_HANDLE)
        ERR_EXIT("Failed to find a suitable GPU.\n", "get_physical_device");

    // Get the queue family indices for the chosen Physical Device
    vulkan_physical_device_queue_family_info indices = get_queue_families(context.device.physical_device);
    context.device.graphics_queue_index = indices.graphics_family_index;
    context.device.present_queue_index = indices.presentation_family_index;
}

void create_logical_device()
{
    b8 present_shares_graphics_queue = context.device.graphics_queue_index == context.device.present_queue_index;

    // For each indices, it requires a queue ->std::unordered_set can be used here
    // No duplicate indices should be alive
    // TODO: We don't have hash_set impl yet so use this manual approach
    size_t indices_count = present_shares_graphics_queue ? 1 : 2;

    i32 *unique_queue_families = (i32 *)(malloc(sizeof(i32) * indices_count));
    if (indices_count == 1)
        unique_queue_families[0] = context.device.graphics_queue_index;
    else if (indices_count == 2)
    {
        unique_queue_families[0] = context.device.graphics_queue_index;
        unique_queue_families[1] = context.device.present_queue_index;
    }

    // Store queue infos
    VkDeviceQueueCreateInfo *queue_create_infos = (VkDeviceQueueCreateInfo *)(malloc(sizeof(VkDeviceQueueCreateInfo) * indices_count));
    {
        float priority = 1.0f;
        size_t i;
        for (i = 0; i < indices_count; ++i)
        {
            // Queue the logical device needs to create and info to do so (Only 1 for now, will add more later!)
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = unique_queue_families[i]; // The index of the family to create a queue from
            queueCreateInfo.queueCount = 1;                              // Number of queues to create
            queueCreateInfo.pQueuePriorities = &priority;                // Vulkan needs to know how to handle multiple queues, so decide priority (1 = highest priority)
            queue_create_infos[i] = queueCreateInfo;
        }
    }

    // Information to create logical device (sometimes called "device")
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = indices_count;                  // Number of Queue Create Infos (number of queues)
    deviceCreateInfo.pQueueCreateInfos = queue_create_infos;                // List of queue create infos so device can create required queues
    deviceCreateInfo.enabledExtensionCount = requested_device_ext_count;    // Number of enabled logical device extensions
    deviceCreateInfo.ppEnabledExtensionNames = requested_device_extensions; // List of enabled logical device extensions

    // Physical Device Features the Logical Device will be using
    VkPhysicalDeviceFeatures deviceFeatures = {};

    deviceCreateInfo.pEnabledFeatures = &deviceFeatures; // Physical Device features Logical Device will use

    uint32_t required_validation_layers_count;
    const char **required_validation_layers = get_required_validation_layers(&required_validation_layers_count);
    if (enable_validation_layers)
    {
        deviceCreateInfo.enabledLayerCount = required_validation_layers_count;
        deviceCreateInfo.ppEnabledLayerNames = required_validation_layers;
    }
    else
    {
        deviceCreateInfo.enabledLayerCount = 0;
        deviceCreateInfo.ppEnabledLayerNames = NULL;
    }

    // Create the logical device for the given physical device
    VkResult result = vkCreateDevice(context.device.physical_device, &deviceCreateInfo, nullptr, &context.device.logical_device);
    if (result != VK_SUCCESS)
        ERR_EXIT("Failed to create a Logical Device!\n", "create_logical_device");

    // Queues are created at the same time as the device...
    // So we want handle to queues
    // From given logical device, of given Queue Family, of given Queue Index (0 since only one queue), place reference in given VkQueue
    vkGetDeviceQueue(context.device.logical_device, context.device.graphics_queue_index, 0, &context.device.graphicsQueue);
    vkGetDeviceQueue(context.device.logical_device, context.device.present_queue_index, 0, &context.device.presentQueue);

    free(unique_queue_families);
    free(queue_create_infos);
}

void create_swap_chain(GLFWwindow *window, u32 width, u32 height)
{
    SwapChainDetails details = get_swap_chain_details(context.device.physical_device);

    // Choose the best values for the swap chain
    VkSurfaceFormatKHR surface_format = choose_best_surface_format(details.formats, details.format_count);
    VkPresentModeKHR presentation_mode = choose_best_presentation_mode(details.presentationModes, details.presentation_mode_count);
    // No need for this since we are passing width and heigh explicitly
    // VkExtent2D extent = choose_swap_extent(window, &details.surfaceCapabilities);
    VkExtent2D extent = choose_swap_extent2(width, height, &details.surfaceCapabilities);

    // How many images to store in the swap chain?
    /*
    However, simply sticking to this minimum means that we may sometimes have to wait on the driver to
    complete internal operations before we can acquire another image to render to.
    Therefore it is recommended to request at least one more image than the minimum:
    */
    uint32_t image_count = details.surfaceCapabilities.minImageCount + 1;
    // Zero has special meaning and make sure it is not greater than  the allowed max
    if (details.surfaceCapabilities.maxImageCount > 0 && image_count > details.surfaceCapabilities.maxImageCount)
        image_count = details.surfaceCapabilities.maxImageCount;

    context.swap_chain.max_frames_in_flight = image_count - 1;

    // Creation information for swap chain
    VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface = context.surface;                   // Swapchain surface
    swapChainCreateInfo.imageFormat = surface_format.format;         // Swapchain format
    swapChainCreateInfo.imageColorSpace = surface_format.colorSpace; // Swapchain colour space
    swapChainCreateInfo.presentMode = presentation_mode;             // Swapchain presentation mode
    swapChainCreateInfo.imageExtent = extent;                        // Swapchain image extents
    swapChainCreateInfo.minImageCount = image_count;                 // Minimum images in swapchain
    // always 1 unless you are developing a stereoscopic 3D application
    swapChainCreateInfo.imageArrayLayers = 1;                                        // Number of layers for each image in chain
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;            // What attachment images will be used as
    swapChainCreateInfo.preTransform = details.surfaceCapabilities.currentTransform; // Transform to perform on swap chain images
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;          // How to handle blending images with external graphics (e.g. other windows)
    swapChainCreateInfo.clipped = VK_TRUE;                                           // Whether to clip parts of image not in view (e.g. behind another window, off screen, etc)

    // If Graphics and Presentation families are different (quite unlikely but we have to handle that possibility), then swapchain must let images be shared between families
    if (context.device.graphics_queue_index != context.device.present_queue_index)
    {
        // Queues to share between
        uint32_t queueFamilyIndices[] = {
            (uint32_t)context.device.graphics_queue_index,
            (uint32_t)context.device.present_queue_index};

        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // Image share handling
        swapChainCreateInfo.queueFamilyIndexCount = 2;                     // Number of queues to share images between
        swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;      // Array of queues to share between
    }
    else
    {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapChainCreateInfo.queueFamilyIndexCount = 0;
        swapChainCreateInfo.pQueueFamilyIndices = NULL;
    }

    // IF old swap chain been destroyed and this one replaces it, then link old one to quickly hand over responsibilities
    // i.e. after resize, the actual swap chain must be destroyed and replaced by a new one.
    // For now, we ignore resizing.
    /*
    With Vulkan it's possible that your swap chain becomes invalid or unoptimized while your application is running, for example because the window was resized.
    In that case the swap chain actually needs to be recreated from scratch and a reference to the old one must be specified in this field.
    */
    swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    // Create Swapchain
    VkResult result = vkCreateSwapchainKHR(context.device.logical_device, &swapChainCreateInfo, context.allocator, &context.swap_chain.handle);
    if (result != VK_SUCCESS)
        ERR_EXIT(
            "Failed to create swap chain!\n",
            "create_swap_chain");

    context.swap_chain.surface_format = surface_format;
    context.swap_chain.extent_2d = extent;
    // Start with a zero frame index.
    context.current_frame = 0;
    context.swap_chain.image_count = 0;

    // Now we have to retrieve the handles to the images
    // The images are created and cleaned up by swapchain impl so no need to create/clean up images explicitly
    // Images are raw data (i.e. Texture in OpenGL) and imageView is an interface to an image to view it.
    // Imageview describes how to access the image and which part of the image to access,
    // for example if it should be treated as a 2D texture depth texture without any mipmapping levels.
    result = vkGetSwapchainImagesKHR(context.device.logical_device, context.swap_chain.handle, &context.swap_chain.image_count, 0);
    if (result != VK_SUCCESS)
        ERR_EXIT("Failed to retrieve swapchain image count.\n", "create_swapchain");

    if (!context.swap_chain.image_count)
        return; // TODO: Not sure what to return

    if (!context.swap_chain.images)
        context.swap_chain.images = (VkImage *)(malloc(sizeof(VkImage) * context.swap_chain.image_count));

    result = vkGetSwapchainImagesKHR(context.device.logical_device, context.swap_chain.handle, &context.swap_chain.image_count, context.swap_chain.images);
    if (result != VK_SUCCESS)
        ERR_EXIT("Failed to retrieve swapchain images.\n", "create_swapchain");

    if (!context.swap_chain.views)
        context.swap_chain.views = (VkImageView *)(malloc(sizeof(VkImageView) * context.swap_chain.image_count));

    for (uint32_t i = 0; i < context.swap_chain.image_count; ++i)
    {
        VkImageViewCreateInfo viewCreateInfo = {};
        viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreateInfo.image = context.swap_chain.images[i];              // Image to create view for
        viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;                  // Type of image (1D, 2D, 3D, Cube, etc)
        viewCreateInfo.format = context.swap_chain.surface_format.format; // Format of image data
        // viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;      // Allows remapping of rgba components to other rgba values
        // viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        // viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        // viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        // Subresources allow the view to view only a part of an image
        viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // Which aspect of image to view (e.g. COLOR_BIT for viewing colour)
        viewCreateInfo.subresourceRange.baseMipLevel = 0;                       // Start mipmap level to view from
        viewCreateInfo.subresourceRange.levelCount = 1;                         // Number of mipmap levels to view
        viewCreateInfo.subresourceRange.baseArrayLayer = 0;                     // Start array level to view from
        viewCreateInfo.subresourceRange.layerCount = 1;                         // Number of array levels to view

        result = vkCreateImageView(context.device.logical_device, &viewCreateInfo, context.allocator, &context.swap_chain.views[i]);
        if (result != VK_SUCCESS)
            ERR_EXIT("Failed to retrieve swapchain images view.\n", "create_swapchain::vkCreateImageView");
    }
}

VkShaderModule create_shader_module(const char *filename)
{
    // Obtain file handle.
    file_handle handle;
    if (!filesystem_open(filename, FILE_MODE_READ, true, &handle))
    {
        ERR_EXIT("Unable to read shader module.\n", "create_shader_module");
    }

    // Read the entire file as binary.
    u64 size = 0;
    u8 *file_buffer = 0;
    if (!filesystem_read_all_bytes(&handle, &file_buffer, &size))
    {
        ERR_EXIT("Unable to binary read shader module.\n", "create_shader_module");
    }

    // Shader Module creation information
    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = size;            // Size of code
    shaderModuleCreateInfo.pCode = (u32 *)file_buffer; // Pointer to code (of uint32_t pointer type)

    // Close the file.
    filesystem_close(&handle);

    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule(context.device.logical_device, &shaderModuleCreateInfo, context.allocator, &shaderModule);
    if (result != VK_SUCCESS)
        ERR_EXIT("Failed to create a shader module!\n", "create_shader_module");

    return shaderModule;
}

VkSubpassDependency define_subpass_dep()
{

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dependencyFlags = 0;

    return dependency;

    /*


    // // Need to determine when layout transitions occur using subpass dependencies
    // VkSubpassDependency subpassDependencies[2] = {};

    // // Transition from colourAttachment.initialLayout to colourAttachmentReference.layout
    // // Conversion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    // // Transition must happen after...
    // subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;                    // Subpass index (VK_SUBPASS_EXTERNAL = Special value meaning outside of renderpass)
    // subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // Pipeline stage
    // subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;           // Stage access mask (memory access)
    // // But must happen before...
    // subpassDependencies[0].dstSubpass = 0;
    // subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    // subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    // subpassDependencies[0].dependencyFlags = 0;

    // // Transition from colourAttachmentReference.layout to  colourAttachment.finalLayout
    // // Conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    // // Transition must happen after...
    // subpassDependencies[1].srcSubpass = 0;
    // subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    // subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    // ;
    // // But must happen before...
    // subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL; // since we have one subpass it is going out of the render pass
    // subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    // subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    // subpassDependencies[1].dependencyFlags = 0;

    */
}

/*
we need to tell Vulkan about the framebuffer attachments that will be used while rendering. We need to specify how many color and depth buffers there will be,
how many samples to use for each of them and how their contents should be handled throughout the rendering operations.
*/
void create_render_pass()
{
    // --- ATTACHMENT ---
    // Colour attachment of render pass
    VkAttachmentDescription colourAttachment = {};
    // The format of the color attachment should match the format of the swap chain images,
    colourAttachment.format = context.swap_chain.surface_format.format; // Format to use for attachment
    colourAttachment.samples = VK_SAMPLE_COUNT_1_BIT;                   // Number of samples to write for multisampling
    // Load op will be performed when it first loads the color attachment i.e. OPENGL -> GL_CLEAR operation
    /*
    VK_ATTACHMENT_LOAD_OP_CLEAR:
    This basically means that when we start the render pass, whatever we're going to be writing to,
    don't load in any data from it, just clear it out.
    We don't want to use anything. What we want to start with a blank slate and just start drawing from there
    */
    colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Describes what to do with attachment before rendering
    /*
     we're saying when we're finished with the render pass, when it ends, what do we want to do with all the data?
    VK_ATTACHMENT_STORE_OP_STORE: store it
    */
    colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;            // Describes what to do with attachment after rendering
    colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;   // Describes what to do with stencil before rendering
    colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Describes what to do with stencil after rendering

    // Layouts can be considered as forms of the subpasses at different stages
    // Framebuffer data will be stored as an image, but images can be given different data layouts
    // to give optimal use for certain operations
    // This is the layout we expect it to be between render pass and the starting of the subpass
    colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Image data layout before render pass starts
    // Then initial layout is converted to the attachmentreference.layout

    // Then, when all the subpasses are finished, when the render pass comes to an end, the attachmentreference.layout is converted to the finallayout
    colourAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Image data layout after render pass (to change to)

    // Attachment reference uses an attachment index that refers to index in the attachment list passed to renderPassCreateInfo
    /*
    Directly referenced from fragment shader's layout(location = 0) out vec4 outColor
    */
    VkAttachmentReference colourAttachmentReference = {};
    colourAttachmentReference.attachment = 0; // (location=0)
    colourAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // --- SUBPASS ---
    /*
     Subpasses are subsequent rendering operations that depend on the contents of framebuffers in previous passes,
     for example a sequence of post-processing effects that are applied one after another.
     If you group these rendering operations into one render pass, then Vulkan is able to reorder
        the operations and conserve memory bandwidth for possibly better performance.
     For our very first triangle, however, we'll stick to a single subpass.
    */
    // Information about a particular subpass the Render Pass is using
    VkSubpassDescription subpass = {};
    // There might be subpasses for compute in future so be explicit about the intent
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // Pipeline type subpass is to be bound to
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colourAttachmentReference;

    // Need to determine when layout transitions occur using subpass dependencies
    VkSubpassDependency subpassDependency = define_subpass_dep();

    // Create info for Render Pass
    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    /*
    Attachments correspond to location for out vars in shaders
    i.e. in shader layout(location=0) out vec4 outColor; corresponds to first color attachment in the render pass
    sometimes attachments correspond to in data but for now we focus on out
    */
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &colourAttachment;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;
    renderPassCreateInfo.dependencyCount = 1; // subpassDependencies.size()
    renderPassCreateInfo.pDependencies = &subpassDependency;

    VkResult result = vkCreateRenderPass(context.device.logical_device, &renderPassCreateInfo, context.allocator, &context.main_renderpass.handle);
    if (result != VK_SUCCESS)
        ERR_EXIT("Failed to create a Render Pass!\n", "create_render_pass");

    context.main_renderpass.w = context.framebuffer_width;
    context.main_renderpass.h = context.framebuffer_height;
    context.main_renderpass.x = 0;
    context.main_renderpass.y = 0;
}

void create_graphics_pipeline()
{
    // Read in SPIR-V code of shaders
    // char vertex_shader_code[1024 * 256];
    // size_t vertex_shader_len;
    // b8 res = parse_file_into_str("assets/shaders/shader_base.vert.spv", 1024 * 256, vertex_shader_code, &vertex_shader_len);

    // char frag_shader_code[1024 * 256];
    // size_t frag_shader_len;
    // res = parse_file_into_str("assets/shaders/shader_base.frag.spv", 1024 * 256, frag_shader_code, &frag_shader_len);

    // Create Shader Modules
    VkShaderModule vertexShaderModule = create_shader_module("assets/shaders/shader_base.vert.spv");
    VkShaderModule fragmentShaderModule = create_shader_module("assets/shaders/shader_base.frag.spv");

    // -- SHADER STAGE CREATION INFORMATION --
    // Vertex Stage creation information
    VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
    vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT; // Shader Stage name
    vertexShaderCreateInfo.module = vertexShaderModule;        // Shader module to be used by stage
    vertexShaderCreateInfo.pName = "main";                     // Entry point in to shader

    // Fragment Stage creation information
    VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
    fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT; // Shader Stage name
    fragmentShaderCreateInfo.module = fragmentShaderModule;        // Shader module to be used by stage
    fragmentShaderCreateInfo.pName = "main";                       // Entry point in to shader

    // Put shader stage creation info in to array
    // Graphics Pipeline creation info requires array of shader stage creates
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertexShaderCreateInfo, fragmentShaderCreateInfo};

    // Create Graphics Pipeline
    // -- VERTEX INPUT (TODO: Put in vertex descriptions when resources created) --
    /*
    - Bindings: spacing between data and whether the data is per-vertex or per-instance (see instancing)
    - Attribute descriptions: type of the attributes passed to the vertex shader,
        which binding to load them from and at which offset
    */
    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
    vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
    vertexInputCreateInfo.pVertexBindingDescriptions = NULL; // List of Vertex Binding Descriptions (data spacing/stride information)
    vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
    vertexInputCreateInfo.pVertexAttributeDescriptions = NULL; // List of Vertex Attribute Descriptions (data format and where to bind to/from)

    // -- INPUT ASSEMBLY --
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    // OpenGL mapping-> GL_TRIANGLES etc.
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // Primitive type to assemble vertices as
    /*
     If you set the primitiveRestartEnable member to VK_TRUE, then
        it's possible to break up lines and triangles in the _STRIP topology
        modes by using a special index of 0xFFFF or 0xFFFFFFFF.
    */
    inputAssembly.primitiveRestartEnable = VK_FALSE; // Allow overriding of "strip" topology to start new primitives

    // -- VIEWPORT & SCISSOR --
    /*
        Useful to generate split screen multiplayer/view
    */
    // Create a viewport info struct
    VkViewport viewport = {};
    viewport.x = 0.0f;                                            // x start coordinate
    viewport.y = 0.0f;                                            // y start coordinate
    viewport.width = (float)context.swap_chain.extent_2d.width;   // width of viewport (f32)context.framebuffer_width
    viewport.height = (float)context.swap_chain.extent_2d.height; // height of viewport (f32)context.framebuffer_height
    viewport.minDepth = 0.0f;                                     // min framebuffer depth
    viewport.maxDepth = 1.0f;                                     // max framebuffer depth

    // Create a scissor info struct
    // Similar to cropping
    VkRect2D scissor = {};
    scissor.offset = {0, 0};                       // Offset to use region from
    scissor.extent = context.swap_chain.extent_2d; // Extent to describe region to use, starting at offset

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports = &viewport;
    viewportStateCreateInfo.scissorCount = 1;
    viewportStateCreateInfo.pScissors = &scissor;

    /*
    Most of the states within the graphics pipeline is immutable which requires you to recreate another pipeline for each combination of state
    you want to change.
    Thankfully, dynamic states come to the rescue. Some states/settings can be labeled as dynamic to be changed in runtime.
    NOTE: Even there are some dynamic states active, you still need to recreate swapchain and/or depth buffer accordingly.
    */
    // -- DYNAMIC STATES --
    // Dynamic states to enable
    // std::vector<VkDynamicState> dynamicStateEnables;
    // dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);	// Dynamic Viewport : Can resize in command buffer with vkCmdSetViewport(commandbuffer, 0, 1, &viewport);
    // dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);	// Dynamic Scissor	: Can resize in command buffer with vkCmdSetScissor(commandbuffer, 0, 1, &scissor);

    const u32 dynamic_state_count = 2;
    VkDynamicState dynamic_states[dynamic_state_count] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR};

    // Dynamic State creation info
    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.dynamicStateCount = dynamic_state_count;
    dynamicStateCreateInfo.pDynamicStates = dynamic_states;

    // -- RASTERIZER --
    /*
    converts primitives into fragments
    */
    VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
    rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    /*
    If depthClampEnable is set to VK_TRUE, then fragments that are beyond the near and far planes are clamped to them as opposed to discarding them.
    Happens in minecraft
        This is useful in some special cases like shadow maps. Using this requires enabling a GPU feature within PhysicalDeviceFeature while
        creating logical device. --> deviceFeature.depthClamp = VK_TRUE
    */
    rasterizerCreateInfo.depthClampEnable = VK_FALSE; // Change if fragments beyond near/far planes are clipped (default) or clamped to plane
    // To skip this rasterization stage (Not very common maybe required when you want to carry out some computations with the data coming from prev stages)
    rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE; // Whether to discard data and skip rasterizer. Never creates fragments, only suitable for pipeline without framebuffer output
    // How to color between points (vertices)
    rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL; // How to handle filling points between vertices
    rasterizerCreateInfo.lineWidth = 1.0f;                   // How thick lines should be when drawn
    // Cull->seeing or not seeing the behind or inside of an object.
    rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;    // Which face of a tri to cull -> do NOT show back
    rasterizerCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE; // Winding to determine which side is front
    rasterizerCreateInfo.depthBiasEnable = VK_FALSE;          // Whether to add depth bias to fragments (good for stopping "shadow acne" in shadow mapping)

    // -- MULTISAMPLING --
    /*
    removing the jaggles of the edges
    */
    VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo = {};
    multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisamplingCreateInfo.sampleShadingEnable = VK_FALSE;               // Enable multisample shading or not
    multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; // Number of samples to use per fragment

    // -- BLENDING --
    // Blending decides how to blend a new colour being written to a fragment, with the old value

    // Blend Attachment State (how blending is handled)
    VkPipelineColorBlendAttachmentState colourState = {};
    colourState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT // Colours to apply blending to
                                 | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colourState.blendEnable = VK_TRUE; // Enable blending

    // Blending uses equation: (srcColorBlendFactor * new colour) colorBlendOp (dstColorBlendFactor * old colour)
    colourState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colourState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colourState.colorBlendOp = VK_BLEND_OP_ADD;

    // Summarised: (VK_BLEND_FACTOR_SRC_ALPHA * new colour) + (VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * old colour)
    //			   (new colour alpha * new colour) + ((1 - new colour alpha) * old colour)

    colourState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colourState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colourState.alphaBlendOp = VK_BLEND_OP_ADD;
    // Summarised: (1 * new alpha) + (0 * old alpha) = new alpha

    VkPipelineColorBlendStateCreateInfo colourBlendingCreateInfo = {};
    colourBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colourBlendingCreateInfo.logicOpEnable = VK_FALSE; // Alternative to calculations is to use logical operations
    colourBlendingCreateInfo.attachmentCount = 1;
    colourBlendingCreateInfo.pAttachments = &colourState;

    // -- PIPELINE LAYOUT (TODO: Apply Future Descriptor Set Layouts) --
    /*
    Uniforms (global objects in shaders) and layouts are required to be specified during pipeline layout
    creation. Even though you are not using, you still have to specify empty(null) ones.
    */
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 0;
    pipelineLayoutCreateInfo.pSetLayouts = NULL;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = NULL;

    // Create Pipeline Layout
    VkResult result = vkCreatePipelineLayout(context.device.logical_device, &pipelineLayoutCreateInfo, context.allocator, &pipelineLayout);
    if (result != VK_SUCCESS)
        ERR_EXIT("Failed to create Pipeline Layout!\n", "create_graphics_pipeline::vkCreatePipelineLayout");

    // -- DEPTH STENCIL TESTING --
    // TODO: Set up depth stencil testing

    // -- GRAPHICS PIPELINE CREATION --
    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = 2;                             // Number of shader stages
    pipelineCreateInfo.pStages = shaderStages;                     // List of shader stages
    pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo; // All the fixed function pipeline states
    pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
    pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
    pipelineCreateInfo.pColorBlendState = &colourBlendingCreateInfo;
    pipelineCreateInfo.pDepthStencilState = NULL;
    pipelineCreateInfo.layout = pipelineLayout; // Pipeline Layout pipeline should use
    //  this pipeline will be used by the render pass, not that the pipeline will use the render pass.
    pipelineCreateInfo.renderPass = context.main_renderpass.handle; // Render pass description the pipeline is compatible with
    pipelineCreateInfo.subpass = 0;                                 // Subpass of render pass to use with pipeline

    // Pipeline Derivatives : Can create multiple pipelines that derive from one another for optimisation
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE; // Existing pipeline to derive from...
    pipelineCreateInfo.basePipelineIndex = -1;              // or index of pipeline being created to derive from (in case creating multiple at once)

    // Create Graphics Pipeline
    // You can also use pipeline cache to avoid recreation
    result = vkCreateGraphicsPipelines(context.device.logical_device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, context.allocator, &graphicsPipeline);
    if (result != VK_SUCCESS)
        ERR_EXIT("Failed to create a Graphics Pipeline!\n", "create_graphics_pipeline::vkCreateGraphicsPipelines");

    // Destroy Shader Modules, no longer needed after Pipeline created
    vkDestroyShaderModule(context.device.logical_device, fragmentShaderModule, context.allocator);
    vkDestroyShaderModule(context.device.logical_device, vertexShaderModule, context.allocator);
}

/**
 * @brief Framebuffers represent a collection of memory attachments that are used by the
 * renderpass. Each attachment include image buffer (vkimageview) and/or depth buffer.
 * Images are created within the swapchain. There should be a framebuffer for each image and/or depth buffer.
 */
void create_frame_buffers()
{
    context.swap_chain.framebuffers = (vulkan_framebuffer *)(malloc(sizeof(vulkan_framebuffer) * context.swap_chain.image_count));
    for (uint32_t i = 0; i < context.swap_chain.image_count; ++i)
    {
        uint32_t attachment_count = 1;
        VkImageView attachments[/*attachment_count*/] = {
            context.swap_chain.views[i] // TODO: add Depth attachment
        };

        // Take a copy of the attachments
        context.swap_chain.framebuffers[i].attachments = (VkImageView *)(malloc(sizeof(VkImageView) * attachment_count));
        for (uint32_t j = 0; j < attachment_count; ++j)
            context.swap_chain.framebuffers[i].attachments[j] = attachments[j];

        context.swap_chain.framebuffers[i].attachment_count = attachment_count;
        context.swap_chain.framebuffers[i].renderpass = &context.main_renderpass;

        // Generate the handle
        VkFramebufferCreateInfo framebuffer_create_info = {};
        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.renderPass = context.main_renderpass.handle;
        framebuffer_create_info.attachmentCount = attachment_count;
        framebuffer_create_info.pAttachments = context.swap_chain.framebuffers[i].attachments;
        framebuffer_create_info.width = context.framebuffer_width;   // context.swap_chain.extent_2d.width;
        framebuffer_create_info.height = context.framebuffer_height; // context.swap_chain.extent_2d.height;
        framebuffer_create_info.layers = 1;

        VkResult result = vkCreateFramebuffer(context.device.logical_device, &framebuffer_create_info,
                                              context.allocator, &context.swap_chain.framebuffers[i].handle);

        if (result != VK_SUCCESS)
            ERR_EXIT("Failed to create VkFramebuffer!\n", "create_frame_buffers::vkCreateFrameBuffer");
    }
}

void create_command_pool()
{
    // Create command pool for graphics queue.
    VkCommandPoolCreateInfo pool_create_info = {};
    pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    // Use the graphics queue
    pool_create_info.queueFamilyIndex = context.device.graphics_queue_index;
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VkResult result = vkCreateCommandPool(
        context.device.logical_device, &pool_create_info, context.allocator, &context.device.graphics_command_pool);

    if (result != VK_SUCCESS)
        ERR_EXIT("Failed to create Command pool!\n", "create_command_pool::vkCreateCommandPool");
}

// FWD decl
void vulkan_command_buffer_free(
    vulkan_context *context,
    VkCommandPool *pool,
    vulkan_command_buffer *command_buffer);
/**
 * Command buffers are destroyed automatically during the destruction of
 * the corresponding command pool so no need to explicitly free.
 */
void create_command_buffers()
{
    // Allocate if not defined yet
    if (!context.graphics_command_buffers)
    {
        context.graphics_command_buffers = (vulkan_command_buffer *)malloc(sizeof(vulkan_command_buffer) * context.swap_chain.image_count);
        for (u32 i = 0; i < context.swap_chain.image_count; ++i)
            memset(&context.graphics_command_buffers[i], 0, sizeof(vulkan_command_buffer));
    }

    for (u32 i = 0; i < context.swap_chain.image_count; ++i)
    {
        if (context.graphics_command_buffers[i].handle)
        {
            // Deallocate the existing ones
            vulkan_command_buffer_free(
                &context,
                &context.device.graphics_command_pool,
                &context.graphics_command_buffers[i]);
        }
        // reset
        memset(&context.graphics_command_buffers[i], 0, sizeof(vulkan_command_buffer));

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = context.device.graphics_command_pool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        allocInfo.pNext = 0;

        context.graphics_command_buffers[i].state = COMMAND_BUFFER_STATE_NOT_ALLOCATED;
        VkResult result = vkAllocateCommandBuffers(context.device.logical_device, &allocInfo, &context.graphics_command_buffers[i].handle);
        if (result != VK_SUCCESS)
            ERR_EXIT("Failed to allocate command buffers!\n", "create_command_buffer::vkAllocateCommandBuffers");
        context.graphics_command_buffers[i].state = COMMAND_BUFFER_STATE_READY;
    }
}

// Sync
//--------------

void vulkan_fence_create(vulkan_context *context, b8 create_signaled, vulkan_fence *out_fence)
{
    out_fence->is_signaled = create_signaled;
    VkFenceCreateInfo fence_create_info{};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if (out_fence->is_signaled)
        fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkResult res = vkCreateFence(context->device.logical_device, &fence_create_info, context->allocator, &out_fence->handle);
}

b8 vulkan_fence_wait(vulkan_context *context, vulkan_fence *fence, u64 timeout_ns)
{
    if (!fence->is_signaled)
    { // We have to wait
        VkResult result = vkWaitForFences(
            context->device.logical_device,
            1,
            &fence->handle,
            VK_TRUE,
            timeout_ns);
        switch (result)
        {
        case VK_SUCCESS:
            fence->is_signaled = true;
            return BC_TRUE;
        case VK_TIMEOUT:
            // KWARN("vk_fence_wait - Timed out");
            break;
        case VK_ERROR_DEVICE_LOST:
            printf("ERROR: vk_fence_wait - VK_ERROR_DEVICE_LOST.");
            break;
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            printf("ERROR: vk_fence_wait - VK_ERROR_OUT_OF_HOST_MEMORY.");
            break;
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            printf("ERROR: vk_fence_wait - VK_ERROR_OUT_OF_DEVICE_MEMORY.");
            break;
        default:
            printf("ERROR: vk_fence_wait - An unknown error has occurred.");
            break;
        }
    }
    else
    {
        // If already signaled, do not wait.
        return BC_TRUE;
    }

    return BC_FALSE;
}

void vulkan_fence_reset(vulkan_context *context, vulkan_fence *fence)
{
    if (fence->is_signaled)
    {
        VkResult res = vkResetFences(context->device.logical_device, 1, &fence->handle);
        if (res != VK_SUCCESS)
            ERR_EXIT("Failed to reset fence.\n", "vkResetFences");

        fence->is_signaled = BC_FALSE;
    }
}

void create_sync_objects()
{
    // TODO: 1 for now
    context.image_available_semaphores = (VkSemaphore *)malloc(sizeof(VkSemaphore) * context.swap_chain.max_frames_in_flight);
    context.queue_complete_semaphores = (VkSemaphore *)malloc(sizeof(VkSemaphore) * context.swap_chain.max_frames_in_flight);
    context.in_flight_fences = (vulkan_fence *)(malloc(sizeof(vulkan_fence) * context.swap_chain.max_frames_in_flight));

    for (u8 i = 0; i < context.swap_chain.max_frames_in_flight; ++i)
    {
        VkSemaphoreCreateInfo semaphore_create_info = {};
        semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VkResult result = vkCreateSemaphore(context.device.logical_device, &semaphore_create_info, context.allocator, &context.image_available_semaphores[i]);
        if (result != VK_SUCCESS)
            ERR_EXIT("Failed to create semaphore for image_available_semaphores.\n", "vkCreateSemaphore");

        result = vkCreateSemaphore(context.device.logical_device, &semaphore_create_info, context.allocator, &context.queue_complete_semaphores[i]);
        if (result != VK_SUCCESS)
            ERR_EXIT("Failed to create semaphore for queue_complete_semaphores.\n", "vkCreateSemaphore");

        vulkan_fence_create(&context, true, &context.in_flight_fences[i]);
    }

    // In flight fences should not yet exist at this point, so clear the list. These are stored in pointers
    // because the initial state should be 0, and will be 0 when not in use. Acutal fences are not owned
    // by this list.
    context.images_in_flight = (vulkan_fence **)(malloc(sizeof(vulkan_fence *) * context.swap_chain.image_count));
    for (u32 i = 0; i < context.swap_chain.image_count; ++i)
    {
        context.images_in_flight[i] = 0;
    }
}

//--------------
// Regenerate
//--------------
void destroy_framebuffers();
void destroy_command_buffers(b8);
void destroy_swapchain(vulkan_context *, b8);
void create_frame_buffers();
void create_command_buffers();

b8 recreate_swapchain(GLFWwindow *window, b8 use_cached_framebuffer_size)
{
    if (context.recreating_swapchain)
    {
        printf("recreate_swapchain is called when already creating. Booting.\n");
        return BC_FALSE;
    }

    if (context.framebuffer_height == 0 || context.framebuffer_width == 0)
    {
        printf("recreate swapchain called when window is < 1 in dimension.\n");
        return BC_FALSE;
    }

    context.recreating_swapchain = BC_TRUE;

    // Wait for any operation to complete
    vkDeviceWaitIdle(context.device.logical_device);

    // 1. Destroy old resources first
    destroy_command_buffers(VK_FALSE);
    destroy_framebuffers();
    destroy_swapchain(&context, VK_TRUE);
    for (u32 i = 0; i < context.swap_chain.image_count; ++i)
        context.images_in_flight[i] = 0;

    // TODO: Make sure we have most up-to-date format available
    // vulkan_device_detect_depth_format(&context.device);

    // 2. Create
    /// Swapchain
    if (use_cached_framebuffer_size)
    {
        context.framebuffer_width = cached_framebuffer_width;
        context.framebuffer_height = cached_framebuffer_height;
    }
    create_swap_chain(window, context.framebuffer_width, context.framebuffer_height);
    if (use_cached_framebuffer_size)
    {
        context.main_renderpass.x = 0;
        context.main_renderpass.y = 0;
        context.main_renderpass.w = context.framebuffer_width;
        context.main_renderpass.h = context.framebuffer_height;
        cached_framebuffer_width = 0;
        cached_framebuffer_height = 0;
    }

    // for resizing
    // Update framebuffer size generation.
    // if(use_cached_framebuffer_size)
    context.framebuffer_size_last_generation = context.framebuffer_size_generation;

    create_frame_buffers();
    create_command_buffers();

    /// 3. Completed
    context.recreating_swapchain = BC_FALSE;

    return BC_TRUE;
}

//--------------
// Begin
//--------------
void command_buffer_begin(vulkan_command_buffer *command_buffer)
{
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = 0;
    VkResult result = vkBeginCommandBuffer(command_buffer->handle, &begin_info);
    if (result != VK_SUCCESS)
        ERR_EXIT("Failed to begin recording command buffer", "command_buffer_begin");
    command_buffer->state = COMMAND_BUFFER_STATE_RECORDING;
}

void renderpass_begin(vulkan_command_buffer *command_buffer, vulkan_renderpass *renderpass, u32 image_index)
{
    VkRenderPassBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    begin_info.renderPass = renderpass->handle;
    begin_info.framebuffer = context.swap_chain.framebuffers[image_index].handle;
    begin_info.renderArea.offset.x = renderpass->x;
    begin_info.renderArea.offset.y = renderpass->y;
    begin_info.renderArea.extent = context.swap_chain.extent_2d;
    // begin_info.renderArea.extent.width = renderpass->w;
    // begin_info.renderArea.extent.height = renderpass->h;

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    begin_info.clearValueCount = 1;
    begin_info.pClearValues = &clearColor;

    vkCmdBeginRenderPass(command_buffer->handle, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
    command_buffer->state = COMMAND_BUFFER_STATE_IN_RENDER_PASS;
}

b8 begin_frame(f32 delta_time, GLFWwindow *window)
{
    context.frame_delta_time = delta_time;

    // Check if recreating swap chain and boot out.
    if (context.recreating_swapchain)
    {
        // Make sure device is in idle and is not doing anything so you can proceed.
        VkResult result = vkDeviceWaitIdle(context.device.logical_device);
        if (!vulkan_result_is_success(result))
        {
            ERR_EXIT("vulkan_renderer_backend_begin_frame vkDeviceWaitIdle failed:", vulkan_result_string(result, true));
            return BC_FALSE;
        }
        printf("Recreating swapchain, booting.");
        return BC_FALSE;
    }

    // Check if the framebuffer has been resized. If so, a new swapchain must be created.
    if (context.framebuffer_size_generation != context.framebuffer_size_last_generation)
    {
        VkResult result = vkDeviceWaitIdle(context.device.logical_device);
        if (!vulkan_result_is_success(result))
        {
            ERR_EXIT("vulkan_renderer_backend_begin_frame vkDeviceWaitIdle (2) failed:", vulkan_result_string(result, true));
            return BC_FALSE;
        }

        // If the swapchain recreation failed (because, for example, the window was minimized),
        // boot out before unsetting the flag.
        if (!recreate_swapchain(window, 1))
        {
            return BC_FALSE;
        }

        printf("Resized, booting.");
        return BC_FALSE;
    }

    // Wait for fence
    if (!vulkan_fence_wait(
            &context,
            &context.in_flight_fences[context.current_frame],
            UINT64_MAX))
    {
        printf("WARN: In-flight fence wait failure!"); // not an error but if we start to see too many, we should keep an eye on it,
        return BC_FALSE;
    }

    // vulkan_swapchain_acquire_next_image_index
    // Acquire the next image from the swap chain. Pass along the semaphore that should signaled when this completes.
    // This same semaphore will later be waited on by the queue submission to ensure this image is available.
    VkResult result = vkAcquireNextImageKHR(
        context.device.logical_device, context.swap_chain.handle,
        UINT64_MAX,
        context.image_available_semaphores[context.current_frame],
        VK_NULL_HANDLE,
        &context.image_index);
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    { // Not a failure
        // Trigger swapchain recreation, then boot out of the render loop.
        // The size of the image should match with framebuffer width and height.
        recreate_swapchain(window, 0);
        return BC_FALSE;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        printf("Failed to acquire swapchain image!");
        return BC_FALSE;
    }
    // end: vulkan_swapchain_acquire_next_image_index

    vulkan_fence_reset(&context, &context.in_flight_fences[context.current_frame]);

    // Begin recording commands
    vulkan_command_buffer *command_buffer = &context.graphics_command_buffers[context.image_index];
    // reset first
    vkResetCommandBuffer(command_buffer->handle, 0);
    command_buffer->state = COMMAND_BUFFER_STATE_READY;
    command_buffer_begin(command_buffer);
    context.main_renderpass.w = context.framebuffer_width;
    context.main_renderpass.h = context.framebuffer_height;
    renderpass_begin(command_buffer, &context.main_renderpass, context.image_index);

    return BC_TRUE;
}

//--------------
// Update
//--------------
void update_global_state()
{
    vulkan_command_buffer *command_buffer = &context.graphics_command_buffers[context.image_index];

    // vulkan_object_shader_use
    vkCmdBindPipeline(command_buffer->handle, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    // END: // vulkan_object_shader_use

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (f32)context.framebuffer_width;   // context.swap_chain.extent_2d.width;
    viewport.height = (f32)context.framebuffer_height; // context.swap_chain.extent_2d.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(command_buffer->handle, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent.width = context.framebuffer_width; // context.swap_chain.extent_2d;
    scissor.extent.height = context.framebuffer_height;
    vkCmdSetScissor(command_buffer->handle, 0, 1, &scissor);

    // vulkan_object_shader_update_global_state
    // END: vulkan_object_shader_update_global_state
}

void update_object()
{
    vulkan_command_buffer *command_buffer = &context.graphics_command_buffers[context.image_index];

    vkCmdDraw(command_buffer->handle, 3, 1, 0, 0);
}

void update()
{
    update_global_state();
    update_object();
}
//--------------
// End
//--------------
b8 end_frame(GLFWwindow *window, f32 delta_time)
{
    vulkan_command_buffer *command_buffer = &context.graphics_command_buffers[context.image_index];

    // End renderpass
    vkCmdEndRenderPass(command_buffer->handle);

    // End command buffer
    command_buffer->state = COMMAND_BUFFER_STATE_RECORDING;
    VkResult res = vkEndCommandBuffer(command_buffer->handle);
    if (res != VK_SUCCESS)
        ERR_EXIT("failed to record command buffer!\n", "vkEndCommandBuffer");
    command_buffer->state = COMMAND_BUFFER_STATE_RECORDING_ENDED;

    // Make sure the previous frame is not using this image (i.e. its fence is being waited on)
    if (context.images_in_flight[context.image_index] != VK_NULL_HANDLE)
    {                      // was frame
        vulkan_fence_wait( // Make sure previous frame is not using this image
            &context,
            context.images_in_flight[context.image_index],
            UINT64_MAX);
    }

    // Mark the image fence as in-use by this frame.
    context.images_in_flight[context.image_index] = &context.in_flight_fences[context.current_frame];

    // Reset the fence for use on the next frame
    vulkan_fence_reset(&context, &context.in_flight_fences[context.current_frame]);

    // Submit the command buffer
    // Begin queue submission
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    // Command buffer(s) to be executed.
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &(command_buffer->handle);
    // The semaphore(s) to be signaled when the queue is complete.
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &context.queue_complete_semaphores[context.current_frame];
    // Wait semaphore ensures that the operation cannot begin until the image is available.
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &context.image_available_semaphores[context.current_frame];

    // Each semaphore waits on the corresponding pipeline stage to complete. 1:1 ratio.
    // VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT prevents subsequent colour attachment
    // writes from executing until the semaphore signals (i.e. makes sure we present one frame is presented at a time)
    VkPipelineStageFlags wait_flags[1] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.pWaitDstStageMask = wait_flags;

    // Submit the queue
    VkResult result = vkQueueSubmit(
        context.device.graphicsQueue,
        1,
        &submit_info,
        context.in_flight_fences[context.current_frame].handle);

    if (result != VK_SUCCESS)
    {
        ERR_EXIT("vkQueueSubmit failed with result:", "vkQueueSubmit");
        return BC_FALSE;
    }

    command_buffer->state = COMMAND_BUFFER_STATE_SUBMITTED;
    // end of queue submission

    // Return the image to the swapchain for presentation.
    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &context.queue_complete_semaphores[context.current_frame];
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &context.swap_chain.handle;
    present_info.pImageIndices = &context.image_index;
    present_info.pResults = 0;

    result = vkQueuePresentKHR(context.device.presentQueue, &present_info);
    // TODO: Handle other non-error cases
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        // Swapchain is out of date, suboptimal or a framebuffer resize has occurred. Trigger swapchain recreation.
        recreate_swapchain(window, VK_FALSE);
    }
    else if (result != VK_SUCCESS)
    {
        ERR_EXIT("Failed to present image to swapchain.\n", "vkQueuePresentKHR");
    }

    // Increment (and loop) the index.
    /*
    By using the modulo (%) operator,
    we ensure that the frame index loops around after every MAX_FRAMES_IN_FLIGHT enqueued frames.
    */
    context.current_frame = (context.current_frame + 1) % context.swap_chain.max_frames_in_flight;
    return BC_TRUE;
}

//--------------
// Draw
//--------------
void draw_frame(f32 delta_time, GLFWwindow *window)
{
    if (begin_frame(delta_time, window))
    {
        update();
        b8 res = end_frame(window, delta_time);
        if (!res)
        {
            ERR_EXIT("end_frame failed.\n", "draw_frame()\n.");
        }
    }
}

//--------------
// Destroy
//--------------
void destroy_device(vulkan_device *the_device)
{
    context.device.graphicsQueue = 0;
    context.device.presentQueue = 0;

    // Destroy logical device
    vkDestroyDevice(the_device->logical_device, context.allocator);
    the_device->logical_device = 0;

    // Release physical device resources
    the_device->physical_device = 0;

    // Capabilities
    if (the_device->swapchain_support.formats)
    {
        free(the_device->swapchain_support.formats);
        the_device->swapchain_support.formats = NULL;
        the_device->swapchain_support.format_count = 0;
    }

    if (the_device->swapchain_support.presentationModes)
    {
        free(the_device->swapchain_support.presentationModes);
        the_device->swapchain_support.presentationModes = NULL;
        the_device->swapchain_support.presentation_mode_count = 0;
    }

    memset(&the_device->swapchain_support.surfaceCapabilities, 0, sizeof(the_device->swapchain_support.surfaceCapabilities));
    the_device->graphics_queue_index = -1;
    the_device->present_queue_index = -1;
}

void destroy_swapchain(vulkan_context *context, b8 is_to_recreate)
{
    if (!is_to_recreate)
        vkDeviceWaitIdle(context->device.logical_device);

    // Destroy the depth attachment (image)
    // vulkan_image_destroy(context, &swapchain->depth_attachment);

    for (uint32_t i = 0; i < context->swap_chain.image_count; ++i)
        vkDestroyImageView(context->device.logical_device, context->swap_chain.views[i], context->allocator);

    vkDestroySwapchainKHR(context->device.logical_device, context->swap_chain.handle, context->allocator);
}

void destroy_graphics_pipeline()
{
    vkDestroyPipeline(context.device.logical_device, graphicsPipeline, context.allocator);
    vkDestroyPipelineLayout(context.device.logical_device, pipelineLayout, context.allocator);
}

void destroy_renderpass()
{
    if (context.main_renderpass.handle)
    {
        vkDestroyRenderPass(context.device.logical_device, context.main_renderpass.handle, context.allocator);
        context.main_renderpass.handle = 0;
    }
}
void vulkan_frame_buffer_destroy(vulkan_context *context, vulkan_framebuffer *frame_buffer)
{
    vkDestroyFramebuffer(context->device.logical_device, frame_buffer->handle, context->allocator);
    if (frame_buffer->attachments)
    {
        free(frame_buffer->attachments);
        frame_buffer->attachments = 0;
    }

    frame_buffer->handle = 0;
    frame_buffer->attachment_count = 0;
    frame_buffer->renderpass = 0;
}

void destroy_framebuffers()
{
    for (uint32_t i = 0; i < context.swap_chain.image_count; ++i)
    {
        vulkan_framebuffer fb = context.swap_chain.framebuffers[i];
        vulkan_frame_buffer_destroy(&context, &fb);
    }
}
// Since this is a device related stuff, this can be handled in destroy_device
void destroy_command_pools()
{
    vkDestroyCommandPool(context.device.logical_device, context.device.graphics_command_pool, context.allocator);
    // Destroy for other command pools when necessary
    /*
    // Do this after device destroy for later
    context->device.graphics_queue_index = -1;
    context->device.present_queue_index = -1;
    */
}

void vulkan_command_buffer_free(vulkan_context *context, VkCommandPool *pool, vulkan_command_buffer *command_buffer)
{
    vkFreeCommandBuffers(context->device.logical_device, *pool, 1, &command_buffer->handle);

    command_buffer->handle = 0;
    command_buffer->state = COMMAND_BUFFER_STATE_NOT_ALLOCATED;
}

// You don't need to destroy vkCommandBuffer
// but we have to remove our handles
void destroy_command_buffers(b8 release_memory_resources)
{
    for (u32 i = 0; i < context.swap_chain.image_count; ++i)
    {
        if (context.graphics_command_buffers[i].handle)
        {
            vulkan_command_buffer_free(
                &context,
                &context.device.graphics_command_pool,
                &context.graphics_command_buffers[i]);
            context.graphics_command_buffers[i].handle = 0;
        }
    }

    if (release_memory_resources)
    {
        free(context.graphics_command_buffers);
        context.graphics_command_buffers = 0;
    }
}

void vulkan_fence_destroy(vulkan_context *context, vulkan_fence *fence)
{
    if (fence->handle)
    {
        vkDestroyFence(
            context->device.logical_device,
            fence->handle,
            context->allocator);
        fence->handle = 0;
    }
    fence->is_signaled = false;
}

void destroy_sync_objects()
{
    for (u8 i = 0; i < context.swap_chain.max_frames_in_flight; ++i)
    {
        if (context.image_available_semaphores[i])
        {
            vkDestroySemaphore(
                context.device.logical_device,
                context.image_available_semaphores[i],
                context.allocator);
            context.image_available_semaphores[i] = 0;
        }
        if (context.queue_complete_semaphores[i])
        {
            vkDestroySemaphore(
                context.device.logical_device,
                context.queue_complete_semaphores[i],
                context.allocator);
            context.queue_complete_semaphores[i] = 0;
        }
        vulkan_fence_destroy(&context, &context.in_flight_fences[i]);
    }
    free(context.image_available_semaphores);
    context.image_available_semaphores = 0;

    free(context.queue_complete_semaphores);
    context.queue_complete_semaphores = 0;

    free(context.in_flight_fences);
    context.in_flight_fences = 0;

    free(context.images_in_flight);
    context.images_in_flight = 0;
}
//--------------
// Public
//--------------
int init_renderer(GLFWwindow *window, u32 width, u32 height)
{
    if (init_volk() == EXIT_FAILURE)
        return EXIT_FAILURE;

    cached_framebuffer_width = width;
    cached_framebuffer_height = height;

    create_instance(); // context.frame_buffer size stuff is set here
    setup_debug_messenger();
    create_surface(window);
    get_physical_device();
    create_logical_device();
    create_swap_chain(window, context.framebuffer_width, context.framebuffer_height);
    create_render_pass();
    create_graphics_pipeline();
    create_frame_buffers();
    create_command_pool();
    create_command_buffers();
    create_sync_objects();

    return EXIT_SUCCESS;
}

void cleanup_renderer()
{
    vkDeviceWaitIdle(context.device.logical_device);
    // destroy in reverse order of creation
    destroy_sync_objects();
    destroy_command_buffers(VK_TRUE);
    destroy_command_pools();
    destroy_framebuffers();
    destroy_graphics_pipeline();
    destroy_renderpass();
    destroy_swapchain(&context, 0);
    destroy_device(&context.device);
    vkDestroySurfaceKHR(context.instance, context.surface, context.allocator);

    if (enable_validation_layers)
        DestroyDebugUtilsMessengerEXT(context.instance, debugMessenger, context.allocator);

    vkDestroyInstance(context.instance, context.allocator);
}

//--------------
// Event handlers
//--------------
void renderer_on_resized(int width, int height)
{
    cached_framebuffer_width = width;
    cached_framebuffer_height = height;
    context.framebuffer_size_generation++;
}