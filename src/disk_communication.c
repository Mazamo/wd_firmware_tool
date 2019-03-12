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
#include "includes/disk_communication.h"

int open_hard_disk_drive(char *hard_disk_dev_file)
{

}

int identify_hard_disk_drive(int hard_disk_file_descriptor)
{

}

int verify_hard_disk_support(char *hard_disk_response)
{

}

int enable_vendor_specific_commands(void)
{

}

int disable_vendor_specific_commands(void)
{

}

int get_rom_acces(void *block, unsigned int size)
{

}

int read_rom_block(void)
{

}

int write_rom_block(void *block, unsigned int size)
{

}
