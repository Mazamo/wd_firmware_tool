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
#include <sys/stat.h>
#include <linux/types.h>
#include <scsi/sg.h>
#include <scsi/scsi_ioctl.h>

/* Application specific */
#include "includes/rom_management.h"
#include "includes/disk_communication.h"

/* Little-endian to native endian */
static inline uint32_t le_32_to_be(uint32_t integer);

/* Native endian to little-endian */
static uint32_t be_32_to_le(uint32_t integer);

/* Calculates the checksum (8 or 16 bit) of a code block in the rom file. */
static unsigned int calculate_rom_block_checksum_8(uint8_t *block,
    unsigned int size);

/* Calculates the checksum for a single line of a code block. */
static unsigned int calculate_line_checksum(uint8_t *block);

/* Open and memory map a rom binary file from a file location. */
static uint8_t *memory_map_rom_file(char *file_location, int *file_size);

/* Unload a rom binary file from memory. */
static inline void unmmap_rom_file(uint8_t *rom_file, unsigned int rom_size);

/* Create rom block table from an in memory representation rom_file. */
static rom_block *create_rom_block_table(uint8_t *rom_file,
    unsigned int *number_of_blocks);

/* Destrom rom block table. */
static inline void destroy_rom_block_table(rom_block *rom_block_table);

/* Display information about a rom block. */
static void display_rom_block(rom_block *block);

/* Verify the integrity of a rom block header. */
static int verify_rom_block_header(rom_block *block);

/* Verify the integrity of a rom block contents. */
static int verify_rom_block_contents(uint8_t *rom, rom_block *rom_block);

/* Serialise rom block header array. */
static int serialise_formatted_rom_block_header(char *rom_header_output_file,
    rom_block *rom_block_table, unsigned int number_of_blocks);

/* Serialise a raw chunk of data to an output file. */
static int serialise_raw_data(char *output_file_name, uint8_t *data,
    unsigned int size_in_bytes);

/* Output information of a specified rom_block to a provided file descriptor. */
static int output_rom_block_to_fd(rom_block *block, FILE *fd);

/* Desirialise formatted rom header file. */
static uint32_t desirialise_rom_table(char *rom_header_file, uint8_t *rom_mem,
    size_t *number_of_blocks);

/* Load a single block of rom from a provided rom_file.*/
static uint32_t load_rom_block_from_file(char *rom_file, uint8_t *rom_buffer,
    rom_block *block);

