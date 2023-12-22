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

// vulkan_swapchain_support_info
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

    // vulkan_image depth_attachment;
    vulkan_framebuffer *framebuffers;

    u8 max_frames_in_flight; //
} vulkan_swapchain;

typedef enum vulkan_command_buffer_state
{
    COMMAND_BUFFER_STATE_READY,
    /*
    states the point when the commands are issued
    to the command buffer.
    */
    COMMAND_BUFFER_STATE_RECORDING,
    /*

    */
    COMMAND_BUFFER_STATE_IN_RENDER_PASS,
    /* states that recording is over*/
    COMMAND_BUFFER_STATE_RECORDING_ENDED,
    /* all the commands in the buffer has been completely
    executed. Occurs after RECORDING_ENDED state.
    */
    COMMAND_BUFFER_STATE_SUBMITTED,
    COMMAND_BUFFER_STATE_NOT_ALLOCATED
} vulkan_command_buffer_state;

// Holds a list of commands to be executed by a queue
typedef struct vulkan_command_buffer
{
    VkCommandBuffer handle;

    // Command buffer state.
    vulkan_command_buffer_state state;
} vulkan_command_buffer;

typedef struct vulkan_fence
{
    VkFence handle;
    /**
     * Happens whenever we are waiting for a fence
     * and then that fence get signalled bec the operation has completed
     *
     */
    b8 is_signaled;
} vulkan_fence;

typedef struct vulkan_device
{
    VkPhysicalDevice physical_device;
    VkDevice logical_device;
    SwapChainDetails swapchain_support;

    i32 graphics_queue_index;
    i32 present_queue_index;
    i32 transfer_queue_index;

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkCommandPool graphics_command_pool;
} vulkan_device;

typedef struct vulkan_context
{
    // The framebuffer's current width.
    u32 framebuffer_width;
    // The framebuffer's current height.
    u32 framebuffer_height;

    // Current generation of framebuffer size. If it does not match framebuffer_size_last_generation,
    // a new one should be generated.
    u64 framebuffer_size_generation;

    // The generation of the framebuffer when it was last created. Set to framebuffer_size_generation
    // when updated.
    u64 framebuffer_size_last_generation;

    // Currently used image's index
    u32 image_index;
    f32 frame_delta_time;
    u32 current_frame;

    VkInstance instance;
    VkAllocationCallbacks *allocator;
    VkSurfaceKHR surface;

    b8 recreating_swapchain;
    vulkan_swapchain swap_chain;

    vulkan_device device;
    vulkan_renderpass main_renderpass;

    vulkan_command_buffer *graphics_command_buffers;

    /**
     * Triggered when an image becomes available for rendering
     * this is when we're done presenting it
     */
    VkSemaphore *image_available_semaphores; // darray

    /**
     * Triggered when a queue run against that and is now complete
     * and the queue is ready to be presented
     */
    VkSemaphore *queue_complete_semaphores; // darray

    u32 in_flight_fence_count;
    vulkan_fence *in_flight_fences;
    // Holds pointers to fences which exist and are owned elsewhere.
    vulkan_fence **images_in_flight; // in sync with current_frame

} vulkan_context;

// Indices (locations) of Queue Families (if they exist at all)
typedef struct vulkan_physical_device_queue_family_info
{
    i32 graphics_family_index = -1;     // Location of Graphics Queue Family
    i32 presentation_family_index = -1; // Location of the presentation queue family
} vulkan_physical_device_queue_family_info;

#endif