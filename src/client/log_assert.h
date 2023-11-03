#include <stdio.h>
#include <stdlib.h>

#define ERR_EXIT(err_msg, err_class) \
    do                               \
    {                                \
        printf(err_msg);             \
        fflush(stdout);              \
        exit(EXIT_FAILURE);          \
    } while (0)