/* Create a rom image based on a provided rom_block_table array. */
static uint32_t create_rom_image(uint8_t *rom_image_buffer,
    size_t number_of_blocks);

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
    if (hdd_fd == -1) {
        fprintf(stderr, "dump_rom_image: Could not handle hard disk drive.\n");
        return -1;
    }

    if (identify_hard_disk_drive(hdd_fd) == -1) {
        fprintf(stderr, "dump_rom_image: Specified hard disk drive is " \
            "not supported\n");
        close(hdd_fd);
        return -1;
    }

    printf("Allocating memory for rom image\n");
    rom_image_buffer = calloc(ROM_IMAGE_SIZE, 1);
    if (rom_image_buffer == NULL) {
        perror("calloc:");
        close(hdd_fd);
        return -1;
    }

    printf("Enabling vendor specific commands\n");
    if (enable_vendor_specific_commands(hdd_fd) == -1) {
        fprintf(stderr, "dump_rom_image: Could not enable " \
            "vendor specific commands.\n");
        free(rom_image_buffer);
        close(hdd_fd);
        return -1;
    }

    printf("Getting access to the rom.\n");
    if (get_rom_acces(hdd_fd, ROM_KEY_READ) == -1) {
        fprintf(stderr, "dump_rom_image: Could not get rom read access.\n");
        free(rom_image_buffer);
        close(hdd_fd);
        return -1;
    }

    unsigned int i;

    printf("Dumping rom\n");
    /* Request the ROM image using four 64 KiB block requests. */
    for (i = 0; i < ROM_IMAGE_SIZE; i += ROM_IMAGE_BLOCK_SIZE) {
        printf("Dumping ROM block from offset: %d\n", i);
        if (read_rom_block(hdd_fd, &rom_image_buffer[i], ROM_IMAGE_BLOCK_SIZE)
            == -1) {
            fprintf(stderr, "dump_rom_image: Could not read rom block: %d\n",
                (i / ROM_IMAGE_BLOCK_SIZE));
            free(rom_image_buffer);
            close(hdd_fd);
            return -1;
        }
    }

    printf("Disabling vendor specific commands\n");
    if (disable_vendor_specific_commands(hdd_fd) == -1) {
        fprintf(stderr, "dump_rom_image: Could not disable " \
            "vendor specific commands.\n");
        free(rom_image_buffer);
        close(hdd_fd);
        return -1;
    }

    close(hdd_fd);

    /* Can potentially be replaced by serialise_raw_data */
    /*
    output_file = open(out_file, O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if (output_file == -1) {
        fprintf(stderr, "dump_rom_image: Could not create %s\n", out_file);
        return -1;
    }

    printf("Writing rom image to file\n");
    if (write(output_file, rom_image_buffer, ROM_IMAGE_SIZE) == -1) {
        fprintf(stderr, "dump_rom_image: Could not write to %s file\n",
            out_file);
        close(output_file);
        return -1;
    }

    close(output_file);
    */

    /* Alternative to above ^^ */
    if (serialise_raw_data(out_file, rom_image_buffer, ROM_IMAGE_SIZE) != 0) {
        fprintf(stderr, "dump_rom_image: Could not write extracted rom to " \
            "the disk.\n");
        return -1;
    }

    return 0;
}

/* Operations: */
/* Open the hard disk device file */
/* Check if device is a supported western digital disk*/
/* Open in_file */
/* Read in_file to rom buffer memory */
/* Close in_file*/
/* Enable vendor specific command */
/* Get rom access */
/* Loop and write contents of rom buffer to hard disk drive */
/* Disable vendor specif commands */
int upload_rom_image(char *hard_disk_dev_file, char *in_file)
{
    uint8_t *rom_image_buffer;
    int input_file;

    int hdd_fd = open_hard_disk_drive(hard_disk_dev_file);
    if (hdd_fd == -1) {
        fprintf(stderr, "upload_rom_image: Could not handle hard disk " \
            "drive.\n");
        return -1;
    }

    if (identify_hard_disk_drive(hdd_fd) == -1) {
        fprintf(stderr, "upload_rom_image: Specified hard disk drive is " \
            "not supported\n");
        close(hdd_fd);
        return -1;
    }

    printf("Allocating memory for rom image\n");
    rom_image_buffer = calloc(ROM_IMAGE_SIZE, 1);
    if (rom_image_buffer == NULL) {
        perror("calloc:");
        close(hdd_fd);
        return -1;
    }

    input_file = open(in_file, O_RDONLY);
    if (input_file < 0) {
        perror("open");
        return -1;
    }

    if (read(input_file, rom_image_buffer, ROM_IMAGE_SIZE) != 0) {
        perror("read");
        close(input_file);
        return -1;
    }

    printf("Enabling vendor specific commands\n");
    if (enable_vendor_specific_commands(hdd_fd) == -1) {
        fprintf(stderr, "upload_rom_image: Could not enable " \
            "vendor specific commands.\n");
        free(rom_image_buffer);
        close(hdd_fd);
        return -1;
    }

    printf("Errasing rom from disk.\n");
    if (get_rom_acces(hdd_fd, ROM_KEY_ERASE) == -1) {
        fprintf(stderr, "upload_rom_image: Could not get rom erase access.\n");
        free(rom_image_buffer);
        close(hdd_fd);
        return -1;
    }

    printf("Getting access to rom.\n");
    if (get_rom_acces(hdd_fd, ROM_KEY_WRTIE) == -1) {
        fprintf(stderr, "upload_rom_image: Could not get rom write eaccess.\n");
        free(rom_image_buffer);
        close(hdd_fd);
        return -1;
    }

    unsigned int i;

    printf("Uploading rom image\n");
    /* Request the ROM image using 64KiB block requests. */
    for (i = 0; i < ROM_IMAGE_SIZE; i += ROM_IMAGE_BLOCK_SIZE) {
        printf("Writing ROM block to offset: %d\n", i);
        if (write_rom_block(hdd_fd, &rom_image_buffer[i], ROM_IMAGE_BLOCK_SIZE)
            == -1) {
            fprintf(stderr, "upload_rom_image: Could not write rom block: %d\n",
                (i / ROM_IMAGE_BLOCK_SIZE));
            free(rom_image_buffer);
            close(hdd_fd);
            return -1;
        }
    }

    printf("Disabling vendor specific commands\n");
    if (disable_vendor_specific_commands(hdd_fd) == -1) {
        fprintf(stderr, "upload_rom_image: Could not disable " \
            "vendor specific commands.\n");
        free(rom_image_buffer);
        close(hdd_fd);
        return -1;
    }

    close(hdd_fd);
    return 0;
}

