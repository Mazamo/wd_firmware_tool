/* Generic libraries */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <ctype.h>

/* Linux specific */
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <scsi/sg.h>
#include <scsi/scsi_ioctl.h>

/* Application specific */
#include "includes/disk_communication.h"

/* Display the model number of the detected hard disk drive. */
static void display_model(uint8_t *hard_disk_response);

/* Display the firmware revision number of the detected hard disk drive. */
static void display_firmware_revision(uint8_t *hard_disk_response);

/* Display the serial number of the detected hard disk drive. */
static void display_serial_number(uint8_t *hard_disk_response);

/* Display the maximum LBA range entry number. */
static void display_number_of_lba_entries(uint8_t *hard_disk_response);

/* Display the sense buffer after an IOCTL fuction has been invoked. */
static inline void display_sense_buffer(unsigned char sense_buffer[32]);

/* Calculate the ID field of a sg_hdr based on the values of the cdb. */
static inline int calculate_pack_id(unsigned char *cdb);

int open_hard_disk_drive(char *hard_disk_dev_file)
{
    if (strncmp(hard_disk_dev_file, "/dev/s", sizeof("/dev/s") - 1) != 0) {
        fprintf(stderr, "open_hard_disk_drive: Invalid device file: %s.\n",
            hard_disk_dev_file);
        return -1;
    }

    int fd = open(hard_disk_dev_file, O_RDWR);
    if (fd == -1) {
        perror("open:");
        return -1;
    }

/*
    if (identify_hard_disk_drive(fd) == -1) {
        fprintf(stderr, "open_hard_disk_drive: Specified hard disk drive is " \
            "not supported\n");
        close(fd);
        return -1;
    }
*/

    return fd;
}

/*
* For more info about cdb and sg_io:
* http://www.t13.org/documents/uploadeddocuments/docs2006/d1699r3f-ata8-acs.pdf
* http://www.tldp.org/HOWTO/SCSI-Generic-HOWTO/sg_io_hdr_t.html
*/
int identify_hard_disk_drive(int hard_disk_file_descriptor)
{
    unsigned char identify_cdb[SG_ATA_16_LEN];

    identify_cdb[0]     = SG_ATA_16; /* operation code: SG_ATA_16 */

    /* multiple count: 0 protocol: 4 extended: 0  */
    /* protocol 4: PIO Data-In */
    identify_cdb[1]     = 0x08;

    /* off.line:00 cc:1 lh.en:1 lm.en:1 ll.en:1 sc.en:1 f.en:0 */
    /* cc 1: generate CHECK CONDITION when ATA command completes */
    /* */
    identify_cdb[2]     = 0x2e;

    identify_cdb[3]     = 0x00; /* Features (8:15): */
    identify_cdb[4]     = 0x00; /* Features (0:7): */
    identify_cdb[5]     = 0x00; /* Sector Count (8:15): */
    identify_cdb[6]     = 0x00; /* Sector Count (0:7): */
    identify_cdb[7]     = 0x00; /* LBA Low (8:15): */
    identify_cdb[8]     = 0x00; /* LBA Low (0:7): */
    identify_cdb[9]     = 0x00; /* LBA Mid (8:15): */
    identify_cdb[10]    = 0x00; /* LBA Mid (0:7): */
    identify_cdb[11]    = 0x00; /* LBA High (8:15): */
    identify_cdb[12]    = 0x00; /* LBA High (0:7): */
    identify_cdb[13]    = 0x40; /* Device: */
    identify_cdb[14]    = ATA_IDENTIFY; /* Command: Identify device */

    /* Control: auto cotingent allegiance not established */
    identify_cdb[15]    = 0x00;

    uint8_t identify_reply_buffer[512 * 2];
    memset(identify_reply_buffer, 0, sizeof(identify_reply_buffer));

    if (execute_command(identify_cdb, hard_disk_file_descriptor,
        identify_reply_buffer, 512, SG_DXFER_FROM_DEV) == -1) {
        fprintf(stderr, "identify_hard_disk_drive: Could not send identify " \
            "command to hard disk drive.\n");
        return -1;
    }

    display_model(identify_reply_buffer);
    display_firmware_revision(identify_reply_buffer);
    display_serial_number(identify_reply_buffer);
    display_number_of_lba_entries(identify_reply_buffer);

    return verify_hard_disk_support((uint8_t *) identify_reply_buffer);
}

