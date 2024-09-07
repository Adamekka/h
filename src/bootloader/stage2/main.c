#include "stdbool.h"
#include "stdint.h"
#include "stdio.h"

void _cdecl cstart_(uint16_t boot_drive) {
    puts("Bootloader stage2 loaded!\r\n");
    while (true)
        ;
}
