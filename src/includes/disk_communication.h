#ifndef DISK_COMMUNICATION_H
#define DISK_COMMUNICATION_H

/*
 * Used ATA commands for more commands see:
 * https://wiki.osdev.org/ATA_Command_Matrix
 *
 * For vendor specific commands reverse WD firmware tool.
 */
#define ATA_VENDOR_SPECIFIC_IDENTIFIER  0x80
#define ATA_IDENTIFY                    0xEC

/* Opens a hard disk drive's device file */
int open_hard_disk_drive(char *hard_disk_dev_file);

/* Identifies a hard disk drive by sending an inquiry packet */
int identify_hard_disk_drive(int hard_disk_file_descriptor);

/* Checks the output of an inquiry packet to determine if the disk is
   supported */
int verify_hard_disk_support(char *hard_disk_response);

/* Send a packet that enables vendor specific command capabilities */
int enable_vendor_specific_commands(void);

/* Send a packet that disables vendor specifc command capabilities */
int disable_vendor_specific_commands(void);

/* Send a packet that enables rom access */
int get_rom_access(void);

/* Read a rom block from the hard disk drive */
int read_rom_block(void *block, unsigned int size);

/* Write a rom block to the hard disk drive */
int write_rom_block(void *block, unsigned int size);

#endif
