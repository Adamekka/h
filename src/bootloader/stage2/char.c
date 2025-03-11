#include "char.h"

bool is_upper(char c) { return c >= 'A' && c <= 'Z'; }

bool is_lower(char c) { return c >= 'a' && c <= 'z'; }

char to_upper(char c) {
    if (is_lower(c))
        return (char)(c - 'a' + 'A');

    return c;
}

char to_lower(char c) {
    if (is_upper(c))
        return (char)(c - 'A' + 'a');

    return c;
}
