#include "string.h"
#include "null.h"

const char* strchr(const char* str, char c) {
    if (str == NULL)
        return NULL;

    while (*str) {
        if (*str == c)
            return str;
        str++;
    }

    return NULL;
}

char* strcpy(char* dest, const char* src) {
    if (dest == NULL)
        return NULL;

    if (src == NULL) {
        *dest = '\0';
        return NULL;
    }

    char* orig_dest = dest;

    while (*src) {
        *dest = *src;
        dest++;
        src++;
    }

    *dest = '\0';

    return orig_dest;
}

uint16_t strlen(const char* str) {
    uint16_t len = 0;

    while (*str) {
        len++;
        str++;
    }

    return len;
}
