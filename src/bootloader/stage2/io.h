#ifndef IO_H
#define IO_H

#include "int.h"

void putc(char c);
void puts(const char* str);
void _cdecl printf(const char* fmt, ...);
void print_buffer(const void* message, const void* buffer, uint16_t size);

#endif
