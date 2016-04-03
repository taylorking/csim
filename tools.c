#include<stdint.h>

uint64_t get_bits(uint64_t source, int32_t low, int32_t high)
{
    if (low > 63 || high > 63) {
        return 0;
    }

    if (low > high) {
        return 0;
    }

    source = source << (63 - high);
    source = source >> (63 - high);
    source = source >> low;
    return source;
}