/* Operations: */
/* Map contents of rom_image to memory */
/* Create array of rom header structures */
/* Create a directory to store extracted data */
/* Store a copy of the rom file in the created directory*/
/* Create a human readble header file that is used for making rom adjustments */
/* Serialise rom header array to output file */
/* Use rom header array to extract and save rom blocks to the disk */
/* Destroy array of rom header structures */
/* Free contents of rom_image from memory */
int unpack_rom_image(char *rom_image)
{
    uint8_t *rom_memory;
    rom_block *rom_header_table;
    int file_size;
    unsigned int number_of_blocks = 0;
    struct stat st = {0};
    char rom_block_file_name[] = "block_xx"; /* Placeholder name */
    char *string_parse_temp;
    char temp_string[255] = {0};
    char copy_file_name[255] = {0};
    char *rom_image_dir;
    uint8_t *temp_rom_block;

    memcpy(temp_string, rom_image, strlen(rom_image));

    /* Define a name for the upper directory by using the name of the rom
    *  file whilst ommiting the file type specifier. */
    if ((string_parse_temp = strrchr(temp_string, '/')) != NULL) {
        if (strlen(string_parse_temp) >= 255) {
            fprintf(stderr, "unpack_rom_image: File name of %s is to long\n",
                string_parse_temp);
            return -1;
        }
        strncpy(copy_file_name, string_parse_temp + 1,
            strlen(string_parse_temp));

        rom_image_dir = strtok(string_parse_temp + 1, ".");
    } else {
        rom_image_dir = strtok(temp_string, ".");
    }

    printf("Output directory is: %s\n", temp_string);
    printf("Output file is: %s\n", copy_file_name);

    printf("Mapping %s to memory\n", rom_image);
    if ((rom_memory = memory_map_rom_file(rom_image, &file_size)) == NULL) {
        fprintf(stderr, "unpack_rom_image: Could not load rom " \
            "image: %s\n", rom_image);
        return -1;
    }

    printf("Identifying the rom block header table\n");
    if ((rom_header_table =
        create_rom_block_table(rom_memory, &number_of_blocks)) == NULL) {
        fprintf(stderr, "unpack_rom_image: Could not create rom header " \
            "table.\n");
        unmmap_rom_file(rom_memory, file_size);
        return -1;
    }

    if (stat(rom_image_dir, &st) == -1) {
        if (mkdir(rom_image_dir, 0777) == -1) {
            perror("unpack_rom_image: mkdir");

            unmmap_rom_file(rom_memory, file_size);
            destroy_rom_block_table(rom_header_table);
            return -1;
        }
    }

    /* Used to remove the requirement of specifying rom_image_dir before each
     * file that is created after this call. */
    if (chdir(rom_image_dir) == -1) {
        perror("unpack_rom_image: chdir");
        unmmap_rom_file(rom_memory, file_size);
        destroy_rom_block_table(rom_header_table);
        return -1;
    }

    printf("Making copy of %s\n", rom_image);
    if (serialise_raw_data(copy_file_name, rom_memory, file_size) == -1) {
        fprintf(stderr, "unpack_rom_image: Could not make a copy of %s\n",
            rom_image);
        unmmap_rom_file(rom_memory, file_size);
        destroy_rom_block_table(rom_header_table);
        return -1;
    }

    printf("Writing rom block header to disk.\n");
    if (serialise_formatted_rom_block_header("formatted_header",
        rom_header_table, number_of_blocks) == -1) {
        fprintf(stderr, "unpack_rom_image: Could not serialise formatted rom " \
            "block header.\n");
        unmmap_rom_file(rom_memory, file_size);
        destroy_rom_block_table(rom_header_table);
        return -1;
    }

    size_t header_name_size = sizeof("_block_header") + strlen(rom_image_dir);
    char block_header_name[header_name_size];
    snprintf(block_header_name, header_name_size, "%s_block_header", rom_image_dir);

    if (serialise_raw_data(block_header_name, rom_memory,
        number_of_blocks * sizeof(rom_block)) == -1) {
        fprintf(stderr, "unpack_rom_image: Could not serialise rom block " \
            "header.\n");
        unmmap_rom_file(rom_memory, file_size);
        destroy_rom_block_table(rom_header_table);
        return -1;
    }

    printf("Extracting and writing rom blocks to disk.\n");
    int i;
    for (i = 0; i < number_of_blocks; ++i) {
        snprintf(rom_block_file_name + 6, 3, "%x",
            rom_header_table[i].block_nr);

        printf("Extracting rom block %#x from %s\n",
            rom_header_table[i].block_nr, rom_image);

        temp_rom_block = (uint8_t *) malloc(rom_header_table[i].size);
        memcpy(temp_rom_block, rom_memory + rom_header_table[i].start_address,
            rom_header_table[i].size);

        printf("Writing %s to disk.\n", rom_block_file_name);
        if ((serialise_raw_data(rom_block_file_name,
            temp_rom_block, rom_header_table[i].size)) == -1) {
            fprintf(stderr, "unpack_rom_image: Could not serialise rom " \
                "block %#x\n", rom_header_table[i].block_nr);
            free(temp_rom_block);
            unmmap_rom_file(rom_memory, file_size);
            destroy_rom_block_table(rom_header_table);
            return -1;
        }

        free(temp_rom_block);
    }

    unmmap_rom_file(rom_memory, file_size);
    destroy_rom_block_table(rom_header_table);

    return 0;
}

