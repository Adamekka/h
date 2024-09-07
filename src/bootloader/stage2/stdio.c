#include "stdio.h"
#include "stdbool.h"
#include "x86.h"

void putc(char c) { x86_Video_WriteCharTeletype(c, 0); }

void puts(const char* str) {
    while (*str) {
        putc(*str);
        str++;
    }
}

typedef enum { DEFAULT, SHORT_SHORT, SHORT, LONG, LONG_LONG } Length;

int16_t* printf_number(int16_t* argp, Length length, bool sign, int16_t radix);

void _cdecl printf(const char* fmt, ...) {
    int16_t* argp = (int16_t*)&fmt + 1;
    enum { NORMAL, LENGTH, LENGTH_SHORT, LENGTH_LONG, SPEC } state = NORMAL;
    Length length = DEFAULT;
    int16_t radix = 10;
    bool sign = false;

    while (*fmt) {
        switch (state) {
            case NORMAL: {
                switch (*fmt) {
                    case '%': state = LENGTH; break;
                    default: putc(*fmt); break;
                }
                break;
            }

            case LENGTH: {
                switch (*fmt) {
                    case 'h': {
                        length = SHORT;
                        state = LENGTH_SHORT;
                        break;
                    }

                    case 'l': {
                        length = LONG;
                        state = LENGTH_LONG;
                        break;
                    }

                    default: goto PRINTF_STATE_SPEC_;
                }
                break;
            }

            case LENGTH_SHORT: {
                if (*fmt == 'h') {
                    length = SHORT_SHORT;
                    state = SPEC;
                } else {
                    goto PRINTF_STATE_SPEC_;
                }
                break;
            }

            case LENGTH_LONG: {
                if (*fmt == 'l') {
                    length = LONG_LONG;
                    state = SPEC;
                } else {
                    goto PRINTF_STATE_SPEC_;
                }
                break;
            }

            PRINTF_STATE_SPEC_:
            case SPEC: {
                switch (*fmt) {
                    case 'c': {
                        putc((char)*argp);
                        argp++;
                        break;
                    }

                    case 's': {
                        puts(*(char**)argp);
                        argp++;
                        break;
                    }

                    case '%': putc('%'); break;

                    case 'd':
                    case 'i': {
                        radix = 10;
                        sign = true;
                        argp = printf_number(argp, length, sign, radix);
                        break;
                    }

                    case 'u': {
                        radix = 10;
                        sign = false;
                        argp = printf_number(argp, length, sign, radix);
                        break;
                    }

                    case 'X':
                    case 'x':
                    case 'p': {
                        radix = 16;
                        sign = false;
                        argp = printf_number(argp, length, sign, radix);
                        break;
                    }

                    case 'o': {
                        radix = 8;
                        sign = false;
                        argp = printf_number(argp, length, sign, radix);
                        break;
                    }

                    // Ignore invalid specifiers
                    default: break;
                }

                // Reset state
                state = NORMAL;
                length = DEFAULT;
                radix = 10;
                sign = false;
                break;
            }
        }

        fmt++;
    }
}

const char g_hex_chars[] = "0123456789abcdef";

int16_t* printf_number(int16_t* argp, Length length, bool sign, int16_t radix) {
    char buffer[32];
    uint64_t number;
    int16_t number_sign = 1;
    int16_t pos = 0;

    // Process length and sign
    switch (length) {
        case SHORT_SHORT:
        case SHORT:
        case DEFAULT: {
            if (sign) {
                int16_t n = *argp;
                if (n < 0) {
                    n = -n;
                    number_sign = -1;
                }
                number = (uint64_t)n;
            } else {
                number = *(uint16_t*)argp;
            }
            argp++;
            break;
        }

        case LONG: {
            if (sign) {
                int32_t n = *(int32_t*)argp;
                if (n < 0) {
                    n = -n;
                    number_sign = -1;
                }
                number = (uint64_t)n;
            } else {
                number = *(uint32_t*)argp;
            }
            argp += 2;
            break;
        }

        case LONG_LONG: {
            if (sign) {
                int64_t n = *(int64_t*)argp;
                if (n < 0) {
                    n = -n;
                    number_sign = -1;
                }
                number = (uint64_t)n;
            } else {
                number = *(uint64_t*)argp;
            }
            argp += 4;
            break;
        }
    }

    // Convert number to ASCII
    do {
        uint32_t rem;
        x86_div64_32(number, radix, &number, &rem);
        buffer[pos++] = g_hex_chars[rem];
    } while (number > 0);

    // Add sign
    if (sign && number_sign < 0)
        buffer[pos++] = '-';

    // Print number in reverse order
    while (--pos >= 0)
        putc(buffer[pos]);

    return argp;
}
