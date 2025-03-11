#include "memory.h"

void far* memcpy(void far* dest, const void far* src, uint16_t size) {
    uint8_t far* u8_dest = (uint8_t far*)dest;
    const uint8_t far* u8_src = (const uint8_t far*)src;

    for (uint16_t i = 0; i < size; i++)
        u8_dest[i] = u8_src[i];

    return dest;
}

void far* memset(void far* dest, uint8_t value, uint16_t size) {
    uint8_t far* u8_ptr = (uint8_t far*)dest;

    for (uint16_t i = 0; i < size; i++)
        u8_ptr[i] = value;

    return dest;
}

int32_t memcmp(const void far* ptr1, const void far* ptr2, uint16_t size) {
    const uint8_t far* u8_ptr1 = (const uint8_t far*)ptr1;
    const uint8_t far* u8_ptr2 = (const uint8_t far*)ptr2;

    for (uint16_t i = 0; i < size; i++)
        if (u8_ptr1[i] != u8_ptr2[i])
            return u8_ptr1[i] - u8_ptr2[i];

    return 0;
}
