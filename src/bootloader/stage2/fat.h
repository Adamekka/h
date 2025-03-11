#ifndef FAT_H
#define FAT_H

#include "bool.h"
#include "disk.h"

#define FAT_FILENAME_SIZE 11
#define FAT_FILENAME_NAME_SIZE 8
#define FAT_FILENAME_EXT_SIZE 3

#pragma pack(push, 1)

typedef struct {
    uint8_t filename[FAT_FILENAME_SIZE];
    uint8_t attributes;
    uint8_t _reserved;
    uint8_t creation_time_tenths;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t first_cluster_high;
    uint16_t last_write_time;
    uint16_t last_write_date;
    uint16_t first_cluster_low;
    uint32_t file_size;
} FATDirectoryEntry;

#pragma pack(pop)

typedef struct {
    int16_t handle;
    bool is_dir;
    uint32_t pos;
    uint32_t size;
} FATFile;

typedef enum {
    FAT_ATTR_READ_ONLY = 0x01,
    FAT_ATTR_HIDDEN = 0x02,
    FAT_ATTR_SYSTEM = 0x04,
    FAT_ATTR_VOLUME_ID = 0x08,
    FAT_ATTR_DIRECTORY = 0x10,
    FAT_ATTR_ARCHIVE = 0x20,
    FAT_ATTR_LFN = FAT_ATTR_READ_ONLY | FAT_ATTR_HIDDEN | FAT_ATTR_SYSTEM
                 | FAT_ATTR_VOLUME_ID,
} FATAttribute;

bool FAT_init(Disk* disk);
FATFile far* FAT_open(Disk* disk, const char* path);
uint32_t
FAT_read(Disk* disk, FATFile far* file, uint32_t byte_count, void* buffer);
bool FAT_read_entry(Disk* disk, FATFile far* file, FATDirectoryEntry* entry);
void FAT_close(FATFile far* file);

#endif
