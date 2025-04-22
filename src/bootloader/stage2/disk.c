#include "disk.h"
#include "io.h"
#include "x86.h"

bool Disk_init(Disk* disk, uint8_t drive_number) {
    // NOLINTBEGIN(cppcoreguidelines-init-variables)
    uint8_t drive_type;
    uint16_t cylinders;
    uint16_t sectors;
    uint16_t heads;
    // NOLINTEND(cppcoreguidelines-init-variables)

    if (!x86_Disk_get_drive_parameters(
            disk->id, &drive_type, &cylinders, &sectors, &heads
        )) {
        return false;
    }

    disk->id = drive_number;
    disk->cylinders = cylinders + 1;
    disk->sectors = sectors;
    disk->heads = heads + 1;

    return true;
}

void Disk_LBA2CHS( // NOLINT(readability-identifier-naming)
    Disk* disk,
    uint32_t lba,
    uint16_t* cylinder,
    uint16_t* sector,
    uint16_t* head
) {
    // sector = LBA % sectors per track + 1
    *sector = lba % disk->sectors + 1;

    // cylinder = (LBA / sectors per track) / heads
    *cylinder = (lba / disk->sectors) / disk->heads;

    // head = (LBA / sectors per track) % heads
    *head = (lba / disk->sectors) % disk->heads;
}

bool Disk_read_sectors(
    Disk* disk, uint32_t lba, uint8_t sector_count, void far* buffer
) {
    // NOLINTBEGIN(cppcoreguidelines-init-variables)
    uint16_t cylinder;
    uint16_t sector;
    uint16_t head;
    // NOLINTEND(cppcoreguidelines-init-variables)

    Disk_LBA2CHS(disk, lba, &cylinder, &sector, &head);

    for (uint8_t i = 0; i < 3; i++) {
        if (x86_Disk_read(
                disk->id, cylinder, sector, head, sector_count, buffer
            ))
            return true;

        x86_Disk_reset(disk->id);
    }

    return false;
}
