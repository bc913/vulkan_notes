# Vulkan Notes


## Dev Environment
- CMake (>= 3.25)
- `Vulkan`
    1. Without `Vulkan SDK` installation: There are several ways to achieve
    - Requirement: Make sure an appropriate loader is available. [volk](https://github.com/zeux/volk) and [glad](https://github.com/Dav1dde/glad) are the most popular ones.
    - Option 1: Bring header-only [`Vulkan-Headers`](https://github.com/KhronosGroup/Vulkan-Headers) using CMake `FetchContent`, `git submodules` or package managers such as `vcpkg` or `conan`. This is C api and provide header files for C and C++.
    - Option 2: Use [`Vulkan-Hpp C++ API`](https://github.com/KhronosGroup/Vulkan-Hpp)
    2. Using `Vulkan SDK` installation:
        - Install Vulkan SDK and make sure `VULKAN_SDK` environment variable is set.
        - Call `find_package(VULKAN REQUIRED)` (through [FindVulkan](https://cmake.org/cmake/help/v3.25/module/FindVulkan.html))
        ```cmake
        find_package(VULKAN REQUIRED)
        target_link_libraries(main PRIVATE Vulkan::Vulkan)
        ```
    3. Alternative way [Requires testing]:
        - [Do I need the Vulkan SDK](https://doc.magnum.graphics/magnum/platforms-vk.html)
        - [Vulkan loader](https://github.com/KhronosGroup/Vulkan-Loader/blob/main/BUILD.md#building-on-windows)

> You can run `vulkaninfo` or `vulkaninfo > <some_file_name>.txt` in cmd line to get your grpahics card information through Vulkand 

## Vulkan
### Introduction
- **Vulkan Instance**: It is used to access to Vulkan context. Once initialized rarely used throughout the rest of the program:

### Device
- **Physical Device**: The GPU itself. Holds memory and queues to process pipeline but can't be accessed directly. They can ONLY be `retrieved` and can NOT be created. Instance can iterate all the available physical devices.
    - Memory: To allocate resources 
    - Queues: Command queues (FIFO). Different types of queues. (Queue Families). There can be more than one family and each family can have one or multiple queues. There are four types of queues.
        - `Graphics`: to process graphics commands
        - `Compute`: To process compute shaders
        - `Transfer`: Data transfer ops. i.e. copy some data within the GPU memory from one location to another
    - When iterating over the `Physical Devices`, the most suitable device can be selected based on one or more criterias given below:
        1. Queue family availability
        2. Based on device props and features
        ```cpp
        bool isDeviceSuitable(VkPhysicalDevice device) {
            VkPhysicalDeviceProperties deviceProperties;
            VkPhysicalDeviceFeatures deviceFeatures;
            vkGetPhysicalDeviceProperties(device, &deviceProperties);
            vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

            // i.e. Only consider dedicated graphics cards with geometry shader support.
            return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
                deviceFeatures.geometryShader;
        }
        ```
        3. Score system based on requirements thus the physical device with the highest score can be picked but fall back to an integrated GPU if that's the only one.
        ```cpp
        int rateDeviceSuitability(VkPhysicalDevice device) {
            ...

            int score = 0;

            // Discrete GPUs have a significant performance advantage
            if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                score += 1000;
            }

            // Maximum possible size of textures affects graphics quality
            score += deviceProperties.limits.maxImageDimension2D;

            // Application can't function without geometry shaders
            if (!deviceFeatures.geometryShader) {
                return 0;
            }

            return score;
        }
        ```

- **Queue Families**: Almost every operation in Vulkan requires commands to be submitted to a queue and we need to find which of those required are supported by the physical device.

> The currently available drivers will only allow you to create a small number of queues for each queue family and you don't really need more than one. That's because you can create all of the command buffers on multiple threads and then submit them all at once on the main thread with a single low-overhead call.

- **Logical Device**: An handle(interface) to the physical device. All the work is done through this.
    - Most objects are created on this device
    - Assign queue families supported by the physical device and number of queues on it
    - Multiple logical device can point (reference) the same physical device
> Whenever a device and/or instance is created, they should be destroyed as well in reverse order. 

### Validation
Validation layers exists on the instance level. The device level validation layers are deprecated but still recommended to add for backwards compatibility of some devices. Validation layers are usually enabled with `DEBUG` builds. Most fundamental validation layer name is `VK_LAYER_KHRONOS_validation`. Initially, only using this layer would suffice.

- DebugCallbacks are should be registered during instance creation and destroyed properly. It should be created after instance creation and destroyed before instance destroy.

### Extensions


#### Surface:
An interface between the surface backed by a windowing system and images(objects) to be displayed (front) in the swapchain. The surface should specifically be created for the windowing system we use. It requires `VK_KHR_surface` instance level extension.

> Its usage is platform agnostic but creation is not so we need some platform information. However if you use glfw, you don't need to use platform specific info for creation too.

Follow this [link](https://www.intel.com/content/www/us/en/developer/articles/training/api-without-secrets-introduction-to-vulkan-part-2.html) to use platform specific info for surface creation. 

Since it is specific to the windowing system, it requires some platform-specific attention to create it. Thanks to `GLFW`, `glfwCreateWindowSurface()` method generates the surface we need in a platform specific way.

> In order to present the image in the swapchain to the surface, a queue is required to handle present operations. This is `Presentation queue`. It is not like the other queues but it has some required features as `Graphics queue`. 
> Usually, the `Presentation` and `Graphics` queues  will usually be the same queue. 

- Usage:
```cpp
static VkSurfaceKHR surface;

// Make sure VK_KHR_surface instance level extension is supported during instance creation
const char **required_extensions = glfwGetRequiredInstanceExtensions(&required_extension_count);

// Logical device should also support for surfaces
// We don't need to check the extension directly on the device level.
// Just check if the logical devices supports a queue type (Presentation) which the surface requires
// Which is usually the same as the Graphics queue
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

// This is to  make sure that our logical device actually supports presenting to a surface

// Pass that info to the logical device
// Information to create logical device (sometimes called "device")
VkDeviceCreateInfo deviceCreateInfo = {};
deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
deviceCreateInfo.queueCreateInfoCount = indices_count;   // Number of Queue Create Infos (number of queues)
deviceCreateInfo.pQueueCreateInfos = queue_create_infos; // List of queue create infos so device can create required queues
VkResult result = vkCreateDevice(mainDevice.physical_device, &deviceCreateInfo, nullptr, &mainDevice.logical_device);

// Create Surface
void create_surface(GLFWwindow *window)
{
    if (glfwCreateWindowSurface(context.instance, window, context.allocator, &surface) != VK_SUCCESS)
        ERR_EXIT(
            "Failed to create window surface.\n",
            "glfwCreateWindowSurface() Failure");
}

// Destroy in reverse order so before destroy instance
vkDestroySurfaceKHR(context.instance, surface, context.allocator);
```

## References
### General
- [vulkan_best_practice_for_mobile_developers](https://arm-software.github.io/vulkan_best_practice_for_mobile_developers/samples/vulkan_basics.html#:~:text=OpenGL%20ES%20uses%20a%20single%2Dthreaded%20rendering%20model%2C%20which%20severely,operations%20across%20multiple%20CPU%20cores.)

- [Using Vulkan Hpp, The C++ Header for Vulkan](https://www.youtube.com/watch?v=KFsbrhcqW2U)

### Dev Environment
- [Vulkan SDK for CI/CD](https://www.reddit.com/r/vulkan/comments/h9exvb/vulkansdk_for_ci/)
- [Vulkan Tutorial - Development Environment](https://vulkan-tutorial.com/Development_environment)

### Repositories
- [KhronosGroup/VulkanSamples](https://github.com/KhronosGroup/Vulkan-Samples)
- [googlesamples/vulkan-basic-samples](https://github.com/googlesamples/vulkan-basic-samples)
- [volk - meta-loader for Vulkan](https://github.com/zeux/volk)
- [krOoze/Hello_Triangle](https://github.com/krOoze/Hello_Triangle)
- [tksuoran/Vulkan-CPPTemplate](https://github.com/tksuoran/Vulkan-CPPTemplate)
- [sjfricke/Vulkan-NDK-Template](https://github.com/sjfricke/Vulkan-NDK-Template)
