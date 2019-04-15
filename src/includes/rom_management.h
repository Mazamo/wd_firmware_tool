#ifndef ROM_MANAGEMENT_H
#define ROM_MANAGEMENT_H

#include <stdint.h>

/* Size of the ROM eeprom used on a WD hard disk drive. */
#define ROM_IMAGE_SIZE          256 * 1024
#define ROM_IMAGE_BLOCK_SIZE    64  * 1024

/* Maximum number of rom block headers found at the start of a rom image. */
#define NUMBER_OF_HEADERS       9

#define FLAG_UNENCRYPTED        0x04

/*
 * Structure used to describe a block of executable firmware code.
 * Source: http://forum.hddguru.com/viewtopic.php?f=13&t=20324&start=40
 * *http://spritesmods.com/?art=hddhack&page=4
 */
typedef struct __attribute__((packed)) {
	uint8_t block_nr; /* Number of the block in the table of block structures */
	uint8_t flag;
	uint8_t unk1; /* ? */
	uint8_t unk2; /* ? */
	uint32_t length_plus_cs; /* Length of the block plus the checksum*/
	uint32_t size; /* Length of the rom block*/
	uint32_t start_address; /* Start of the block address */

	/* Physical addr where decompresed blocks have to be stored */
	uint32_t load_address;
	uint32_t execution_address; /* Execution address */
	uint32_t unk3; /* ? */
	uint32_t fstw_plus_cs; /* Checksum of the block */
} rom_block;

/* Dumps the rom image from a wd hard disk drive. */
int dump_rom_image(char *hard_disk_dev_file, char *out_file);

/* Upload the rom image to a wd hard disk drive. */
int upload_rom_image(char *hard_disk_dev_file, char *in_file);

/* Unpacks a packed rom image. */
int unpack_rom_image(char *rom_image, char *out_file);

/* Packs a rom image. */
int pack_rom_image(char *rom_image, char *out_file);

/* Adds a code block to a rom file (also packs the new block). */
int add_rom_block(char *rom_file, char *block_file);

/* Display information about the blocks found in a rom image. */
int display_rom_info(char *rom_image);

#endif
