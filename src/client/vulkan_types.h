#ifndef VULKAN_NOTES_1702350587_VULKAN_TYPES_H
#define VULKAN_NOTES_1702350587_VULKAN_TYPES_H

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

#include "defines.h"

typedef struct vulkan_renderpass
{
    VkRenderPass handle;
    // render area
    // an area of the image to render to
    f32 x, y, w, h;
    // Clear colors
    f32 r, g, b, a;

    f32 depth;
    u32 stencil;

    // vulkan_render_pass_state state;
} vulkan_renderpass;

typedef struct vulkan_framebuffer
{
    VkFramebuffer handle;
    u32 attachment_count;
    VkImageView *attachments;
    vulkan_renderpass *renderpass;
} vulkan_framebuffer;

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
    VkExtent2D extent_2d; // size of the swapchain images
    VkSwapchainKHR handle;
    uint32_t image_count;
    VkImage *images;
    VkImageView *views;

    vulkan_framebuffer *framebuffers;
} vulkan_swapchain;

typedef struct vulkan_device
{
    VkPhysicalDevice physical_device;
    VkDevice logical_device;
    SwapChainDetails swapchain_support;

    VkQueue graphicsQueue;
    VkQueue presentQueue;
} vulkan_device;

typedef struct vulkan_context
{
    VkInstance instance;
    VkAllocationCallbacks *allocator;
    VkSurfaceKHR surface;
    vulkan_swapchain swap_chain;
    vulkan_device device;
    vulkan_renderpass main_renderpass;
} vulkan_context;

// Indices (locations) of Queue Families (if they exist at all)
typedef struct QueueFamilyIndices
{
    int graphicsFamily = -1;     // Location of Graphics Queue Family
    int presentationFamily = -1; // Location of the presentation queue family
} QueueFamilyIndices;

#endif