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


### Presentation
- Displaying images in Vulkan is not supported by default. WSI (Window system integration) extensions should be enabled before.
Some brief explanation: Swapchain will generate and store some image (data) and it has to present these images (data) onto the platform agnostic `VkSurfaceKHR` surface which is backed by the windowing system and acts such an interface. GLFW will be reading from this surface.

> Vulkan does NOT have the concept of `default frame buffer` which OPENGL offers. This is handled via `swapchain` for Vulkan.
#### Swapchain
It is a group of images (data) that can be drawn to and presented to surface. `Swapchain` is a complex object that handles the retrieval and updating the data of the image(s) being displayed or yet to be displayed. It does NOT display the image. It only handles the data about it. It requires `VK_KHR_swapchain` extension which is device level extension.

> GLFW is already providing that extensions using `glfwGetRequiredInstanceExtensions()` method.

- How it works?:
You should query to the swapchain: `Hey swapchain, do you have new image (data) to be drawn?`. If yes, draw the image and tell swapchain to queue it up to the presentation queue to be presented on the surface. It requires some synchronizations:
1.  If we ask a new image, we don't want to present that image before it's drawing is completed.
2. ???

```cpp
static const char *requested_device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
static uint32_t requested_device_ext_count = 1;

// Since, it is a physical device feature and not supported by all available hardware
// Check for the swap chain support on the physical device
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
    uint32_t requested_device_ext_count = 1;
    const char *requested_device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkBool32 result = check_extensions(requested_device_ext_count, requested_device_extensions, extension_count, extensions);

    free(extensions);
    return result;
}
// Enable extension at the logical device level
deviceCreateInfo.enabledExtensionCount = requested_device_ext_count;    // Number of enabled logical device extensions
deviceCreateInfo.ppEnabledExtensionNames = requested_device_extensions;

// Query details of swap chain support
/*
Just checking if a swap chain is available is not sufficient, because it may not actually be compatible with our window surface.
*/

SwapChainDetails get_swap_chain_details(VkPhysicalDevice device)
{
    SwapChainDetails details;

    // Get the surface capabilities for the given surface on the given physical device
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.surfaceCapabilities);

    // formats
    uint32_t formats_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formats_count, NULL);
    if (formats_count)
    {
        details.format_count = formats_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formats_count, details.formats);
    }

    // Presentation modes
    uint32_t presentation_modes_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentation_modes_count, NULL);
    if (presentation_modes_count)
    {
        details.presentation_mode_count = presentation_modes_count;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentation_modes_count, details.presentationModes);
    }

    return details;
}

// For some reason it might be possible that any surface capability or format is not supported
// so we have to check if our device supports these

VkBool32 check_device_suitable(VkPhysicalDevice device, SwapChainDetails *details)
{
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

// OPTIONAL
// Choosing the right settings for the swap chain

```

Swapchain is based on Surface so first Surface query might be done before. Swapchain has 3 major parts:
- Surface Capabilities:
- Surface Format:
- [Presentation Mode](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPresentModeKHR.html): It is a concept about the order and timing of images being presented to the Surface. At any time, there can be ONLY 1 image present at the surface.

- How monitors work?
Monitors start drawing from top-left pixel as row-by-row until reaches bottom-right corner. When the image completes, the screen is then cleared to start drawing again. The period of time AFTER this clear and BEFORE it starts drawing again is called as `Vertical Blank Interval` or `Vertical Blank`. This is usually the best time to replace the image.

- What is tearing?
When an image is being drawn to the screen, if another image is started to being drawn to the screen it is tearing.

- Images and Image Views:
When Swapchain is created, it will automatically create a set of images to be

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


### Graphics Pipeline
> The graphics pipeline in Vulkan is almost completely immutable, so you must recreate the pipeline from scratch if you want to change shaders, bind different framebuffers or change the blend function. (Dynamic states can be used to overcome) The disadvantage is that you'll have to create a number of pipelines that represent all of the different combinations of states you want to use in your rendering operations. However, because all of the operations you'll be doing in the pipeline are known in advance, the driver can optimize for it much better. 

#### Tessellation:
Allows you to subdivide geometry based on certain rules to increase the mesh quality. It can be used to generate brick walls, staircases, waves or ocean effects

#### Geometry Shader:
Can be used for shadowing.

#### Shaders and SPIR-V
There are two ways handling shaders:
1. Load the raw shader code and compile it into shader in runtime. (do NOT implement this in Vulkan)
2. Pre-compile the shader code to intermediate code named `SPIR-V` and load into a shader module.
    - compilation will generate `.spv` as binary file.
    - Then load that binary file into the shader module.
The shader syntax is compiled for GLSL. `SPIR-V` can be compiled using tool `glslangValidator.exe` which is part of the VULKAN SDK.

- https://github.com/KhronosGroup/SPIRV-Reflect
- https://github.com/KhronosGroup/SPIRV-Headers
- https://github.com/KhronosGroup/SPIRV-Tools
- https://github.com/google/shaderc


