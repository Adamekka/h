#include "fat.h"
#include "io.h"

void _cdecl cstart_(uint16_t boot_drive) {
    Disk disk;
    if (!Disk_init(&disk, boot_drive)) {
        puts("Disk initialization failed!\r\n");
        goto end;
    }

    if (!FAT_init(&disk)) {
        puts("FAT initialization failed!\r\n");
        goto end;
    }

    // ls
    FATFile far* file = FAT_open(&disk, "/");
    FATDirectoryEntry entry;

    uint8_t i = 0;
    while (FAT_read_entry(&disk, file, &entry) && i++ < 5) {
        for (uint8_t i = 0; i < FAT_FILENAME_SIZE; i++)
            putc((char)entry.filename[i]);

        puts("\r\n");
    }

    FAT_close(file);

    // cat
    char buffer[100];
    uint32_t read; // NOLINT(cppcoreguidelines-init-variables)
    file = FAT_open(&disk, "testdir/test.txt");
    while ((read = FAT_read(&disk, file, sizeof(buffer), buffer)) > 0)
        for (uint32_t i = 0; i < read; i++)
            putc(buffer[i]);

    FAT_close(file);

end:
    while (true)
        ;
}
