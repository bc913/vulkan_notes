#include "utils.h"
#include "log_assert.h"
#include <stdio.h>

int clamp(int val, int low, int high)
{
    return val < low ? low : (val > high ? high : val);
}

b8 parse_file_into_str(const char *file_name, char *shader_str, int max_len)
{
    FILE *file = fopen(file_name, "r");
    if (!file)
    {
        ERR_EXIT("opening file for reading:\n", "parse_file_into_str");
    }

    size_t cnt = fread(shader_str, 1, max_len - 1, file);
    if ((int)cnt >= max_len - 1)
        printf("WARNING: file%s too big - truncated.\n", file_name);

    if (ferror(file))
    {
        fclose(file);
        printf("Can not read file %s\n:", file_name);
        ERR_EXIT("reading shader file:\n", "parse_file_into_str");
    }

    // append \0 to end of file string
    shader_str[cnt] = 0;
    fclose(file);
    return BC_TRUE;
}