static void display_model(uint8_t *hard_disk_response)
{
    printf("Detected hard disk: ");
    int i;

    /* Range in which the the model number of the hard disk is stored. */
    for (i = IDENTIFY_MODEL_NUMBER_START;
        i < IDENTIFY_MODEL_NUMBER_END; i += 2) {
        if (hard_disk_response[i] == 0) {
            break;
        }

        putchar(hard_disk_response[i+1]);
        putchar(hard_disk_response[i]);
    }
    printf("\n");
}

static void display_firmware_revision(uint8_t *hard_disk_response)
{
    printf("Firmeware revision: ");
    int i;

    for (i = IDENTIFY_FIRMWARE_REVISION_START ;
        i < IDENTIFY_FIRMWARE_REVISION_END; ++i) {
        if (hard_disk_response[i] == 0) {
            break;
        }

        putchar(hard_disk_response[i+1]);
        putchar(hard_disk_response[i]);
    }
    printf("\n");
}

static void display_serial_number(uint8_t *hard_disk_response)
{
    printf("Serial number: ");
    int i;

    for (i = IDENTIFY_SERIAL_NUMBER_START;
        i < IDENTIFY_SERIAL_NUMBER_END; ++i) {
        if (hard_disk_response[i] == 0) {
            break;
        }

        if (hard_disk_response[i + 1] != ' ') {
            putchar(hard_disk_response[i+1]);
        }

        if (hard_disk_response[i] != ' ') {
            putchar(hard_disk_response[i]);
        }
    }

    printf("\n");
}

static void display_number_of_lba_entries(uint8_t *hard_disk_response)
{
    printf("Maximum number of 512-byte blocks of LBA Range Entries: ");
    printf("0x%lx\n", *(uint64_t *) (hard_disk_response + (MAXIMUM_LBA_ENTRY)));
}

int verify_hard_disk_support(uint8_t *hard_disk_response)
{
    if (hard_disk_response[54] != 'D' || hard_disk_response[55] != 'W'
        || hard_disk_response[57] != 'C') {
        return -1;
    }

    return 0;
}

int enable_vendor_specific_commands(int hard_disk_file_descriptor)
{
    unsigned char enable_vsc_cdb[SG_ATA_16_LEN];

    enable_vsc_cdb[0]     = SG_ATA_16; /* operation code: SG_ATA_16 */

    /* multiple count: 0 protocol: 4 extended: 0  */
    /* protocol 4: PIO Data-In */
    enable_vsc_cdb[1]     = 0x06;

    /* off.line: cc: lh.en: ll.en: sc.en: f.en: */
    enable_vsc_cdb[2]     = 0x20;
    enable_vsc_cdb[3]     = 0x00; /* Features (8:15): */
    enable_vsc_cdb[4]     = 0x45; /* Features (0:7): */
    enable_vsc_cdb[5]     = 0x00; /* Sector Count (8:15): */
    enable_vsc_cdb[6]     = 0x00; /* Sector Count (0:7): */
    enable_vsc_cdb[7]     = 0x00; /* LBA Low (8:15): */
    enable_vsc_cdb[8]     = 0x00; /* LBA Low (0:7): */
    enable_vsc_cdb[9]     = 0x00; /* LBA Mid (8:15): */
    enable_vsc_cdb[10]    = 0x44; /* LBA Mid (0:7): */
    enable_vsc_cdb[11]    = 0x00; /* LBA High (8:15): */
    enable_vsc_cdb[12]    = 0x57; /* LBA High (0:7): */
    enable_vsc_cdb[13]    = 0xa0; /* Device: */

    /* Command: Vendor Specific Command */
    enable_vsc_cdb[14]    = ATA_VENDOR_SPECIFIC_COMMAND;
    enable_vsc_cdb[15]    = 0x00; /* Control: */

    if (execute_command(enable_vsc_cdb, hard_disk_file_descriptor,
        NULL, 0, SG_DXFER_NONE) == -1) {
        fprintf(stderr, "enable_vendor_specific_commands: Could not send " \
            " enable vcs command to hard disk drive.\n");
        return -1;
    }

    return 0;
}

