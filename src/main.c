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
#include <dirent.h>

/* Application specific */
#include "includes/rom_management.h"
#include "includes/disk_communication.h"

/* Function prototypes: */

 /* Scan for all connected hard disk drive */
static void scan_hard_disk_drives(void);

/* Display the application's options */
static void display_options(char *app_name);

/* Read a LBA block from the specified hard disk drive. */
int read_lba_block(char *hard_disk_dev_file, unsigned long lba_id);

/* Write a LBA block from the specified hard disk drive. */
int write_lba_block(char *hard_disk_dev_file, unsigned long lba_id,
	uint8_t *data_buffer, size_t size);

int main(int argc, char *argv[])
{
    if (argc < 2) {
        display_options(argv[0]);
        exit(1);
    }

    /* Option: Dump rom contents from hard disk drive */
    if (strcmp(argv[1], "-d") == 0) {
        if (argc != 4) {
            display_options(argv[0]);
            exit(1);
        }

        if (getuid() != 0) {
            fprintf(stderr, "main: Application should be run as root for " \
                "this operation.\n");
            exit(1);
        }

        /* argv[2] = hard disk location */
        /* argv[3] = output file */
        if (dump_rom_image(argv[2], argv[3]) == -1) {
            fprintf(stderr, "main: Could not dump rom image from the hard " \
                "disk drive.\n");
            exit(1);
        }

        printf("Finished dumping rom from %s\n", argv[3]);
	/* Option: Load rom file to hard disk drive */
    } else if (strcmp(argv[1], "-l") == 0) {
		if (argc != 4) {
			display_options(argv[0]);
			exit(1);
		}

		if (getuid() != 0) {
			fprintf(stderr, "main: Application should be run as root for " \
				"this operation.\n");
			exit(1);
		}

		/* argv[2] = hard disk location */
		/* argv[3] = input file */
		if (upload_rom_image(argv[2], argv[3]) == -1) {
			fprintf(stderr, "main: Could not upload rom image to the hard " \
				"disk drive.\n");
			exit(1);
		}

		printf("Finished uploading rom from %s\n", argv[3]);
	/* Option: print info rom blocks */
    } else if (strcmp(argv[1], "-i") == 0) {
		if (argc != 3) {
			display_options(argv[0]);
			exit(1);
		}

		if (display_rom_info(argv[2]) != 0) {
			fprintf(stderr, "main: Could not display information about the " \
				"provided binary file.\n");
			exit(1);
		}
	/* Option: Pack a rom image based on a rom block table file */
    } else if (strcmp(argv[1], "-p") == 0) {
		if (argc != 4) {
			display_options(argv[0]);
			exit(1);
		}

		if (pack_rom_image(argv[3], argv[4]) != 0) {
			fprintf(stderr, "main: Could not pack rom image %s using rom " \
				"header %s \n", argv[3], argv[4]);
			exit(1);
		}

		printf("Successfully packed rom image %s using the %s rom header " \
			"file \n", argv[4], argv[3]);
	/* Modify instruction in a file */
	} else if (strcmp(argv[1], "-m") == 0) {
		if (argc != 4) {
			display_options(argv[0]);
			exit(1);
		}

		uint32_t address = strtol(argv[3], NULL, 16);
		uint32_t instruction = strtol(argv[4], NULL, 16);
		uint32_t instruction_length = strnlen(argv[4], 4);

		if (modify_instruction(argv[2], address, instruction,
			instruction_length) != 0) {
			fprintf(stderr, "main: Could not modify an instruction in %s\n",
				argv[2]);
			exit(1);
		}

		printf("Successfully modified an instruction in %s\n", argv[2]);
	/* Option: Unpack a rom image */
	} else if (strcmp(argv[1], "-u") == 0) {
		if (argc != 3) {
			display_options(argv[0]);
			exit(1);
		}

		printf("Unpacking rom image\n");
		if (unpack_rom_image(argv[2]) != 0) {
			fprintf(stderr, "main: Could not unpack rom image\n");
			exit(1);
		}

		printf("Finished extracting %s rom image\n", argv[2]);
	/* Option: Scan connected hard disk drives */
    } else if (strcmp(argv[1], "-s") == 0) {
        if (getuid() != 0) {
            fprintf(stderr, "main: Application should be run as root for " \
                "this operation.\n");
            exit(1);
        }

        scan_hard_disk_drives();
	/* Read LBA from a hard disk drive */
    } else if (strcmp(argv[1], "-r") == 0) {
        if (argc != 4) {
            display_options(argv[0]);
            exit(1);
        }

        if (getuid() != 0) {
            fprintf(stderr, "main: Application should be run as root for " \
                "this operation.\n");
            exit(1);
        }

        unsigned long block_id;
        if (argv[3][0] == '0' && argv[3][1] == 'x') {
            block_id = strtol(argv[3], NULL, 16);
        } else {
            block_id = strtol(argv[3], NULL, 10);
        }

        if (read_lba_block(argv[2], block_id) == -1) {
            fprintf(stderr, "main: Could not read lba block %s from %s\n",
                argv[3], argv[0]);
            exit(1);
        }
	/* Write LBA to a hard disk drive */
    } else if (strcmp(argv[1], "-w") == 0) {
        if (argc != 5) {
            printf("%d\n", argc);
            display_options(argv[0]);
            exit(1);
        }

        unsigned int size;

        if ((size = strlen(argv[2])) > 512) {
            fprintf(stderr, "main: LBA input must be equal to or shorter " \
                "than 512 bytes.\n");
            exit(1);
        }

        if (getuid() != 0) {
            fprintf(stderr, "main: Application should be run as root for " \
                "this operation.\n");
            exit(1);
        }

        unsigned long block_id;
        if (argv[3][0] == '0' && argv[3][1] == 'x') {
            block_id = strtol(argv[3], NULL, 16);
        } else {
            block_id = strtol(argv[3], NULL, 10);
        }

        if (write_lba_block(argv[2], block_id, (uint8_t *) argv[4],
            size) == -1) {
            fprintf(stderr, "main: Could not write lba block %s to %s\n",
                argv[2], argv[0]);
            exit(1);
        }
    } else {
        display_options(argv[0]);
        exit(1);
    }

    return 0;
}