- [Converting shader source to SPIR-V on the fly without having to use command line tools](https://www.reddit.com/r/vulkan/comments/ecnjn7/converting_shader_source_to_spirv_on_the_fly/?rdt=35539)
#### Renderpaass
It is something that holds a pipeline and then handles how it is executed. You can multiple smaller subpasses inside the render pass that each subpass can use a different pipeline.

#### Framebuffers
They are connection between an image (or images) and the renderpass. Interface to an image
    - Attach image(s) to a framebuffer
    - renderpass then outputs fragment data from a pipeline's execution to the images bound to the framebuffer's attachments
> Framebuffer images line up 1-to-1 with the attachments in the renderpass so the order should match
- Framebuffers has a ref to attachments that are rendered to. They are used to relate data back to swapchain. Framebuffer glues things together.

- Anytime you changed the window size, you need to regenerate the framebuffers.
- It is always required a framebuffer for swapchain image.

#### Command buffers
Unlike opengl where one command is submitted to the GPU at a time, Vulkan works by pre-recording a group of commands, and then submitting them all to a queue at once. The commands are recorded in the command buffer and the command buffer(s) can be (re)used by the queue.

Commands to draw something will usually in the form:
    1. Start a renderpass
    2. Bind a pipeline
    3. Bind vertex/index data
    4. Bind descriptor sets and push constants
    5. Draw (output to whatever images are at the frame buffer that our sub pass is connected to)

> One can also submit a command to begin a new subpass, but you will need to bind the appropriate pipeline again since each subpass requires a separate pipeline.
- It is always required a command buffer for each swapchain image.

Command buffers are stored in command pools and in order to execute the commands within, the command buffers should be submitted to the appropriate queue.

- **Semaphores** flags that say if a resource can be accessed. If they are signalled (set to true) then the resource is available to use. If it is set to false (unsignalling), then the resource is in use, and the program must wait.
> Semaphores are sync mechanism ONLY between GPU  functions. Yes, you can create them on CPU side but can't control them after creation.
Use cases for semaphores:
    - when an image has become available following presentation. (the gpu will not draw until it is available)
    - when an image has finished being rendered and is therefore ready to present, we want to let GPU know that it's ready to be presented. (we don't want to present when the image is still being rendered. As soon as it's finished being rendering, it will signal to say that it's finished being rendering and then it will present it.)

- **Fences**: similar to semaphores with the ability to achieve sync between CPU and GPU. (allowing us to block on the CPU) It is still GPU's responsibility to signal (set to true) to say that `resource is available` but it is up to the CPU now to unsignal.
    Use cases:
        - `vkWaitForFences`: This will block the CPU code until the GPU signals the fence.
        - `vkResetFence`: This will unsignal a fence until the gpu signals it again.

Usually they are used next to each other. Fences are used to ensure a frame is available so we don't accidentally flood the queue.


## References
### General
- [vulkan_best_practice_for_mobile_developers](https://arm-software.github.io/vulkan_best_practice_for_mobile_developers/samples/vulkan_basics.html#:~:text=OpenGL%20ES%20uses%20a%20single%2Dthreaded%20rendering%20model%2C%20which%20severely,operations%20across%20multiple%20CPU%20cores.)

- [Using Vulkan Hpp, The C++ Header for Vulkan](https://www.youtube.com/watch?v=KFsbrhcqW2U)

- [Intel - API Without Secrets to Vulkan](https://www.intel.com/content/www/us/en/developer/articles/training/api-without-secrets-introduction-to-vulkan-part-1.html)
- [Vulkan Guide](https://vkguide.dev/)
- [Vulkan Compute](https://vulkan.org/user/pages/09.events/vulkanised-2023/vulkanised_2023_transitioning_to_vulkan_for_compute.pdf)

### Vulkan Objects
- [Understanding Vulkan objects](https://gpuopen.com/learn/understanding-vulkan-objects/)
- [Vulkan Render target](https://i.stack.imgur.com/K0NRD.png)

### Dev Environment
- [Vulkan SDK for CI/CD](https://www.reddit.com/r/vulkan/comments/h9exvb/vulkansdk_for_ci/)
- [Vulkan Tutorial - Development Environment](https://vulkan-tutorial.com/Development_environment)

### Repositories
- [Vulkan Samples Tutorial](https://vulkan.lunarg.com/doc/view/latest/windows/tutorial/html/index.html)
- [KhronosGroup/VulkanSamples](https://github.com/KhronosGroup/Vulkan-Samples)
- [Vulkan C++ examples and demos - SaschaWillems/Vulkan](https://github.com/SaschaWillems/Vulkan)
- [Intel -> GameTechDev/IntroductionToVulkan](https://github.com/GameTechDev/IntroductionToVulkan)
- [googlesamples/vulkan-basic-samples](https://github.com/googlesamples/vulkan-basic-samples)
- [PacktPublishing/Learning-Vulkan](https://github.com/PacktPublishing/Learning-Vulkan)
- [blurrypiano/littleVulkanEngine](https://github.com/blurrypiano/littleVulkanEngine)
- [volk - meta-loader for Vulkan](https://github.com/zeux/volk)
- [krOoze/Hello_Triangle](https://github.com/krOoze/Hello_Triangle)
- [tksuoran/Vulkan-CPPTemplate](https://github.com/tksuoran/Vulkan-CPPTemplate)
- [sjfricke/Vulkan-NDK-Template](https://github.com/sjfricke/Vulkan-NDK-Template)

### CMake
- [FindVulkan - Doc](https://cmake.org/cmake/help/v3.25/module/FindVulkan.html)
- [FindVulkan - src](https://gitlab.kitware.com/cmake/cmake/blob/master/Modules/FindVulkan.cmake)


https://github.com/waitin2010/awesome-computer-graphics/blob/master/subjects/water.md
https://www.vulkan.org/learn#blogs -> news to programming

https://farside.ph.utexas.edu/teaching/336L/Fluidhtml/Fluidhtml.html
