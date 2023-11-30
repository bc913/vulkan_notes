#ifndef VULKAN_NOTES_1701137841_UTILS_H
#define VULKAN_NOTES_1701137841_UTILS_H

#include "defines.h"

int clamp(int val, int low, int high);
b8 parse_file_into_str(const char *file_name, int max_len, char *shader_str, size_t *str_len);

#endif