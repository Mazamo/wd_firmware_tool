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

/* Check if the process has enough privilidges to perform an action */
static inline int check_root_permission(void);

int main(int argc, char *argv[])
{
    if (argc < 2) {
        display_options(argv[0]);
        exit(1);
    }

    /* Option: Dump */
    if (strcmp(argv[1], "-d") == 0) {
        if (argc != 4) {
            display_options(argv[0]);
            exit(1);
        }

        if (check_root_permission() != 0) {
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
    }
    else if (strcmp(argv[1], "-l") == 0) {
        if (argc != 4) {
            display_options(argv[0]);
            exit(1);
        }

        if (check_root_permission() != 0) {
            fprintf(stderr, "main: Application should be run as root for " \
                "this operation.\n");
            exit(1);
        }

        /* argv[2] = hard disk location */
        /* argv[3] = input binary file */
        if (upload_rom_image(argv[2], argv[3]) == -1) {
            fprintf(stderr, "main: Could not upload firmware image to the " \
                " the hard disk disk drive.\n");
            exit(1);
        }

        printf("Finished uploading rom image to %s\n", argv[3]);
    }
    else if (strcmp(argv[1], "-i") == 0) {

    }
    else if (strcmp(argv[1], "-p") == 0) {

    }
    else if (strcmp(argv[1], "-d") == 0) {

    }
    else if (strcmp(argv[1], "-a") == 0) {

    }
    else if (strcmp(argv[1], "-s") == 0) {
        if (check_root_permission() != 0) {
            fprintf(stderr, "main: Application should be run as root for " \
                "this operation.\n");
            exit(1);
        }

        scan_hard_disk_drives();
    }
    else {
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

                if (fd != -1) {
                    close(fd);
                }
            }
        }
        closedir(dev_directory);
    }
}

void display_options(char *app_name)
{
    printf("Usage:\n");
    printf("Dump ROM image: %s -d <hard disk location> <filename>\n",
        app_name);
    printf("Print info blocks: %s -i <rom file>\n", app_name);
    printf("Load ROM image: %s -l <hard disk location> <rom file>\n",
        app_name);
    printf("Pack image: %s -p <rom file>\n", app_name);
    printf("Add block: %s -p <rom file> <block file>\n", app_name);
    printf("Hard disk scan: %s -s\n", app_name);
}

static inline int check_root_permission(void)
{
    if (getuid() == 0) {
        return 0;
    }
    else {
        return 1;
    }
}
