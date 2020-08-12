#include "fs.h"
#include "mbr.h"
#include "ext2.h"
#include "../alloc.h"
#include "../io/bios_drive.h"

#include "../stdlib.h"

void fs_init(uint8_t drive_num)
{
    struct mbr_sector* bootsect = kalloc(512);
    bdrive_read(drive_num, 1, 0, bootsect);

    for (int i = 0; i < 4; i++) {
        if (bootsect->partitions[i].drive_attributes == 0x83) {
#ifdef EXT2_ENABLE
            ext2_init(
                drive_num, 
                bootsect->partitions[i].start_lba, 
                bootsect->partitions[i].num_sectors
            );
#endif
        }
    }

    kfree(bootsect);
}
