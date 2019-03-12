/* Generic libraries */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <ctype.h>

/* Linux specific */
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <scsi/sg.h>
#include <scsi/scsi_ioctl.h>

/* Application specific */
#include "includes/rom_management.h"

static inline unsigned int calculate_rom_block_checksum( void *block,
    unsigned int size);

int dump_rom_image(char *hard_disk_dev_file, char *out_file)
{
    /* Create a rom image file */
    /* Open the hard disk device file */
    /* Check if device is a supported western digital disk*/
    /* Enable vendor specific command */
    /* Get rom access */
    /* Loop and read rom from hard disk drive */
    /* Write rom contents to rom image file */
    /* Close rom image file */
    /* Disable vendor specif commands */

    return 0;
}

int unpack_rom_image(char *rom_image)
{

}

int pack_rom_image(char *rom_image, char *out_file)
{

}

int add_rom_block(char *rom_file, char *block_file)
{

}

int display_rom_info(char *rom_image)
{

}

static inline unsigned int calculate_rom_block_checksum(void *block,
    unsigned int size)
{

}
