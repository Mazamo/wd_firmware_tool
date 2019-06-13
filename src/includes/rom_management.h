#ifndef ROM_MANAGEMENT_H
#define ROM_MANAGEMENT_H

#include <stdint.h>

/* Size of the ROM eeprom used on a WD hard disk drive. */
#define ROM_IMAGE_SIZE          256 * 1024
#define ROM_IMAGE_BLOCK_SIZE    64  * 1024

/* Maximum number of rom block headers found at the start of a rom image. */
#define NUMBER_OF_HEADERS       9

#define FLAG_UNENCRYPTED        0x04

#define SERIALISE_LINES_PER_BLOCK 12

/*
 * Structure used to describe a block of executable firmware code.
 * Source:
 * https://forum.hddguru.com/viewtopic.php?f=13&t=20324 - checksum
 * http://forum.hddguru.com/viewtopic.php?f=13&t=20324&start=40 - structure
 * http://www.onicos.com/staff/iz/formats/lzh.html - compression algorithm
 * http://www.fileformat.info/format/lzh/corion.htm - compression algorithm
 */
typedef struct __attribute__((packed)) {
	uint8_t block_nr; /* Number of the block in the table of block structures */
	uint8_t flag; /* Compression flag */
	uint8_t unk1; /* ? */
	uint8_t unk2; /* ? */
	uint32_t length_plus_cs; /* Length of the block plus the checksum*/
	uint32_t size; /* Length of the rom block*/
	uint32_t start_address; /* Offset of the block address in flash */

	/* Physical addr where decompresed blocks have to be stored */
	uint32_t load_address;
	uint32_t execution_address; /* Execution address */
	uint32_t unk3; /* ? */
	uint32_t fstw_plus_cs; /* 8-bit Checksum of the block */
} rom_block;

/* Dumps the rom image from a wd hard disk drive. */
int dump_rom_image(char *hard_disk_dev_file, char *out_file);

/* Upload the rom image to a wd hard disk drive. */
int upload_rom_image(char *hard_disk_dev_file, char *in_file);

/* Unpacks a packed rom image. */
int unpack_rom_image(char *rom_image);

/* Packs a rom image based with the name specified by out_file based on
   the init file specified by rom_image. */
int pack_rom_image(char *rom_image, char *out_file);

/* Replaces an instruction at memory_address with new_instruction in the rom
   image specified by the rom_image init file. */
int modify_instruction(char *rom_image, uint32_t memory_adress,
	uint32_t new_instruction, uint32_t instruction_byte_size);

/* Display information about the blocks found in a rom image. */
int display_rom_info(char *rom_image);

#endif