/* Should this be a ini like file or just a text version of the -i option? */
static int serialise_formatted_rom_block_header(char *rom_header_output_file,
    rom_block *rom_block_table, unsigned int number_of_blocks)
{
    FILE *output_file = fopen(rom_header_output_file, "w");
    if (output_file == NULL) {
        fprintf(stderr, "serialise_formatted_rom_block_header: Could not " \
            " create %s.\n", rom_header_output_file);
        return -1;
    }

    unsigned int i;
    for (i = 0; i < number_of_blocks; ++i) {
        output_rom_block_to_fd(&rom_block_table[i], output_file);

        if (i + 1 != number_of_blocks) {
            /* Line seperation used to identify new blocks */
            fprintf(output_file, "\n");
        }
    }

    fclose(output_file);
    return 0;
}

static int output_rom_block_to_fd(rom_block *block, FILE *fd)
{
    fprintf(fd, "Block number:               %#x\n", block->block_nr);
    fprintf(fd, "Encryption flag:            %#x\n", block->flag);
    fprintf(fd, "Unkown 1:                   %#x\n", block->unk1);
    fprintf(fd, "Unkown 2:                   %#x\n", block->unk2);
    fprintf(fd, "Block length plus checksum: %#x\n",
        le_32_to_be(block->length_plus_cs));
    fprintf(fd, "Block size:                 %#x\n", le_32_to_be(block->size));
    fprintf(fd, "Block start address:        %#x\n",
        le_32_to_be(block->start_address));
    fprintf(fd, "Block load address:         %#x\n",
        le_32_to_be(block->load_address));
    fprintf(fd, "Block execution address:    %#x\n",
        le_32_to_be(block->execution_address));
    fprintf(fd, "Unkown 3:                   %#x\n", block->unk3);
    fprintf(fd, "Block checksum:             %#x\n", block->fstw_plus_cs);
    return 0;
}