int disable_vendor_specific_commands(int hard_disk_file_descriptor)
{
    unsigned char disable_vsc_cdb[SG_ATA_16_LEN];

    disable_vsc_cdb[0]     = SG_ATA_16; /* operation code: SG_ATA_16 */

    /* multiple count: 0 protocol: 4 extended: 0  */
    /* protocol 4: PIO Data-In */
    disable_vsc_cdb[1]     = 0x06;

    /* off.line: cc: lh.en: ll.en: sc.en: f.en: */
    disable_vsc_cdb[2]     = 0x20;
    disable_vsc_cdb[3]     = 0x00; /* Features (8:15): */
    disable_vsc_cdb[4]     = 0x44; /* Features (0:7): */
    disable_vsc_cdb[5]     = 0x00; /* Sector Count (8:15): */
    disable_vsc_cdb[6]     = 0x00; /* Sector Count (0:7): */
    disable_vsc_cdb[7]     = 0x00; /* LBA Low (8:15): */
    disable_vsc_cdb[8]     = 0x00; /* LBA Low (0:7): */
    disable_vsc_cdb[9]     = 0x00; /* LBA Mid (8:15): */
    disable_vsc_cdb[10]    = 0x44; /* LBA Mid (0:7): */
    disable_vsc_cdb[11]    = 0x00; /* LBA High (8:15): */
    disable_vsc_cdb[12]    = 0x57; /* LBA High (0:7): */
    disable_vsc_cdb[13]    = 0xa0; /* Device: */

    /* Command: Vendor Specific Command */
    disable_vsc_cdb[14]    = ATA_VENDOR_SPECIFIC_COMMAND;
    disable_vsc_cdb[15]    = 0x00; /* Control: */

    if (execute_command(disable_vsc_cdb, hard_disk_file_descriptor,
        NULL, 0, SG_DXFER_NONE) == -1) {
        fprintf(stderr, "disable_vendor_specific_commands: Could not send " \
            " enable vcs command to hard disk drive.\n");
        return -1;
    }

    return 0;
}

int get_rom_acces(int hard_disk_file_descriptor, int read_write)
{
    if (read_write != ROM_KEY_READ && read_write != ROM_KEY_WRTIE) {
        fprintf(stderr, "get_rom_acces: Invallid read/write direction.\n");
        return -1;
    }

    unsigned char get_rom_access_cdb[SG_ATA_16_LEN];

    get_rom_access_cdb[0]     = SG_ATA_16; /* operation code: SG_ATA_16 */

    /* multiple count: 0 protocol: 4 extended: 0  */
    /* protocol 4: PIO Data-In */
    get_rom_access_cdb[1]     = 0x0a;

    /* off.line: cc: lh.en: ll.en: sc.en: f.en: */
    get_rom_access_cdb[2]     = 0x26;
    get_rom_access_cdb[3]     = 0x00; /* Features (8:15): */
    get_rom_access_cdb[4]     = 0xd6; /* Features (0:7): */
    get_rom_access_cdb[5]     = 0x00; /* Sector Count (8:15): */
    get_rom_access_cdb[6]     = 0x80; /* Sector Count (0:7): */
    get_rom_access_cdb[7]     = 0x00; /* LBA Low (8:15): */
    get_rom_access_cdb[8]     = 0xbf; /* LBA Low (0:7): */
    get_rom_access_cdb[9]     = 0x00; /* LBA Mid (8:15): */
    get_rom_access_cdb[10]    = 0x4f; /* LBA Mid (0:7): */
    get_rom_access_cdb[11]    = 0x00; /* LBA High (8:15): */
    get_rom_access_cdb[12]    = 0xc2; /* LBA High (0:7): */
    get_rom_access_cdb[13]    = 0xa0; /* Device: */

    get_rom_access_cdb[14]    = ATA_OP_SMART; /* Command: smart ata operation */
    get_rom_access_cdb[15]    = 0x00; /* Control: */

    uint8_t command_buffer[512];
    memset(command_buffer, 0, sizeof(command_buffer));

    command_buffer[0] = 0x24; /* Command */
    command_buffer[2] = read_write;

    if (execute_command(get_rom_access_cdb, hard_disk_file_descriptor,
        command_buffer, 512, SG_DXFER_TO_DEV) == -1) {
        fprintf(stderr, "get_rom_acces: Could not send " \
            " smart log enable rom command to hard disk drive.\n");
        return -1;
    }

    return 0;
}

