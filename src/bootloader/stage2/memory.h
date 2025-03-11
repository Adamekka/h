#ifndef MEMORY_H
#define MEMORY_H

#include "int.h"

void far* memcpy(void far* dest, const void far* src, uint16_t size);
void far* memset(void far* dest, uint8_t value, uint16_t size);
int32_t memcmp(const void far* ptr1, const void far* ptr2, uint16_t size);

#endif
