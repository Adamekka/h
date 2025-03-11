#include "util.h"

uint32_t align(uint32_t value, uint32_t alignment) {
    if (!alignment)
        return value;

    uint32_t rem = value % alignment;
    return (rem > 0) ? value + alignment - rem : value;
}