int read_rom_block(int hard_disk_file_descriptor, void *block,
    size_t size)
{
    unsigned char read_rom_block_cdb[SG_ATA_16_LEN];

    read_rom_block_cdb[0]     = SG_ATA_16; /* operation code: SG_ATA_16 */

    /* multiple count: 0 protocol: 4 extended: 0  */
    /* protocol 4: PIO Data-In */
    read_rom_block_cdb[1]     = 0x08;

    /* off.line: cc: lh.en: ll.en: sc.en: f.en: */
    read_rom_block_cdb[2]     = 0x2e;
    read_rom_block_cdb[3]     = 0x00; /* Features (8:15): */
    read_rom_block_cdb[4]     = 0xd5; /* Features (0:7): */
    read_rom_block_cdb[5]     = 0x00; /* Sector Count (8:15): */
    read_rom_block_cdb[6]     = 0x80; /* Sector Count (0:7): */
    read_rom_block_cdb[7]     = 0x00; /* LBA Low (8:15): */
    read_rom_block_cdb[8]     = 0xbf; /* LBA Low (0:7): */
    read_rom_block_cdb[9]     = 0x00; /* LBA Mid (8:15): */
    read_rom_block_cdb[10]    = 0x4f; /* LBA Mid (0:7): */
    read_rom_block_cdb[11]    = 0x00; /* LBA High (8:15): */
    read_rom_block_cdb[12]    = 0xc2; /* LBA High (0:7): */
    read_rom_block_cdb[13]    = 0xa0; /* Device: */
    read_rom_block_cdb[14]    = ATA_OP_SMART; /* Command: smart ata operation */
    read_rom_block_cdb[15]    = 0x00; /* Control: */

    if (execute_command(read_rom_block_cdb, hard_disk_file_descriptor,
        block, size, SG_DXFER_FROM_DEV) == -1) {
        fprintf(stderr, "read_rom_block: Could not send smart log " \
            "read rom command to hard disk drive.\n");
        return -1;
    }

    return 0;
}

int write_rom_block(int hard_disk_file_descriptor, void *block,
    size_t size)
{
    unsigned char write_rom_block_cdb[SG_ATA_16_LEN];

    write_rom_block_cdb[0]     = SG_ATA_16; /* operation code: SG_ATA_16 */

    /* multiple count: 0 protocol: 4 extended: 0  */
    /* protocol 4: PIO Data-In */
    write_rom_block_cdb[1]     = 0x08;

    /* off.line: cc: lh.en: ll.en: sc.en: f.en: */
    write_rom_block_cdb[2]     = 0x2e;
    write_rom_block_cdb[3]     = 0x00; /* Features (8:15): */
    write_rom_block_cdb[4]     = 0xd6; /* Features (0:7): */
    write_rom_block_cdb[5]     = 0x00; /* Sector Count (8:15): */
    write_rom_block_cdb[6]     = 0x80; /* Sector Count (0:7): */
    write_rom_block_cdb[7]     = 0x00; /* LBA Low (8:15): */
    write_rom_block_cdb[8]     = 0xbf; /* LBA Low (0:7): */
    write_rom_block_cdb[9]     = 0x00; /* LBA Mid (8:15): */
    write_rom_block_cdb[10]    = 0x4f; /* LBA Mid (0:7): */
    write_rom_block_cdb[11]    = 0x00; /* LBA High (8:15): */
    write_rom_block_cdb[12]    = 0xc2; /* LBA High (0:7): */
    write_rom_block_cdb[13]    = 0xa0; /* Device: */
    write_rom_block_cdb[14]    = ATA_OP_SMART; /* Command: smart ata operation */
    write_rom_block_cdb[15]    = 0x00; /* Control: */

    if (execute_command(write_rom_block_cdb, hard_disk_file_descriptor,
        block, size, SG_DXFER_TO_DEV) == -1) {
        fprintf(stderr, "write_rom_block: Could not send smart log " \
            "write rom command to hard disk drive.\n");
        return -1;
    }

    return 0;
}

