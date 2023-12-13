#ifndef VULKAN_NOTES_1698946259_VULKAN_RENDERER_H
#define VULKAN_NOTES_1698946259_VULKAN_RENDERER_H

#include "defines.h"

struct GLFWwindow;

int init_renderer(GLFWwindow *window, u32 width, u32 height);
void cleanup_renderer();

#endif