static int serialise_raw_data(char *output_file_name, uint8_t *data,
    unsigned int size_in_bytes)
{
    int output_file = open(output_file_name, O_CREAT | O_WRONLY |
        O_TRUNC, 0777);
    if (output_file == -1) {
        fprintf(stderr, "serialise_raw_data: Could not create %s.\n",
            output_file_name);
        return -1;
    }

    if(write(output_file, data, size_in_bytes) == -1) {
        fprintf(stderr, "serialise_raw_data: Could not write to " \
            "%s file\n", output_file_name);
        close(output_file);
        return -1;
    }

    close(output_file);
    return 0;
}

/* Operations: */
/* Open rom_header_file */
/* Itterate: */
/* - Read contents of block header description */
/* - Write block to rom_mom */
/* - Increase number_of_blocks */
/* Close rom_header_file */
static uint32_t desirialise_rom_table(char *rom_header_file, uint8_t *rom_mem,
    size_t *number_of_blocks)
{
    *number_of_blocks = 0;
    rom_block block = {0};
    FILE *fp;
    char *line = NULL;
    size_t write_offset = 0;

    fp = fopen(rom_header_file, "r");
    if (fp == NULL) {
        fprintf(stderr, "desirialise_rom_table: Could not open formatted " \
            "header\n");
        return -1;
    }

    while (getline(&line, 0, fp) != -1) {
        if (strncmp(line, "Block number:", sizeof("Block number:") - 1) == 0) {
            block.block_nr  = strtol(line + 28, NULL, 16);
        }else if (strncmp(line, "Encryption flag:",
            sizeof("Encryption flag:") - 1) == 0) {
            block.flag  = strtol(line + 28, NULL, 16);
        }  else if (strncmp(line, "Unkown 1:", sizeof("Unkown 1:") - 1) == 0) {
            block.unk1  = strtol(line + 28, NULL, 16);
        } else if (strncmp(line, "Unkown 2:",
            sizeof("Unkown 2:") - 1) == 0) {
            block.unk2  = strtol(line + 28, NULL, 16);
        } else if (strncmp(line, "Block length plus checksum:",
            sizeof("Block length plus checksum:") - 1) == 0) {
            block.length_plus_cs  = strtol(line + 28, NULL, 16);
        } else if (strncmp(line, "Block size:",
            sizeof("Block size:") - 1) == 0) {
            block.size  = strtol(line + 28, NULL, 16);
        } else if (strncmp(line, "Block start address:",
            sizeof("Block start address:") - 1) == 0) {
            block.start_address  = strtol(line + 28, NULL, 16);
        } else if (strncmp(line, "Block load address:",
            sizeof("Block load address:") - 1) == 0) {
            block.load_address  = strtol(line + 28, NULL, 16);
        } else if (strncmp(line, "Block execution address:",
            sizeof("Block execution address:") - 1) == 0) {
            block.execution_address  = strtol(line + 28, NULL, 16);
        } else if (strncmp(line, "Unkown 3:", sizeof("Unkown 3:") - 1) == 0) {
            block.unk3  = strtol(line + 28, NULL, 16);
        } else if (strncmp(line, "Block checksum:",
            sizeof("Block checksum:") - 1) == 0) {
            block.fstw_plus_cs  = strtol(line + 28, NULL, 16);
        }

        if(line[0] == '\n') {
            memcpy(rom_header_file + write_offset, &block, sizeof(rom_block));
            memset(&block, 0, sizeof(block));
            write_offset += sizeof(rom_block);
            number_of_blocks += 1;
        }
    }

    memcpy(rom_header_file + write_offset, &block, sizeof(rom_block));
    number_of_blocks += 1;

    fclose(fp);
    if (line != NULL) {
        free(line);
    }

    return 0;
}