int read_dma_ext(int hard_disk_file_descriptor, unsigned long lba_id,
	uint8_t * data_buffer, size_t size)
{
    unsigned char read_dma_block_cdb[SG_ATA_16_LEN];

    read_dma_block_cdb[0]     = SG_ATA_16; /* operation code: SG_ATA_16 */

    /* multiple count: 0 protocol: 4 extended: 0  */
    /* protocol D: DMA Data-In */
    read_dma_block_cdb[1]     = 0x0D;

    /* off.line: cc: lh.en: ll.en: sc.en: f.en: */
    read_dma_block_cdb[2]     = 0x2e;
    read_dma_block_cdb[3]     = 0x00; /* Features (8:15): */
    read_dma_block_cdb[4]     = 0x00; /* Features (0:7): */
    read_dma_block_cdb[5]     = 0x00; /* Sector Count (8:15): */
    read_dma_block_cdb[6]     = 0x01; /* Sector Count (0:7): */
    read_dma_block_cdb[7]     = lba_id >> 8; /* LBA Low (8:15): */
    read_dma_block_cdb[8]     = lba_id; /* LBA Low (0:7): */
    read_dma_block_cdb[9]     = lba_id >> 24; /* LBA Mid (8:15): */
    read_dma_block_cdb[10]    = lba_id >> 16; /* LBA Mid (0:7): */
    read_dma_block_cdb[11]    = 0x00; /* LBA High (8:15): */
    read_dma_block_cdb[12]    = 0x00; /* LBA High (0:7): */
    read_dma_block_cdb[13]    = 0x40; /* Device: */

    /* Command: smart ata operation */
    read_dma_block_cdb[14]    = ATA_READ_DMA_EXT;
    read_dma_block_cdb[15]    = 0x00; /* Control: */

    if (execute_command(read_dma_block_cdb, hard_disk_file_descriptor,
        data_buffer, size, SG_DXFER_FROM_DEV) == -1) {
        fprintf(stderr, "read_dma_ext: Could not send read dma ext " \
            "command to hard disk drive.\n");
        return -1;
    }

    return 0;
}

/* Does not work as expected. Needs fixing. */
int write_dma_ext(int hard_disk_file_descriptor, unsigned long lba_id,
	uint8_t * data_buffer, size_t size)
{
    unsigned char write_dma_block_cdb[SG_ATA_16_LEN];

    write_dma_block_cdb[0]     = SG_ATA_16; /* operation code: SG_ATA_16 */

    /* multiple count: 0 protocol: 4 extended: 0  */
    /* protocol D: DMA Data-In */
    write_dma_block_cdb[1]     = 0x0D;

    /* off.line: cc: lh.en: ll.en: sc.en: f.en: */
    write_dma_block_cdb[2]     = 0x26;
    write_dma_block_cdb[3]     = 0x00; /* Features (8:15): */
    write_dma_block_cdb[4]     = 0x00; /* Features (0:7): */
    write_dma_block_cdb[5]     = 0x00; /* Sector Count (8:15): */
    write_dma_block_cdb[6]     = 0x01; /* Sector Count (0:7): */
    write_dma_block_cdb[7]     = lba_id >> 8; /* LBA Low (8:15): */
    write_dma_block_cdb[8]     = lba_id; /* LBA Low (0:7): */
    write_dma_block_cdb[9]     = lba_id >> 24; /* LBA Mid (8:15): */
    write_dma_block_cdb[10]    = lba_id >> 16; /* LBA Mid (0:7): */
    write_dma_block_cdb[11]    = 0x00; /* LBA High (8:15): */
    write_dma_block_cdb[12]    = 0x00; /* LBA High (0:7): */
    write_dma_block_cdb[13]    = 0x40; /* Device: */

    /* Command: smart ata operation */
    write_dma_block_cdb[14]    = ATA_WRITE_DMA_EXT;
    write_dma_block_cdb[15]    = 0x00; /* Control: */

    if (execute_command(write_dma_block_cdb, hard_disk_file_descriptor,
        data_buffer, size, SG_DXFER_TO_DEV) == -1) {
        fprintf(stderr, "write_dma_ext: Could not send write dma ext " \
            "command to hard disk drive.\n");
        return -1;
    }

    return 0;
}

