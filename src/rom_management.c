/* Generic libraries */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <ctype.h>
#include <unistd.h>

/* Linux specific */
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <scsi/sg.h>
#include <scsi/scsi_ioctl.h>

/* Application specific */
#include "includes/rom_management.h"
#include "includes/disk_communication.h"

/* Calculates the checksum of a code block in the rom file */
static inline unsigned int calculate_rom_block_checksum( void *block,
    unsigned int size);

/* Operations: */
/* Open the hard disk device file */
/* Check if device is a supported western digital disk*/
/* Enable vendor specific command */
/* Get rom access */
/* Loop and read rom from hard disk drive */
/* Create a rom image file */
/* Write rom contents to rom image file */
/* Close rom image file */
/* Disable vendor specif commands */
int dump_rom_image(char *hard_disk_dev_file, char *out_file)
{
    uint8_t *rom_image_buffer;
    int output_file;

    int hdd_fd = open_hard_disk_drive(hard_disk_dev_file);
    if(hdd_fd == -1)
    {
        fprintf(stderr, "dump_rom_image: Could not handle hard disk drive.\n");
        return -1;
    }

    printf("Allocating memory for rom image\n");
    rom_image_buffer = calloc(ROM_IMAGE_SIZE, 1);
    if(rom_image_buffer == NULL)
    {
        perror("calloc:");
        close(hdd_fd);
        return -1;
    }

    printf("Enabling vendor specific commands\n");
    if(enable_vendor_specific_commands(hdd_fd) == -1)
    {
        fprintf(stderr, "dump_rom_image: Could not enable " \
            "vendor specific commands.\n");
        free(rom_image_buffer);
        close(hdd_fd);
        return -1;
    }

    printf("Getting access to rom.\n");
    if(get_rom_acces(hdd_fd) == -1)
    {
        fprintf(stderr, "get_rom_acces: Could not get rom access.\n");
        free(rom_image_buffer);
        close(hdd_fd);
        return -1;
    }

    unsigned int i;

    printf("Dumping rom\n");
    /* Request the ROM image using 64KiB block requests. */
    for(i = 0; i < ROM_IMAGE_SIZE; i += ROM_IMAGE_BLOCK_SIZE)
    {
        if(read_rom_block(hdd_fd, rom_image_buffer + i, ROM_IMAGE_BLOCK_SIZE)
            == -1)
        {
            fprintf(stderr, "dump_rom_image: Could not read the %drom block\n",
                (i / ROM_IMAGE_BLOCK_SIZE) + 1);
            free(rom_image_buffer);
            close(hdd_fd);
            return -1;
        }
    }

    printf("Disabling vendor specific commands\n");
    if(disable_vendor_specific_commands(hdd_fd) == -1)
    {
        fprintf(stderr, "dump_rom_image: Could not disable " \
            "vendor specific commands.\n");
        free(rom_image_buffer);
        close(hdd_fd);
        return -1;
    }

    close(hdd_fd);

    output_file = open(out_file, O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if(output_file == -1)
    {
        fprintf(stderr, "dump_rom_image: Could not create %s\n", out_file);
        return -1;
    }

    printf("Writing rom image to file\n");
    if(write(output_file, rom_image_buffer, ROM_IMAGE_BLOCK_SIZE) == -1)
    {
        fprintf(stderr, "dump_rom_image: Could not write to %s file\n",
            out_file);
        close(output_file);
        return -1;
    }

    close(output_file);

    return 0;
}

int unpack_rom_image(char *rom_image)
{
    return 0;
}

int pack_rom_image(char *rom_image, char *out_file)
{
    return 0;
}

int add_rom_block(char *rom_file, char *block_file)
{
    return 0;
}

int display_rom_info(char *rom_image)
{
    return 0;
}

static inline unsigned int calculate_rom_block_checksum(void *block,
    unsigned int size)
{
    return 0;
}
