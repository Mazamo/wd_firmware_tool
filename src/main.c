/* Generic libraries */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <ctype.h>

/* Application specific */
#include "includes/rom_management.h"

int scan_hard_disk_drives(void)
{

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
    return;
}

int main(int argc, char *argv[])
{
    if(argc < 2)
    {
        display_options(argv[0]);
        exit(1);
    }

    /* Option: Dump */
    if(strcmp(argv[1], "-d") == 0)
    {
        if(argc != 4)
        {
            display_options(argv[0]);
            exit(1);
        }

        /* argv[2] = hard disk location */
        /* argv[3] = output file */
        if(dump_rom_image(argv[2], argv[3]) == -1)
        {
            fprintf(stderr, "main: Could not dump rom image.\n");
            exit(1);
        }
    }
    else if(strcmp(argv[1], "-l") == 0)
    {

    }
    else if(strcmp(argv[1], "-i") == 0)
    {

    }
    else if(strcmp(argv[1], "-p") == 0)
    {

    }
    else if(strcmp(argv[1], "-d") == 0)
    {

    }
    else if(strcmp(argv[1], "-a") == 0)
    {

    }
    else if(strcmp(argv[1], "-s") == 0)
    {

    }
    else
    {
        display_options(argv[0]);
        exit(1);
    }

    return 0;
}
