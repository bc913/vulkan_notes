#include "vulkan_renderer.h"
#include "log_assert.h"
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

//--------------
// Definitions
//--------------
typedef struct vulkan_context
{
    VkInstance instance;
    VkAllocationCallbacks *allocator;
} vulkan_context;

typedef struct main_device
{
    VkPhysicalDevice physical_device;
    VkDevice logical_device;

} main_device;

// Indices (locations) of Queue Families (if they exist at all)
typedef struct QueueFamilyIndices
{
    int graphicsFamily = -1;     // Location of Graphics Queue Family
    int presentationFamily = -1; // Location of the presentation queue family
} QueueFamilyIndices;

static vulkan_context context;
static main_device mainDevice;
static VkQueue graphicsQueue;
static VkQueue presentQueue;
static VkSurfaceKHR surface;

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

    return 1;
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

// Valdiation
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
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentation_support);
        // Check if queue is presentation type (can be both graphics and presentation)
        if (queue_families[i].queueCount > 0 && presentation_support)
            indices.presentationFamily = i;

        // Check if queue family indices are in a valid state, stop searching if so
        if (is_valid_queue_family_indices(indices))
            break;
    }

    return indices;
}

// DEvice
// ###############
VkBool32 check_device_suitable(VkPhysicalDevice device)
{
    /*
    // Information about the device itself (ID, name, type, vendor, etc)
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    // Information about what the device can do (geo shader, tess shader, wide lines, etc)
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
    */

    return is_valid_queue_family_indices(get_queue_families(device));
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

//--------------
// Private
//--------------

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
// Swapchain - Surface
//--------------
void create_surface(GLFWwindow *window)
{
    if (glfwCreateWindowSurface(context.instance, window, context.allocator, &surface) != VK_SUCCESS)
        ERR_EXIT(
            "Failed to create window surface.\n",
            "glfwCreateWindowSurface() Failure");
}

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
        if (check_device_suitable(physical_devices[i]))
        {
            mainDevice.physical_device = physical_devices[i];
            break;
        }
    }

    if (mainDevice.physical_device == VK_NULL_HANDLE)
        ERR_EXIT("Failed to find a suitable GPU.\n", "get_physical_device");
}

void create_logical_device()
{
    // Get the queue family indices for the chosen Physical Device
    QueueFamilyIndices indices = get_queue_families(mainDevice.physical_device);

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
    deviceCreateInfo.queueCreateInfoCount = indices_count;   // Number of Queue Create Infos (number of queues)
    deviceCreateInfo.pQueueCreateInfos = queue_create_infos; // List of queue create infos so device can create required queues
    deviceCreateInfo.enabledExtensionCount = 0;              // Number of enabled logical device extensions
    deviceCreateInfo.ppEnabledExtensionNames = nullptr;      // List of enabled logical device extensions

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
    VkResult result = vkCreateDevice(mainDevice.physical_device, &deviceCreateInfo, nullptr, &mainDevice.logical_device);
    if (result != VK_SUCCESS)
        ERR_EXIT("Failed to create a Logical Device!\n", "create_logical_device");

    // Queues are created at the same time as the device...
    // So we want handle to queues
    // From given logical device, of given Queue Family, of given Queue Index (0 since only one queue), place reference in given VkQueue
    vkGetDeviceQueue(mainDevice.logical_device, indices.graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(mainDevice.logical_device, indices.presentationFamily, 0, &presentQueue);

    free(unique_queue_families);
    free(queue_create_infos);
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

    return EXIT_SUCCESS;
}

void cleanup_renderer()
{
    if (enable_validation_layers)
        DestroyDebugUtilsMessengerEXT(context.instance, debugMessenger, context.allocator);

    vkDestroyDevice(mainDevice.logical_device, context.allocator);
    vkDestroySurfaceKHR(context.instance, surface, context.allocator);
    vkDestroyInstance(context.instance, context.allocator);
}