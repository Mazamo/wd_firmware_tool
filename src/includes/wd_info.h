#ifndef WD_INFO_H
#define WD_INFO_H

#define VENDOR_SPECIFIC_KEY_WRITE 0x02
#define VENDOR_SPECIFIC_KEY_READ  0x01

/* Used ROM type.
 * This tool is made for disk only using external rom. For internal
 * rom use 0x02
 */
#define ROMTYPE_SPI 0x03

char *supported_hard_disk_drives[] =
    {
        "wd800jd",
        "wd1600aajs",
    };

#endif