static inline int calculate_pack_id(unsigned char *cdb)
{
    uint32_t lba24;
    uint32_t lbah;

    lba24 = (cdb[12] << 16) | (cdb[10] << 8) | (cdb[8]);
    lbah = (cdb[13] & 0x0F);

    return (((uint64_t )lbah) << 24) | (uint64_t) lba24;
}

static inline void display_sense_buffer(unsigned char sense_buffer[32])
{
    int i;
    for (i = 0; i < 32; ++i) {
        fprintf(stderr, "%hx ", (unsigned short) sense_buffer[i]);
    }
    fprintf(stderr, "\n");
}

int execute_command(unsigned char *cdb, int hard_disk_file_descriptor,
    void *response_buffer, size_t response_buffer_size,
    int data_direction)
{
    sg_io_hdr_t io_hdr;
    unsigned char sense_buffer[32];

    memset(&io_hdr, 0, sizeof(sg_io_hdr_t));
    memset(&sense_buffer, 0, sizeof(sense_buffer));

    io_hdr.interface_id = 'S';
    io_hdr.cmd_len = SG_ATA_16_LEN;
    io_hdr.mx_sb_len = sizeof(sense_buffer);
    io_hdr.dxfer_direction = data_direction;
    io_hdr.dxfer_len = response_buffer ? response_buffer_size : 0;
    io_hdr.dxferp = response_buffer;
    io_hdr.cmdp = cdb;
    io_hdr.sbp = sense_buffer;
    io_hdr.timeout = SCSI_DEFAULT_TIMEOUT;

    /* Maybe not necessery:
    http://www.tldp.org/HOWTO/SCSI-Generic-HOWTO/x249.html*/
    io_hdr.pack_id = calculate_pack_id(cdb);

    if (ioctl(hard_disk_file_descriptor, SG_IO, &io_hdr) < 0) {
        perror("ioctl: ");
        display_sense_buffer(sense_buffer);
        return -1;
    }

    if (io_hdr.host_status || io_hdr.driver_status != SG_DRIVER_SENSE ||
        (io_hdr.status && io_hdr.status != SG_CHECK_CONDITION)) {
        fprintf(stderr, "execute_command: Received error response\n");
        display_sense_buffer(sense_buffer);
        return -1;
    }

    /*
    * This is not necessarly a failure and thus returns another warning message.
    */
    if (sense_buffer[0] != 0x72 || sense_buffer[7] < 14 ||
        sense_buffer[8] != 0x09 || sense_buffer[9] < 0x0c) {
        fprintf(stderr, "execute_command: Detected error in sense buffer\n");
        display_sense_buffer(sense_buffer);
        return -2;
    }

    if (sense_buffer[21] & (ATA_STAT_ERR | ATA_STAT_DRQ)) {
        fprintf(stderr, "execute_command: Detected I/O error\n");
        fprintf(stderr, "ata operation: 0x%02x\n", cdb[14]);
        fprintf(stderr, "ata status:    0x%02x\n", sense_buffer[21]);
        fprintf(stderr, "ata error:     0x%02x\n", sense_buffer[11]);
        return -1;
    }

    return 0;
}
