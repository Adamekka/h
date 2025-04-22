#include "fat.h"
#include "char.h"
#include "cmp.h"
#include "io.h"
#include "memdefs.h"
#include "memory.h"
#include "null.h"
#include "string.h"
#include "x86.h"

#define SECTOR_SIZE 512
#define MAX_PATH_SIZE 256
#define MAX_FILE_HANDLES 10
#define ROOT_DIR_HANDLE -1

#pragma pack(push, 1)

typedef struct {
    uint8_t boot_jump_instruction[3];
    uint8_t oem[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_count;
    uint16_t dir_entry_count;
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
} FATBootSector;

#pragma pack(pop)

typedef struct {
    uint8_t buffer[SECTOR_SIZE];
    FATFile file;
    bool open;
    uint32_t first_cluster;
    uint32_t current_cluster;
    uint32_t current_sector; // In cluster
} FATFileData;

typedef struct {
    union {
        FATBootSector struct_;
        uint8_t raw[SECTOR_SIZE];
    } boot_sector;

    FATFileData root_dir;

    FATFileData open_files[MAX_FILE_HANDLES];
} FATData;

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
static FATData far* g_fat_data;
static uint8_t far* g_fat = NULL;
static uint32_t g_data_section_lba;
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

bool FAT_read_boot_sector(Disk* disk) {
    return Disk_read_sectors(disk, 0, 1, g_fat_data->boot_sector.raw);
}

bool FAT_read_fat(Disk* disk) {
    return Disk_read_sectors(
        disk,
        g_fat_data->boot_sector.struct_.reserved_sectors,
        g_fat_data->boot_sector.struct_.sectors_per_fat,
        g_fat
    );
}

bool FAT_init(Disk* disk) {
    // NOLINTNEXTLINE(bugprone-casting-through-void)
    g_fat_data = (FATData far*)MEMORY_FAT_ADDR;

    if (!FAT_read_boot_sector(disk)) {
        puts("FAT: Failed to read boot sector\r\n");
        return false;
    }

    g_fat = (uint8_t far*)g_fat_data + sizeof(FATData);
    uint32_t fat_size
        = (uint32_t)g_fat_data->boot_sector.struct_.bytes_per_sector
        * (uint32_t)g_fat_data->boot_sector.struct_.sectors_per_fat;

    if (sizeof(FATData) + fat_size >= MEMORY_FAT_SIZE) {
        printf(
            "FAT: FAT too large, required %lu, have %d\r\n",
            sizeof(FATData) + fat_size,
            MEMORY_FAT_SIZE
        );
        return false;
    }

    if (!FAT_read_fat(disk)) {
        puts("FAT: Failed to read FAT\r\n");
        return false;
    }

    // Open root directory
    uint32_t root_dir_lba = (g_fat_data->boot_sector.struct_.sectors_per_fat
                             * g_fat_data->boot_sector.struct_.fat_count)
                          + g_fat_data->boot_sector.struct_.reserved_sectors;

    uint32_t root_dir_size = g_fat_data->boot_sector.struct_.dir_entry_count
                           * sizeof(FATDirectoryEntry);

    g_fat_data->root_dir.file.handle = ROOT_DIR_HANDLE;
    g_fat_data->root_dir.file.is_dir = true;
    g_fat_data->root_dir.file.pos = 0;
    g_fat_data->root_dir.file.size = root_dir_size;
    g_fat_data->root_dir.open = true;
    g_fat_data->root_dir.first_cluster = root_dir_lba;
    g_fat_data->root_dir.current_cluster = root_dir_lba;
    g_fat_data->root_dir.current_sector = 0;

    if (!Disk_read_sectors(
            disk, root_dir_lba, 1, g_fat_data->root_dir.buffer
        )) {
        puts("FAT: Failed to read root directory\r\n");
        return false;
    }

    // Calculate data section LBA
    uint32_t data_sectors
        = (root_dir_size + g_fat_data->boot_sector.struct_.bytes_per_sector - 1)
        / g_fat_data->boot_sector.struct_.bytes_per_sector;

    g_data_section_lba = root_dir_lba + data_sectors;

    // Reset open files
    for (uint8_t i = 0; i < MAX_FILE_HANDLES; i++)
        g_fat_data->open_files[i].open = false;

    return true;
}

uint32_t FAT_cluster_to_lba(uint32_t cluster) {
    return ((cluster - 2) * g_fat_data->boot_sector.struct_.sectors_per_cluster)
         + g_data_section_lba;
}

FATFile far* FAT_open_entry(Disk* disk, FATDirectoryEntry* entry) {
    int16_t handle = -1;
    for (int16_t i = 0; i < MAX_FILE_HANDLES && handle < 0; i++)
        if (!g_fat_data->open_files[i].open)
            handle = i;

    if (handle < 0) {
        puts("FAT: No free file handles\r\n");
        return NULL;
    }

    FATFileData far* fd = &g_fat_data->open_files[handle];
    fd->file.handle = handle;
    fd->file.is_dir = (entry->attributes & FAT_ATTR_DIRECTORY) != 0;
    fd->file.pos = 0;
    fd->file.size = entry->file_size;
    fd->first_cluster = entry->first_cluster_low
                      + ((uint32_t)entry->first_cluster_high << 16);
    fd->current_cluster = fd->first_cluster;
    fd->current_sector = 0;

    if (!Disk_read_sectors(
            disk, FAT_cluster_to_lba(fd->current_cluster), 1, fd->buffer
        )) {
        printf("FAT: Failed to read cluster %lu\r\n", fd->current_cluster);
        return NULL;
    }

    fd->open = true;
    return &fd->file;
}

uint32_t FAT_next_cluster(uint32_t current_cluster) {
    uint32_t fat_index = current_cluster * 3 / 2;

    if (current_cluster % 2 == 0)
        return (*(uint16_t far*)(g_fat + fat_index)) & 0x0FFF;

    return current_cluster = (*(uint16_t far*)(g_fat + fat_index)) >> 4;
}

uint32_t
FAT_read(Disk* disk, FATFile far* file, uint32_t byte_count, void* buffer) {
    // Get file data
    FATFileData far* fd = file->handle == ROOT_DIR_HANDLE
                            ? &g_fat_data->root_dir
                            : &g_fat_data->open_files[file->handle];

    uint8_t* u8_buffer = (uint8_t*)buffer;

    // Don't read past the end of the file
    if (!fd->file.is_dir)
        byte_count = min(byte_count, fd->file.size - fd->file.pos);

    while (byte_count > 0) {
        uint32_t left_in_buffer = SECTOR_SIZE - (fd->file.pos % SECTOR_SIZE);
        uint32_t take = min(byte_count, left_in_buffer);

        memcpy(u8_buffer, fd->buffer + (fd->file.pos % SECTOR_SIZE), take);
        u8_buffer += take;
        fd->file.pos += take;
        byte_count -= take;

        // Check if we need more data else continue to next iteration
        if (left_in_buffer != take)
            continue;

        // Special case for root directory
        if (fd->file.handle == ROOT_DIR_HANDLE) {
            fd->current_cluster++;

            // Read next sector
            if (!Disk_read_sectors(disk, fd->current_cluster, 1, fd->buffer)) {
                puts("FAT: Read error");
                break;
            }

            continue;
        }

        // Calculate next custer and sector to read
        if (++fd->current_sector
            >= g_fat_data->boot_sector.struct_.sectors_per_cluster) {
            fd->current_sector = 0;
            fd->current_cluster = FAT_next_cluster(fd->current_cluster);
        }

        if (fd->current_cluster >= 0xFF8) {
            // End of file
            fd->file.size = fd->file.pos;
            break;
        }

        // Read next sector
        if (!Disk_read_sectors(
                disk, FAT_cluster_to_lba(fd->current_cluster), 1, fd->buffer
            )) {
            puts("FAT: Read error");
            break;
        }
    }

    return u8_buffer - (uint8_t*)buffer;
}

bool FAT_read_entry(Disk* disk, FATFile far* file, FATDirectoryEntry* entry) {
    return FAT_read(disk, file, sizeof(FATDirectoryEntry), entry)
        == sizeof(FATDirectoryEntry);
}

void FAT_close(FATFile far* file) {
    if (file->handle == ROOT_DIR_HANDLE) {
        file->pos = 0;
        g_fat_data->root_dir.current_cluster
            = g_fat_data->root_dir.first_cluster;
        return;
    }

    g_fat_data->open_files[file->handle].open = false;
}

bool FAT_find_file(
    Disk* disk,
    FATFile far* file,
    const char* name,
    FATDirectoryEntry* found_entry // NOLINT
) {
    char fat_name[FAT_FILENAME_SIZE + 1];

    // Convert name to FAT format
    memset(fat_name, ' ', sizeof(fat_name));
    fat_name[FAT_FILENAME_SIZE] = '\0';

    const char* extension = strchr(name, '.');
    if (extension == NULL)
        extension = name + FAT_FILENAME_SIZE;

    for (uint8_t i = 0;
         i < FAT_FILENAME_NAME_SIZE && name[i] && name + i < extension;
         i++)
        fat_name[i] = to_upper(name[i]);

    if (extension != NULL)
        for (uint8_t i = 0; i < FAT_FILENAME_EXT_SIZE && extension[i + 1]; i++)
            fat_name[FAT_FILENAME_NAME_SIZE + i] = to_upper(extension[i + 1]);

    FATDirectoryEntry current_entry;

    while (FAT_read_entry(disk, file, &current_entry)) {
        if (memcmp(fat_name, current_entry.filename, FAT_FILENAME_SIZE) == 0) {
            *found_entry = current_entry;
            return true;
        }
    }

    return false;
}

FATFile far* FAT_open(Disk* disk, const char* path) {
    char name[MAX_PATH_SIZE];

    // Ignore leading slash
    if (path[0] == '/')
        path++;

    FATFile far* current = &g_fat_data->root_dir.file;

    while (*path) {
        // Extract file name from the path
        bool is_last = false;
        const char* delim = strchr(path, '/');

        if (delim != NULL) {
            memcpy(name, path, delim - path);
            name[delim - path + 1] = '\0';
            path = delim + 1;
        } else {
            uint16_t len = strlen(path);
            memcpy(name, path, len);
            name[len + 1] = '\0';
            path += len;
            is_last = true;
        }

        // Find directory entry in the current directory
        FATDirectoryEntry entry;

        if (!FAT_find_file(disk, current, name, &entry)) {
            FAT_close(current);
            printf("FAT: File not found: %s\r\n", name);
            return NULL;
        }

        FAT_close(current);

        // Check if the entry is a directory
        if (!is_last && (entry.attributes & FAT_ATTR_DIRECTORY) == 0) {
            printf("FAT: Not a directory: %s\r\n", name);
            return NULL;
        }

        // Open new directory entry
        current = FAT_open_entry(disk, &entry);
    }

    return current;
}
