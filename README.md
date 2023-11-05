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

## Vulkan
### Introduction
- **Vulkan Instance**: It is used to access to Vulkan context. Once initialized rarely used throughout the rest of the program:
- **Device**:
    - **Physical Device**: The GPU itself. Holds memory and queues to process pipeline but can't be accessed directly. They can ONLY be `retrieved` and can NOT be created. Instance can iterate all the available physical devices.
        - Memory: To allocate resources 
        - Queues: Command queues (FIFO). Different types of queues. (Queue Families). There can be more than one family and each family can have one or multiple queues. There are four types of queues.
            - `Graphics`: to process graphics commands
            - `Compute`: To process compute shaders
            - `Transfer`: Data transfer ops. i.e. copy some data within the GPU memory from one location to another
        > When iterate over the available `Physical Devices` you need to check the device has the queue families you need for the app.
    - **Logical Device**: An handle(interface) to the physical device. All the work is done through this.
        - Most objects are created on this device
        - Define queue families and number of queues on it
        - Multiple logical device can point (reference) the same physical device
> Whenever a device and/or instance is created, they should be destroyed as well in reverse order. 

### Validation


### Extensions


### Swapchain

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