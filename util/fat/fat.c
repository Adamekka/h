#include <ctype.h>
#include <math.h>
#include <memory.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define eprintf(...) (void)fprintf(stderr, __VA_ARGS__)

typedef struct __attribute__((packed)) BootSector {
    uint8_t boot_jump_instruction[3];
    uint8_t oem[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_count;
    uint16_t dir_entries_count;
    uint16_t total_sectors;
    uint8_t media_descriptor_type;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t heads;
    uint32_t hidden_sectors;
    uint32_t large_sector_count;

    // Extended boot record
    uint8_t drive_number;
    uint8_t _reserved;
    uint8_t signature;
    uint8_t volume_id[4];
    uint8_t volume_label[11];
    uint8_t system_id[8];
} BootSector;

typedef struct __attribute__((packed)) DirectoryEntry {
    uint8_t filename[11];
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
} DirectoryEntry;

BootSector g_boot_sector;
uint8_t* g_fat = NULL;
DirectoryEntry* g_root_dir = NULL;
uint32_t g_root_dir_end = 0;

bool read_boot_sector(FILE* disk) {
    return fread(&g_boot_sector, sizeof(BootSector), 1, disk);
}

bool read_sectors(
    FILE* disk, const uint32_t lba, const uint32_t count, void* buffer
) {
    return !fseek(disk, lba * g_boot_sector.bytes_per_sector, SEEK_SET)
        && fread(buffer, g_boot_sector.bytes_per_sector, count, disk) == count;
}

bool read_fat(FILE* disk) {
    g_fat = malloc(
        g_boot_sector.sectors_per_fat * g_boot_sector.bytes_per_sector
    );

    return read_sectors(
        disk,
        g_boot_sector.reserved_sectors,
        g_boot_sector.sectors_per_fat,
        g_fat
    );
}

bool read_root_dir(FILE* disk) {
    uint32_t lba = g_boot_sector.reserved_sectors
                 + g_boot_sector.sectors_per_fat * g_boot_sector.fat_count;
    uint32_t size = g_boot_sector.dir_entries_count * sizeof(DirectoryEntry);
    uint32_t sectors = ceilf((float)size / g_boot_sector.bytes_per_sector);

    g_root_dir = malloc(sectors * g_boot_sector.bytes_per_sector);
    g_root_dir_end = lba + sectors;

    return read_sectors(disk, lba, sectors, g_root_dir);
}

DirectoryEntry* find_file(const char* name) {
    for (typeof(g_boot_sector.dir_entries_count) i = 0;
         i < g_boot_sector.dir_entries_count;
         i++)
        if (!memcmp(name, g_root_dir[i].filename, 11))
            return &g_root_dir[i];

    return NULL;
}

bool read_file(DirectoryEntry* file, FILE* disk, uint8_t* buffer) {
    uint16_t cluster = file->first_cluster_low;

    while (cluster < 0x0FF8) {
        uint32_t lba = g_root_dir_end
                     + (cluster - 2) * g_boot_sector.sectors_per_cluster;

        if (!read_sectors(disk, lba, g_boot_sector.sectors_per_cluster, buffer))
            return false;

        buffer += g_boot_sector.sectors_per_cluster
                * g_boot_sector.bytes_per_sector;

        uint32_t fat_offset = cluster * 3 / 2;
        if (cluster % 2)
            cluster = (*(uint16_t*)(g_fat + fat_offset)) >> 4;
        else
            cluster = (*(uint16_t*)(g_fat + fat_offset)) & 0x0FFF;
    }

    return true;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        printf("Usage: %s <disk image> <file name>\n", argv[0]);
        return -1;
    }

    FILE* disk = fopen(argv[1], "rb");

    if (!disk) {
        eprintf("Failed to open disk image %s\n", argv[1]);
        return -1;
    }

    if (!read_boot_sector(disk)) {
        eprintf("Failed to read boot sector\n");
        return -2;
    }

    if (!read_fat(disk)) {
        eprintf("Failed to read FAT\n");
        free(g_fat);
        return -3;
    }

    if (!read_root_dir(disk)) {
        eprintf("Failed to read root directory\n");
        free(g_fat);
        free(g_root_dir);
        return -4;
    }

    DirectoryEntry* file = find_file(argv[2]);

    if (!file) {
        eprintf("File not found: %s\n", argv[2]);
        free(g_fat);
        free(g_root_dir);
        return -5;
    }

    uint8_t* buffer
        = (uint8_t*)malloc(file->file_size + g_boot_sector.bytes_per_sector);

    if (!read_file(file, disk, buffer)) {
        eprintf("Failed to read file\n");
        free(g_fat);
        free(g_root_dir);
        free(buffer);
        return -6;
    }

    for (typeof(file->file_size) i = 0; i < file->file_size; i++) {
        uint8_t c = buffer[i]; // NOLINT
        isprint(c) ? putchar(c) : printf("<%02X>", c);
    }

    printf("\n");

    free(g_fat);
    free(g_root_dir);
    free(buffer);
    return 0;
}
