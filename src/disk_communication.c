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
    if(strncmp(hard_disk_dev_file, "/dev/s", sizeof("/dev/s") - 1) != 0)
    {
        fprintf(stderr, "open_hard_disk_drive: Invalid device file.\n");
        return -1;
    }

    int fd = open(hard_disk_dev_file, O_RDWR);
    if(fd == -1)
    {
        perror("open:");
        return -1;
    }

    if(identify_hard_disk_drive(fd) == -1)
    {
        fprintf(stderr, "open_hard_disk_drive: Specified hard disk drive is " \
            "not supported\n");
        return -1;
    }

    return fd;
}

int identify_hard_disk_drive(int hard_disk_file_descriptor)
{

    unsigned char identify_cdb[16];

    identify_cdb[0]     = 0x85; /* operation code: */

    /* multiple count: 0 protocol: 4 extended: 0  */
    /* protocol 4: PIO Data-In */
    identify_cdb[1]     = 0x08;

    /* off.line: cc: lh.en: ll.en: sc.en: f.en: */
    identify_cdb[2]     = 0x2e;
    identify_cdb[3]     = 0x00; /* Features (8:15): */
    identify_cdb[4]     = 0x00; /* Features (0:7): */
    identify_cdb[5]     = 0x00; /* Sector Count (8:15): */
    identify_cdb[6]     = 0x00; /* Sector Count (0:7): */
    identify_cdb[7]     = 0x00; /* LBA Low (8:15): */
    identify_cdb[8]     = 0x00; /* LBA Low (0:7): */
    identify_cdb[9]     = 0x00; /* LBA Mid (8:15): */
    identify_cdb[10]    = 0x00; /* LBA Mid (0:7): */
    identify_cdb[11]    = 0x00; /* LBA High (8:15): */
    identify_cdb[12]    = 0x00; /* LBA High (0:7): */
    identify_cdb[13]    = 0x40; /* Device: */
    identify_cdb[14]    = 0xEC; /* Command: */
    identify_cdb[15]    = 0x00; /* Control: */

    sg_io_hdr_t io_hdr;
    uint16_t identify_reply_buffer[512];
    unsigned char sense_buffer[32];

    memset(identify_reply_buffer, 0, sizeof(identify_reply_buffer));
    memset(&io_hdr, 0, sizeof(sg_io_hdr_t));

    io_hdr.interface_id = 'S';
    io_hdr.cmd_len = sizeof(identify_cdb);
    io_hdr.mx_sb_len = sizeof(sense_buffer);
    io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
    io_hdr.dxfer_len = 512;
    io_hdr.dxferp = identify_reply_buffer;
    io_hdr.cmdp = identify_cdb;
    io_hdr.sbp = sense_buffer;
    io_hdr.timeout = 20000;

    if(ioctl(hard_disk_file_descriptor, SG_IO, &io_hdr) < 0)
    {
        perror("ioctl: ");

        int i;
        for(i = 0; i < sizeof(sense_buffer); ++i)
        {
            printf("%hx ", (unsigned short) sense_buffer[i]);
        }

        return -1;
    }

    printf("\n\n");

    int j;
    for(j = 0; j < sizeof(identify_reply_buffer); ++j)
    {
        printf("%d: %x \n", j, identify_reply_buffer[j]);
    }
    printf("\n");

    return 0;
}

int verify_hard_disk_support(char *hard_disk_response)
{
    return 0;
}

int enable_vendor_specific_commands(void)
{
    return 0;
}

int disable_vendor_specific_commands(void)
{
    return 0;
}

int get_rom_acces(void)
{
    return 0;
}

int read_rom_block(void *block, unsigned int size)
{
    return 0;
}

int write_rom_block(void *block, unsigned int size)
{
    return 0;
}
