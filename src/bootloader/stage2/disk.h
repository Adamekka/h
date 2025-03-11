#ifndef DISK_H
#define DISK_H

#include "bool.h"
#include "int.h"

typedef struct {
    uint8_t id;
    uint16_t cylinders;
    uint16_t sectors;
    uint16_t heads;
} Disk;

bool Disk_init(Disk* disk, uint8_t drive_number);
bool Disk_read_sectors(
    Disk* disk, uint32_t lba, uint8_t sector_count, void far* buffer
);

#endif