int modify_instruction(char *rom_image, uint32_t memory_adress,
	uint32_t new_instruction, uint32_t instruction_byte_size)
{
    int file_size;
    uint8_t *rom_mem;

    rom_mem = memory_map_rom_file(rom_image, &file_size);

    if (rom_mem == NULL) {
        fprintf(stderr, "modify_instruction: Could not memory map file.\n");
        return -1;
    }

    if (memcpy(rom_mem + memory_adress, &new_instruction,
        instruction_byte_size) != 0) {
        perror("memcpy");
        unmmap_rom_file(rom_mem, file_size);
        return -1;
    }

    /* Write rom_mem to file */
    if (serialise_raw_data(rom_image, rom_mem, file_size) != 0) {
        fprintf(stderr, "modify_instruction: Could not save modifications");
        return -1;
    }

    /* Unnmap rom_mem buffer once done */
    unmmap_rom_file(rom_mem, file_size);

    return 0;
}

/* Operations: */
/* Open rom_file file */
/* Read rom_block_size from rom_file file descriptor to block */
/* Close rom_file */
static uint32_t load_rom_block_from_file(char *rom_file, uint8_t *rom_buffer,
    rom_block *block)
{
    FILE *file;
    void *write_location = rom_buffer + block->start_address;

    file = fopen(rom_file, "r");
    if (rom_file == NULL) {
        perror("load_rom_block_from_file: fopen");
        return -1;
    }

    if (fread(write_location, block->length_plus_cs, 1, file) != 0) {
        perror("load_rom_block_from_file: fread");
        fclose(file);
        return -1;
    }

    fclose(file);

    return 0;
}

/* Operations: */
/* Create rom memory buffer of size ROM_IMAGE_SIZE */
/* Call desirialise_rom_table with as parameter the rom_block_format_file */
/* Call create_rom_image function to place the rom blocks into the rom
    memory buffer. */
/* Recalculate checksums for each block and table line */
/* Create out_file */
/* Write rom memory buffer to out_file */
/* Close out_file */
/* Free contents of rom memory buffer */
int pack_rom_image(char *rom_header_file, char *out_file)
{
    size_t number_of_blocks;

    uint8_t *rom_memory_buffer = calloc(ROM_IMAGE_SIZE, 1);
    if (rom_memory_buffer == NULL) {
        perror("pack_rom_image: calloc");
        return -1;
    }

    if (desirialise_rom_table(rom_header_file, rom_memory_buffer,
        &number_of_blocks) != 0) {
        fprintf(stderr, "pack_rom_image: Could not desirialise rom table\n");
        free(rom_memory_buffer);
        return -1;
    }

    if(create_rom_image(rom_memory_buffer, number_of_blocks) != 0) {
        fprintf(stderr, "pack_rom_image: Could not create a rom image.\n");
        free(rom_memory_buffer);
        return -1;
    }

    if(serialise_raw_data(out_file, rom_memory_buffer, ROM_IMAGE_SIZE) != 0) {
        fprintf(stderr, "pack_rom_image: Could not write rom image to disk\n");
        free(rom_memory_buffer);
        return -1;
    }

    unmmap_rom_file(rom_memory_buffer, ROM_IMAGE_SIZE);

    free(rom_memory_buffer);
    return 0;
}


/* Operations: */
/* Create rom_image_buffer of size ROM_IMAGE_SIZE */
/* For each block in rom_block_table: */
    /* Lookup size of block */
    /* Lookup offset of block */
    /* Open file with name 'block_' + block_id */
    /* Write block to rom_image_buffer at correct offset */
    /* Close block file */
