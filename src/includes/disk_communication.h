#ifndef DISK_COMMUNICATION_H
#define DISK_COMMUNICATION_H

#include <stdint.h>

/*
 * Used sources:
 * - http://www.t13.org/Documents/
        UploadedDocuments/docs2016/di529r14-ATAATAPI_Command_Set_-_4.pdf
 * - https://wiki.osdev.org/ATA_Command_Matrix
 *
 * For vendor specific commands reverse WD firmware tool.
 */
#define ATA_VENDOR_SPECIFIC_COMMAND     0x80
#define ATA_IDENTIFY                    0xEC
#define ATA_OP_SMART                    0xb0

#define SG_ATA_16                       0x85
#define SG_ATA_16_LEN		            16

#define IDENTIFY_SERIAL_NUMBER_START    10 * 2
#define IDENTIFY_SERIAL_NUMBER_END      19 * 2

#define IDENTIFY_FIRMWARE_REVISION_START    23 * 2
#define IDENTIFY_FIRMWARE_REVISION_END      26 * 2

#define IDENTIFY_MODEL_NUMBER_START     27 * 2
#define IDENTIFY_MODEL_NUMBER_END       46 * 2

#define SCSI_DEFAULT_TIMEOUT            20000

#define SG_CHECK_CONDITION	            0x02
#define SG_DRIVER_SENSE		            0x08

#define ROM_KEY_READ                    0x01
#define ROM_KEY_WRTIE                   0x02

/*
 * Some useful ATA register bits (source: http://idle3-tools.sourceforge.net/)
 */
enum {
	ATA_USING_LBA		= (1 << 6),
	ATA_STAT_DRQ		= (1 << 3),
	ATA_STAT_ERR		= (1 << 0),
};

/* Opens a hard disk drive's device file */
int open_hard_disk_drive(char *hard_disk_dev_file);

/* Identifies a hard disk drive by sending an inquiry packet */
int identify_hard_disk_drive(int hard_disk_file_descriptor);

/* Checks the output of an inquiry packet to determine if the disk is
   supported */
int verify_hard_disk_support(uint8_t *hard_disk_response);

/* Send a packet that enables vendor specific command capabilities */
int enable_vendor_specific_commands(int hard_disk_file_descriptor);

/* Send a packet that disables vendor specifc command capabilities */
int disable_vendor_specific_commands(int hard_disk_file_descriptor);

/* Send a packet that enables rom access */
int get_rom_acces(int hard_disk_file_descriptor);

/* Read a rom block from the hard disk drive */
int read_rom_block(int hard_disk_file_descriptor, void *block,
    unsigned int size);

/* Write a rom block to the hard disk drive */
int write_rom_block(int hard_disk_file_descriptor, void *block,
    unsigned int size);

/* Execute Linux SCSI command */
int execute_command(unsigned char *cdb, int hard_disk_file_descriptor,
    void *response_buffer, unsigned int response_buffer_size,
    int data_direction);

#endif
