#include "utils.h"

int clamp(int val, int low, int high)
{
    return val < low ? low : (val > high ? high : val);
}