void scan_hard_disk_drives(void)
{
    DIR *dev_directory;
    struct dirent *directory;
    int fd;

    char dev_prefix[] = "/dev/";

    dev_directory = opendir(dev_prefix);
    if (dev_directory) {
        while ((directory = readdir(dev_directory)) != NULL) {
            if (!(directory->d_type == DT_BLK ||
                directory->d_type == DT_UNKNOWN)) {
                continue;
            }

            /* Used to exclude partitions (example sda1, sda2, ..., sdaN). */
            if (strlen(directory->d_name) != 3) {
                continue;
            }

            if (strncmp("sd", directory->d_name, 2) == 0) {
                char filename[strlen(dev_prefix) +
                    strlen(directory->d_name) + 1];

                memcpy(filename, dev_prefix, strlen(dev_prefix));
                memcpy(filename + strlen(dev_prefix), directory->d_name,
                    strlen(directory->d_name) + 1);

                printf("%s:\n", filename);
                fd = open_hard_disk_drive(filename);
                identify_hard_disk_drive(fd);

                if (fd != -1) {
                    close(fd);
                }
            }
        }
        closedir(dev_directory);
    }
}

/* Should be called only when DMA is supported */
/* LBA_ID should be less then MAXIMUM LBA RANGE ENTRY */
int read_lba_block(char *hard_disk_dev_file, unsigned long lba_id)
{
    uint8_t lba_data_buffer[512] = {0};

    int hdd_fd = open_hard_disk_drive(hard_disk_dev_file);
    if (hdd_fd == -1) {
        fprintf(stderr, "read_lba_block: Could not handle hard disk drive.\n");
        return -1;
    }

    if (read_dma_ext(hdd_fd, lba_id, lba_data_buffer,
        sizeof(lba_data_buffer)) != 0) {
        fprintf(stderr, "read_lba_block: Could not display LBA block %ld\n",
            lba_id);
        return -1;
    }

    close(hdd_fd);

    printf("Read the following from LBA block %ld:\n", lba_id);

    int i;
    for (i = 0; i < sizeof(lba_data_buffer); ++i) {
        if (i > 0 && (i % 16) == 0) {
            printf("\n");
        }
        printf("0x%-4x ", lba_data_buffer[i]);
    }
    printf("\n");

    return 0;
}

int write_lba_block(char *hard_disk_dev_file, unsigned long lba_id,
	uint8_t *data_buffer, size_t size)
{
    uint8_t lba_data_buffer[512] = {0};
    memcpy(lba_data_buffer, data_buffer, size);

    printf("Writing the following to LBA block %ld:\n", lba_id);

    int i;
    for (i = 0; i < sizeof(lba_data_buffer); ++i) {
        if (i > 0 && (i % 16) == 0) {
            printf("\n");
        }
        printf("0x%-4x ", lba_data_buffer[i]);
    }
    printf("\n");

    int hdd_fd = open_hard_disk_drive(hard_disk_dev_file);
    if (hdd_fd == -1) {
        fprintf(stderr, "write_lba_block: Could not handle hard disk " \
            "drive.\n");
        return -1;
    }

    if (write_dma_ext(hdd_fd, lba_id, lba_data_buffer, size) != 0) {
        fprintf(stderr, "write_lba_block: Could not display LBA block " \
            "%ld\n", lba_id);
        return -1;
    }

    close(hdd_fd);

    return 0;
}

void display_options(char *app_name)
{
    printf("Usage:\n");
    printf("Dump ROM image: %s -d <hard disk location> <filename>\n",
        app_name);
    printf("Print info blocks: %s -i <rom file>\n", app_name);
    printf("Load ROM image: %s -l <hard disk location> <rom file>\n",
		app_name);
	printf("Unpack rom image: %s -u <rom file> \n", app_name);
    printf("Pack image: %s -p <rom file> <output file>\n", app_name);
	printf("Modify rom: %s -m <rom file>\n", app_name);
    printf("Hard disk scan: %s -s\n", app_name);
    printf("Read specific LBA: %s -r <hard disk location> <block number>\n",
        app_name);
    printf("Write specifc LBA: %s -w <hard disk location> <block number> " \
        "<data> (MUST be equal or less to 512 bytes)\n", app_name);
}
