#ifndef ROM_MANAGEMENT_H
#define ROM_MANAGEMENT_H

/* Dumps the rom image from a wd hard disk drive. */
int dump_rom_image(char *hard_disk_dev_file, char *out_file);

/* Unpacks a packed rom image. */
int unpack_rom_image(char *rom_image);

/* Packs a rom image. */
int pack_rom_image(char *rom_image, char *out_file);

/* Adds a code block to a rom file (also packs the new block). */
int add_rom_block(char *rom_file, char *block_file);

/* Display information about the blocks found in a rom image. */
int display_rom_info(char *rom_image);

#endif