static uint32_t create_rom_image(uint8_t *rom_image_buffer,
    size_t number_of_blocks)
{
    if (rom_image_buffer == NULL) {
        perror("calloc");
        return -1;
    }

    rom_block *rom_block_table = (rom_block *) rom_image_buffer;

    char rom_block_file_name[] = "block_xx"; /* Placeholder name */

    int i;
    for (i = 0; i < number_of_blocks; ++i) {

        if ((rom_block_table[i].start_address +
            rom_block_table[i].length_plus_cs) > ROM_IMAGE_SIZE) {
            fprintf(stderr, "create_rom_image: rom block %#x is to large\n",
                rom_block_table[i].block_nr);
            return -1;
        }

        /* Construct character string name of the block to append to the
           buffer. */
        snprintf(rom_block_file_name + 6, 3, "%x",
            rom_block_table[i].block_nr);

        if (load_rom_block_from_file(rom_block_file_name, rom_image_buffer,
            &rom_block_table[i]) != 0) {
            fprintf(stderr, "Could not copy %s to rom_image_buffer\n",
                rom_block_file_name);
            return -1;
        }
    }

    return 0;
}

/* Operations: */
/* Open provided file_location */
/* Map contents of file_location to memory map */
/* Close file_location */
/* Return memory mapped rom image */
static uint8_t *memory_map_rom_file(char *file_location, int *file_size)
{
    uint8_t *rom_memory;
    int fd = open(file_location, O_RDONLY);

    if (fd < 1) {
        perror("open");
        return NULL;
    }

    *file_size = lseek(fd, 0, SEEK_END);

    rom_memory = mmap(NULL, *file_size, PROT_READ,
        MAP_SHARED, fd, 0);

    if (rom_memory == NULL) {
        perror("mmap");
    }

    close(fd);
    return rom_memory;
}

static inline void unmmap_rom_file(uint8_t *rom_file, unsigned int rom_size)
{
    if (rom_file != NULL) {
        munmap(rom_file, rom_size);
        rom_file = NULL;
    }
}

/* Operations: */
/* Map contents of rom_image to memory */
/* Create array of rom header structures */
/* Display array of rom header structures (NUMBER_OF_HEADERS times) */
/* Destroy array of rom header structures */
int display_rom_info(char *rom_image)
{
    uint8_t *rom_memory;
    rom_block *rom_header_table;
    int file_size;
    unsigned int number_of_headers = 0;

    if ((rom_memory = memory_map_rom_file(rom_image, &file_size)) == NULL) {
        fprintf(stderr, "display_rom_info: Could not load rom " \
            "image: %s\n", rom_image);
        return -1;
    }

    if ((rom_header_table =
        create_rom_block_table(rom_memory, &number_of_headers)) == NULL) {
        fprintf(stderr, "display_rom_info: Could not create rom header " \
            "table.\n");
        unmmap_rom_file(rom_memory, file_size);
        return -1;
    }

    int i;
    for (i = 0; i < number_of_headers; ++i) {
        display_rom_block(&rom_header_table[i]);
        verify_rom_block_header(&rom_header_table[i]);
        verify_rom_block_contents(rom_memory, &rom_header_table[i]);
        printf("\n");
    }

    printf("End of the rom block header: %#lx\n",
        (number_of_headers * sizeof(rom_block)));

    destroy_rom_block_table(rom_header_table);
    unmmap_rom_file(rom_memory, file_size);

    return 0;
}

/* Little endian to big endian.
 * Source:
 * https://stackoverflow.com/questions/19275955/convert-little-endian-to-big-endian/19276193 */
static inline uint32_t le_32_to_be(uint32_t integer)
{
	unsigned char *p=(unsigned char*)&integer;
	return p[0]+(p[1]<<8)+(p[2]<<16)+(p[3]<<24);
}

/* Big endian to little endian.
 * Source:
 * https://stackoverflow.com/questions/2182002/convert-big-endian-to-little-endian-in-c-without-using-provided-func
 */
