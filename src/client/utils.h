#ifndef VULKAN_NOTES_1701137841_UTILS_H
#define VULKAN_NOTES_1701137841_UTILS_H

#include "defines.h"

int clamp(int val, int low, int high);
b8 parse_file_into_str(const char *file_name, char *shader_str, int max_len);

#endif