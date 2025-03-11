#ifndef X86_H
#define X86_H

#include "bool.h"
#include "int.h"

// MARK: Helpers

void _cdecl x86_div64_32(
    uint64_t dividend, uint32_t divisor, uint64_t* quotient, uint32_t* remainder
);

// MARK: Video

void _cdecl x86_Video_write_char_teletype(char c, uint8_t page);

// MARK: Disk

bool _cdecl x86_Disk_reset(uint8_t drive);

bool _cdecl x86_Disk_read(
    uint8_t drive_number,
    uint16_t cylinder,
    uint16_t sector,
    uint16_t head,
    uint8_t sector_count,
    void far* buffer
);

bool _cdecl x86_Disk_get_drive_parameters(
    uint8_t drive,
    uint8_t* drive_type,
    uint16_t* cylinders,
    uint16_t* sectors,
    uint16_t* heads
);

#endif