static uint32_t be_32_to_le(uint32_t integer)
{
	uint32_t ret;
	unsigned char *p=(unsigned char *)&ret;
	p[0]=(integer)&0xff;
	p[1]=(integer>>8)&0xff;
	p[2]=(integer>>16)&0xff;
	p[3]=(integer>>24)&0xff;
	return ret;
}
static void display_rom_block(rom_block *block)
{
    printf("Block number:               %#x\n", block->block_nr);
    printf("Encryption flag:            %#x\n", block->flag);

    printf("Block encrypted:            %s\n",
        (block->flag == FLAG_UNENCRYPTED) ? "no" : "yes");

    printf("Unkown 1:                   %#x\n", block->unk1);
    printf("Unkown 2:                   %#x\n", block->unk2);
    printf("Block length plus checksum: %#x\n",
        le_32_to_be(block->length_plus_cs));
    printf("Block size:                 %#x\n", le_32_to_be(block->size));
    printf("Block start address:        %#x\n",
        le_32_to_be(block->start_address));
    printf("Block load address:         %#x\n",
        le_32_to_be(block->load_address));
    printf("Block execution address:    %#x\n",
        le_32_to_be(block->execution_address));
    printf("Unkown 3:                   %#x\n", block->unk3);
    printf("Block checksum:             %#x\n", block->fstw_plus_cs);
}

static int verify_rom_block_header(rom_block *block)
{
    uint8_t checksum = calculate_line_checksum((uint8_t *) block);

    if (checksum != ((uint8_t *) block)[31]) {
        fprintf(stderr, "Rom block checksum FAIL: %#x != %#x\n",
            checksum, ((uint8_t *) block)[31]);
        fprintf(stderr, "Rom block memory is corrupted.\n");
        return -1;
    } else {
        printf("Rom block header checksum OK:   %#x\n", checksum);
        return 0;
    }
}

/* Check performed to verfiry the integrity of each rom block for memory
 * corruption. */
static int verify_rom_block_contents(uint8_t *rom, rom_block *rom_block)
{
    int checksum;
    int rom_contents;

    if ((rom_block->length_plus_cs - rom_block->size) == 1) {
        checksum = calculate_rom_block_checksum_8(
            &rom[rom_block->start_address], rom_block->size);

        rom_contents = rom[rom_block->start_address +
            rom_block->size];
    } else {
        fprintf(stderr, "verify_rom_block_contents: Detected irregular " \
            "checksum size.\n");
        return -1;
    }

    if (checksum != rom_contents) {
        fprintf(stderr, "verify_rom_block_contents: Checksum fail: "\
            "%#x != %#x.\n", checksum, rom_contents);
    } else {
        printf("Rom block contents checksum OK: %#x\n", checksum);
    }

    return 0;
}

static unsigned int calculate_line_checksum(uint8_t *block)
{
    uint8_t checksum = 0;

    int i;
    for (i = 0; i < 31; ++i) {
        checksum += block[i];
    }

    return checksum;
}

static rom_block *create_rom_block_table(uint8_t *rom_file,
    unsigned int *number_of_blocks)
{
    unsigned int table_size;
    int i = 0;

    /* Observed block numbers in the extracted (rom) firmware images. */
    while (((rom_block *) rom_file)[i].block_nr <= 0x0a ||
        ((rom_block *) rom_file)[i].block_nr== 0x5a) {
        ++i;
    }
    *number_of_blocks = i;

    table_size = sizeof(rom_block) * *number_of_blocks;
    rom_block *rom_block_table = (rom_block *) malloc(table_size);

    if (rom_block_table == NULL) {
        perror("create_rom_block_table: malloc");
        return NULL;
    }

    memcpy(rom_block_table, rom_file, table_size);
    return rom_block_table;
}

static inline void destroy_rom_block_table(rom_block *rom_block_table)
{
    if (rom_block_table != NULL) {
        free(rom_block_table);
        rom_block_table = NULL;
    }
}

/* https://forum.arduino.cc/index.php?topic=486751.0 */
/* https://reverseengineering.stackexchange.com/questions/15484/simple-8bit-checksum */
static unsigned int calculate_rom_block_checksum_8(uint8_t *block,
    unsigned int size)
{
    uint8_t checksum = 0;
    int i;

    for (i = 0; i < size; ++i) {
        checksum += block[i];
    }

    return checksum;
}
