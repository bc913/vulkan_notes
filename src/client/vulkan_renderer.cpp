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
    int graphicsFamily = -1; // Location of Graphics Queue Family
} QueueFamilyIndices;

static vulkan_context context;
static main_device mainDevice;
static VkQueue graphicsQueue;

//--------------
// Helpers
//--------------
VkBool32 is_valid_queue_family_indices(QueueFamilyIndices qfi)
{
    return qfi.graphicsFamily >= 0;
}

VkBool32 check_extensions(uint32_t check_count, const char **check_names,
                          uint32_t extension_count, VkExtensionProperties *extensions)
{
    uint32_t i = 0, j;
    for (; i < check_count; ++i)
    {
        VkBool32 found = 0;
        for (j = 0; j < extension_count; ++j)
        {
            if (!strcmp(check_names[i], extensions[j].extensionName))
            {
                found = 1;
                break;
            }
        }

        if (!found)
        {
            fprintf(stderr, "Can not find extension: %s\n", check_names[i]);
            return 0;
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

    return check_extensions(check_count, check_names, extensionCount, extensions);
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

        // Check if queue family indices are in a valid state, stop searching if so
        if (is_valid_queue_family_indices(indices))
            break;
    }

    return indices;
}

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
// Private
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

void create_instance()
{
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
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = required_extension_count;
    createInfo.ppEnabledExtensionNames = required_extensions;
    // TODO: No validation layers at the moment
    createInfo.enabledLayerCount = 0;
    createInfo.ppEnabledLayerNames = NULL;

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
}

void create_logical_device()
{
    // Get the queue family indices for the chosen Physical Device
    QueueFamilyIndices indices = get_queue_families(mainDevice.physical_device);

    // Queue the logical device needs to create and info to do so (Only 1 for now, will add more later!)
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = indices.graphicsFamily; // The index of the family to create a queue from
    queueCreateInfo.queueCount = 1;                            // Number of queues to create
    float priority = 1.0f;
    queueCreateInfo.pQueuePriorities = &priority; // Vulkan needs to know how to handle multiple queues, so decide priority (1 = highest priority)

    // Information to create logical device (sometimes called "device")
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;             // Number of Queue Create Infos (number of queues)
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo; // List of queue create infos so device can create required queues
    deviceCreateInfo.enabledExtensionCount = 0;            // Number of enabled logical device extensions
    deviceCreateInfo.ppEnabledExtensionNames = nullptr;    // List of enabled logical device extensions

    // Physical Device Features the Logical Device will be using
    VkPhysicalDeviceFeatures deviceFeatures = {};

    deviceCreateInfo.pEnabledFeatures = &deviceFeatures; // Physical Device features Logical Device will use

    // Create the logical device for the given physical device
    VkResult result = vkCreateDevice(mainDevice.physical_device, &deviceCreateInfo, nullptr, &mainDevice.logical_device);
    if (result != VK_SUCCESS)
        ERR_EXIT("Failed to create a Logical Device!\n", "create_logical_device");

    // Queues are created at the same time as the device...
    // So we want handle to queues
    // From given logical device, of given Queue Family, of given Queue Index (0 since only one queue), place reference in given VkQueue
    vkGetDeviceQueue(mainDevice.logical_device, indices.graphicsFamily, 0, &graphicsQueue);
}
//--------------
// Public
//--------------
int init_renderer()
{
    if (init_volk() == EXIT_FAILURE)
        return EXIT_FAILURE;

    create_instance();
    get_physical_device();
    create_logical_device();

    return EXIT_SUCCESS;
}

void cleanup_renderer()
{
    vkDestroyDevice(mainDevice.logical_device, NULL);
    vkDestroyInstance(context.instance, context.allocator);
}