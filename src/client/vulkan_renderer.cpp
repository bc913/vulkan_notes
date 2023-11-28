#include "vulkan_renderer.h"
#include "log_assert.h"
#include "utils.h"

// Should come before includes
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

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>

//--------------
// Definitions
//--------------

typedef struct SwapChainDetails
{
    VkSurfaceCapabilitiesKHR surfaceCapabilities; // Surface properties, e.g. image size/extent
    uint32_t format_count;
    VkSurfaceFormatKHR *formats; // Surface image formats, e.g. RGBA and size of each colour

    uint32_t presentation_mode_count;
    VkPresentModeKHR *presentationModes; // How images should be presented to screen
} SwapChainDetails;

typedef struct vulkan_swapchain
{
    VkSurfaceFormatKHR surface_format;
    VkSwapchainKHR handle;
    VkImage *images;
    VkImageView *views;
} vulkan_swapchain;

typedef struct main_device
{
    VkPhysicalDevice physical_device;
    VkDevice logical_device;
    SwapChainDetails swapchain_support;
} main_device;

typedef struct vulkan_context
{
    VkInstance instance;
    VkAllocationCallbacks *allocator;
    VkSurfaceKHR surface;
    vulkan_swapchain swap_chain;
    main_device device;
} vulkan_context;

// Indices (locations) of Queue Families (if they exist at all)
typedef struct QueueFamilyIndices
{
    int graphicsFamily = -1;     // Location of Graphics Queue Family
    int presentationFamily = -1; // Location of the presentation queue family
} QueueFamilyIndices;

static vulkan_context context;
static VkQueue graphicsQueue;
static VkQueue presentQueue;

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
VkBool32 is_valid_queue_family_indices(QueueFamilyIndices qfi)
{
    return qfi.graphicsFamily >= 0 && qfi.presentationFamily >= 0;
}
QueueFamilyIndices get_queue_families(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;

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
            indices.graphicsFamily = i;

        // Check if Queue Family supports presentation
        VkBool32 presentation_support = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, context.surface, &presentation_support);
        // Check if queue is presentation type (can be both graphics and presentation)
        if (queue_families[i].queueCount > 0 && presentation_support)
            indices.presentationFamily = i;

        // Check if queue family indices are in a valid state, stop searching if so
        if (is_valid_queue_family_indices(indices))
            break;
    }

    return indices;
}

// Swap-chain - surface
// ###############
SwapChainDetails get_swap_chain_details(VkPhysicalDevice device)
{
    SwapChainDetails details;

    // Get the surface capabilities for the given surface on the given physical device
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, context.surface, &details.surfaceCapabilities);

    // formats
    uint32_t formats_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, context.surface, &formats_count, NULL);
    if (formats_count)
    {
        details.format_count = formats_count;
        details.formats = (VkSurfaceFormatKHR *)(malloc(sizeof(VkSurfaceFormatKHR) * formats_count));
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, context.surface, &formats_count, details.formats);
    }

    // Presentation modes
    uint32_t presentation_modes_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, context.surface, &presentation_modes_count, NULL);
    if (presentation_modes_count)
    {
        details.presentation_mode_count = presentation_modes_count;
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
}

void create_logical_device()
{
    // Get the queue family indices for the chosen Physical Device
    QueueFamilyIndices indices = get_queue_families(context.device.physical_device);

    // For each indices, it requires a queue ->std::unordered_set can be used here
    // No duplicate indices should be alive
    // TODO: We don't have hash_set impl yet so use this manual approach
    size_t indices_count = indices.graphicsFamily == indices.presentationFamily ? 1 : 2;

    int *unique_queue_families = (int *)(malloc(sizeof(int) * indices_count));
    if (indices_count == 1)
        unique_queue_families[0] = indices.graphicsFamily;
    else if (indices_count == 2)
    {
        unique_queue_families[0] = indices.graphicsFamily;
        unique_queue_families[1] = indices.presentationFamily;
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
    vkGetDeviceQueue(context.device.logical_device, indices.graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(context.device.logical_device, indices.presentationFamily, 0, &presentQueue);

    free(unique_queue_families);
    free(queue_create_infos);
}

void create_swap_chain(GLFWwindow *window)
{
    SwapChainDetails details = get_swap_chain_details(context.device.physical_device);

    // Choose the best values for the swap chain
    context.swap_chain.surface_format = choose_best_surface_format(details.formats, details.format_count);
    VkPresentModeKHR presentation_mode = choose_best_presentation_mode(details.presentationModes, details.presentation_mode_count);
    VkExtent2D extent = choose_swap_extent(window, &details.surfaceCapabilities);

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

    // Creation information for swap chain
    VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface = context.surface;                                      // Swapchain surface
    swapChainCreateInfo.imageFormat = context.swap_chain.surface_format.format;         // Swapchain format
    swapChainCreateInfo.imageColorSpace = context.swap_chain.surface_format.colorSpace; // Swapchain colour space
    swapChainCreateInfo.presentMode = presentation_mode;                                // Swapchain presentation mode
    swapChainCreateInfo.imageExtent = extent;                                           // Swapchain image extents
    swapChainCreateInfo.minImageCount = image_count;                                    // Minimum images in swapchain
    // always 1 unless you are developing a stereoscopic 3D application
    swapChainCreateInfo.imageArrayLayers = 1;                                        // Number of layers for each image in chain
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;            // What attachment images will be used as
    swapChainCreateInfo.preTransform = details.surfaceCapabilities.currentTransform; // Transform to perform on swap chain images
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;          // How to handle blending images with external graphics (e.g. other windows)
    swapChainCreateInfo.clipped = VK_TRUE;                                           // Whether to clip parts of image not in view (e.g. behind another window, off screen, etc)

    // Get queue family indices
    QueueFamilyIndices indices = get_queue_families(context.device.physical_device);

    // If Graphics and Presentation families are different (quite unlikely but we have to handle that possibility), then swapchain must let images be shared between families
    if (indices.graphicsFamily != indices.presentationFamily)
    {
        // Queues to share between
        uint32_t queueFamilyIndices[] = {
            (uint32_t)indices.graphicsFamily,
            (uint32_t)indices.presentationFamily};

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

    // Now we have to retrieve the images
}
//--------------
// Destroy
//--------------
void destroy_device(main_device *the_device)
{
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
}

//--------------
// Public
//--------------
int init_renderer(GLFWwindow *window)
{
    if (init_volk() == EXIT_FAILURE)
        return EXIT_FAILURE;

    create_instance();
    setup_debug_messenger();
    create_surface(window);
    get_physical_device();
    create_logical_device();
    create_swap_chain(window);

    return EXIT_SUCCESS;
}

void cleanup_renderer()
{
    if (enable_validation_layers)
        DestroyDebugUtilsMessengerEXT(context.instance, debugMessenger, context.allocator);

    vkDestroySwapchainKHR(context.device.logical_device, context.swap_chain.handle, context.allocator);
    destroy_device(&context.device);
    vkDestroySurfaceKHR(context.instance, context.surface, context.allocator);
    vkDestroyInstance(context.instance, context.allocator);
}