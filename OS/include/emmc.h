/* Copyright (C) 2013 by John Cronin <jncronin@tysos.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/* Provides an interface to the EMMC controller and commands for interacting
 * with an sd card */

/* References:
 *
 * PLSS 	- SD Group Physical Layer Simplified Specification ver 3.00
 * HCSS		- SD Group Host Controller Simplified Specification ver 3.00
 *
 * Broadcom BCM2835 Peripherals Guide
 */


#include <timer.h>
#include <util.h>
#include <types.h>

extern void set_ptr(u32, u32);
extern u32  get_ptr(u32);
#define mmio_read get_ptr
#define mmio_write set_ptr

#ifdef DEBUG2
#define EMMC_DEBUG
#endif

// Configuration options

// Enable 1.8V support
#define SD_1_8V_SUPPORT

// Enable 4-bit support
#define SD_4BIT_DATA

// SD Clock Frequencies (in Hz)
#define SD_CLOCK_ID         400000
#define SD_CLOCK_NORMAL     25000000
#define SD_CLOCK_HIGH       50000000
#define SD_CLOCK_100        100000000
#define SD_CLOCK_208        208000000

// Enable SDXC maximum performance mode
// Requires 150 mA power so disabled on the RPi for now
//#define SDXC_MAXIMUM_PERFORMANCE

// Enable SDMA support
//#define SDMA_SUPPORT

// SDMA buffer address
#define SDMA_BUFFER     0x6000
#define SDMA_BUFFER_PA  (SDMA_BUFFER + 0xC0000000)

// Enable card interrupts
//#define SD_CARD_INTERRUPTS

// Enable EXPERIMENTAL (and possibly DANGEROUS) SD write support
//#define SD_WRITE_SUPPORT

// Allow old sdhci versions (may cause errors)
//#define EMMC_ALLOW_OLD_SDHCI

// The particular SDHCI implementation
#define SDHCI_IMPLEMENTATION_GENERIC        0
#define SDHCI_IMPLEMENTATION_BCM_2708       1
#define SDHCI_IMPLEMENTATION                SDHCI_IMPLEMENTATION_BCM_2708

static char driver_name[] = "emmc";
static char device_name[] = "emmc0";	// We use a single device name as there is only
// one card slot in the RPi

static u32 hci_ver = 0;
static u32 capabilities_0 = 0;
static u32 capabilities_1 = 0;

struct sd_scr
{
    u32    scr[2];
    u32    sd_bus_widths;
    int         sd_version;
};

struct emmc_block_dev
{
    struct block_device bd;
    u32 card_supports_sdhc;
    u32 card_supports_18v;
    u32 card_ocr;
    u32 card_rca;
    u32 last_interrupt;
    u32 last_error;

    struct sd_scr *scr;

    int failed_voltage_switch;

    u32 last_cmd_reg;
    u32 last_cmd;
    u32 last_cmd_success;
    u32 last_r0;
    u32 last_r1;
    u32 last_r2;
    u32 last_r3;

    void *buf;
    int blocks_to_transfer;
    size_t block_size;
    int use_sdma;
    int card_removal;
    u32 base_clock;
};

#define EMMC_BASE		0x3f300000
#define	EMMC_ARG2		0
#define EMMC_BLKSIZECNT		4
#define EMMC_ARG1		8
#define EMMC_CMDTM		0xC
#define EMMC_RESP0		0x10
#define EMMC_RESP1		0x14
#define EMMC_RESP2		0x18
#define EMMC_RESP3		0x1C
#define EMMC_DATA		0x20
#define EMMC_STATUS		0x24
#define EMMC_CONTROL0		0x28
#define EMMC_CONTROL1		0x2C
#define EMMC_INTERRUPT		0x30
#define EMMC_IRPT_MASK		0x34
#define EMMC_IRPT_EN		0x38
#define EMMC_CONTROL2		0x3C
#define EMMC_CAPABILITIES_0	0x40
#define EMMC_CAPABILITIES_1	0x44
#define EMMC_FORCE_IRPT		0x50
#define EMMC_BOOT_TIMEOUT	0x70
#define EMMC_DBG_SEL		0x74
#define EMMC_EXRDFIFO_CFG	0x80
#define EMMC_EXRDFIFO_EN	0x84
#define EMMC_TUNE_STEP		0x88
#define EMMC_TUNE_STEPS_STD	0x8C
#define EMMC_TUNE_STEPS_DDR	0x90
#define EMMC_SPI_INT_SPT	0xF0
#define EMMC_SLOTISR_VER	0xFC

#define SD_CMD_INDEX(a)		((a) << 24)
#define SD_CMD_TYPE_NORMAL	0x0
#define SD_CMD_TYPE_SUSPEND	(1 << 22)
#define SD_CMD_TYPE_RESUME	(2 << 22)
#define SD_CMD_TYPE_ABORT	(3 << 22)
#define SD_CMD_TYPE_MASK    (3 << 22)
#define SD_CMD_ISDATA		(1 << 21)
#define SD_CMD_IXCHK_EN		(1 << 20)
#define SD_CMD_CRCCHK_EN	(1 << 19)
#define SD_CMD_RSPNS_TYPE_NONE	0			// For no response
#define SD_CMD_RSPNS_TYPE_136	(1 << 16)		// For response R2 (with CRC), R3,4 (no CRC)
#define SD_CMD_RSPNS_TYPE_48	(2 << 16)		// For responses R1, R5, R6, R7 (with CRC)
#define SD_CMD_RSPNS_TYPE_48B	(3 << 16)		// For responses R1b, R5b (with CRC)
#define SD_CMD_RSPNS_TYPE_MASK  (3 << 16)
#define SD_CMD_MULTI_BLOCK	(1 << 5)
#define SD_CMD_DAT_DIR_HC	0
#define SD_CMD_DAT_DIR_CH	(1 << 4)
#define SD_CMD_AUTO_CMD_EN_NONE	0
#define SD_CMD_AUTO_CMD_EN_CMD12	(1 << 2)
#define SD_CMD_AUTO_CMD_EN_CMD23	(2 << 2)
#define SD_CMD_BLKCNT_EN		(1 << 1)
#define SD_CMD_DMA          1

#define SD_ERR_CMD_TIMEOUT	0
#define SD_ERR_CMD_CRC		1
#define SD_ERR_CMD_END_BIT	2
#define SD_ERR_CMD_INDEX	3
#define SD_ERR_DATA_TIMEOUT	4
#define SD_ERR_DATA_CRC		5
#define SD_ERR_DATA_END_BIT	6
#define SD_ERR_CURRENT_LIMIT	7
#define SD_ERR_AUTO_CMD12	8
#define SD_ERR_ADMA		9
#define SD_ERR_TUNING		10
#define SD_ERR_RSVD		11

#define SD_ERR_MASK_CMD_TIMEOUT		(1 << (16 + SD_ERR_CMD_TIMEOUT))
#define SD_ERR_MASK_CMD_CRC		(1 << (16 + SD_ERR_CMD_CRC))
#define SD_ERR_MASK_CMD_END_BIT		(1 << (16 + SD_ERR_CMD_END_BIT))
#define SD_ERR_MASK_CMD_INDEX		(1 << (16 + SD_ERR_CMD_INDEX))
#define SD_ERR_MASK_DATA_TIMEOUT	(1 << (16 + SD_ERR_CMD_TIMEOUT))
#define SD_ERR_MASK_DATA_CRC		(1 << (16 + SD_ERR_CMD_CRC))
#define SD_ERR_MASK_DATA_END_BIT	(1 << (16 + SD_ERR_CMD_END_BIT))
#define SD_ERR_MASK_CURRENT_LIMIT	(1 << (16 + SD_ERR_CMD_CURRENT_LIMIT))
#define SD_ERR_MASK_AUTO_CMD12		(1 << (16 + SD_ERR_CMD_AUTO_CMD12))
#define SD_ERR_MASK_ADMA		(1 << (16 + SD_ERR_CMD_ADMA))
#define SD_ERR_MASK_TUNING		(1 << (16 + SD_ERR_CMD_TUNING))

#define SD_COMMAND_COMPLETE     1
#define SD_TRANSFER_COMPLETE    (1 << 1)
#define SD_BLOCK_GAP_EVENT      (1 << 2)
#define SD_DMA_INTERRUPT        (1 << 3)
#define SD_BUFFER_WRITE_READY   (1 << 4)
#define SD_BUFFER_READ_READY    (1 << 5)
#define SD_CARD_INSERTION       (1 << 6)
#define SD_CARD_REMOVAL         (1 << 7)
#define SD_CARD_INTERRUPT       (1 << 8)

#define SD_RESP_NONE        SD_CMD_RSPNS_TYPE_NONE
#define SD_RESP_R1          (SD_CMD_RSPNS_TYPE_48 | SD_CMD_CRCCHK_EN)
#define SD_RESP_R1b         (SD_CMD_RSPNS_TYPE_48B | SD_CMD_CRCCHK_EN)
#define SD_RESP_R2          (SD_CMD_RSPNS_TYPE_136 | SD_CMD_CRCCHK_EN)
#define SD_RESP_R3          SD_CMD_RSPNS_TYPE_48
#define SD_RESP_R4          SD_CMD_RSPNS_TYPE_136
#define SD_RESP_R5          (SD_CMD_RSPNS_TYPE_48 | SD_CMD_CRCCHK_EN)
#define SD_RESP_R5b         (SD_CMD_RSPNS_TYPE_48B | SD_CMD_CRCCHK_EN)
#define SD_RESP_R6          (SD_CMD_RSPNS_TYPE_48 | SD_CMD_CRCCHK_EN)
#define SD_RESP_R7          (SD_CMD_RSPNS_TYPE_48 | SD_CMD_CRCCHK_EN)

#define SD_DATA_READ        (SD_CMD_ISDATA | SD_CMD_DAT_DIR_CH)
#define SD_DATA_WRITE       (SD_CMD_ISDATA | SD_CMD_DAT_DIR_HC)

#define SD_CMD_RESERVED(a)  0xffffffff

#define SUCCESS(a)          (a->last_cmd_success)
#define FAIL(a)             (a->last_cmd_success == 0)
#define TIMEOUT(a)          (FAIL(a) && (a->last_error == 0))
#define CMD_TIMEOUT(a)      (FAIL(a) && (a->last_error & (1 << 16)))
#define CMD_CRC(a)          (FAIL(a) && (a->last_error & (1 << 17)))
#define CMD_END_BIT(a)      (FAIL(a) && (a->last_error & (1 << 18)))
#define CMD_INDEX(a)        (FAIL(a) && (a->last_error & (1 << 19)))
#define DATA_TIMEOUT(a)     (FAIL(a) && (a->last_error & (1 << 20)))
#define DATA_CRC(a)         (FAIL(a) && (a->last_error & (1 << 21)))
#define DATA_END_BIT(a)     (FAIL(a) && (a->last_error & (1 << 22)))
#define CURRENT_LIMIT(a)    (FAIL(a) && (a->last_error & (1 << 23)))
#define ACMD12_ERROR(a)     (FAIL(a) && (a->last_error & (1 << 24)))
#define ADMA_ERROR(a)       (FAIL(a) && (a->last_error & (1 << 25)))
#define TUNING_ERROR(a)     (FAIL(a) && (a->last_error & (1 << 26)))

#define SD_VER_UNKNOWN      0
#define SD_VER_1            1
#define SD_VER_1_1          2
#define SD_VER_2            3
#define SD_VER_3            4
#define SD_VER_4            5

static u32 emmc_base = EMMC_BASE;

void emmc_set_base(u32 base)
{
    emmc_base = base;
}

static char *sd_versions[] = { "unknown", "1.0 and 1.01", "1.10",
                               "2.00", "3.0x", "4.xx" };

#ifdef EMMC_DEBUG
static char *err_irpts[] = { "CMD_TIMEOUT", "CMD_CRC", "CMD_END_BIT", "CMD_INDEX",
	"DATA_TIMEOUT", "DATA_CRC", "DATA_END_BIT", "CURRENT_LIMIT",
	"AUTO_CMD12", "ADMA", "TUNING", "RSVD" };
#endif

int sd_read(struct block_device *, u8 *, size_t buf_size, u32);
int sd_write(struct block_device *, u8 *, size_t buf_size, u32);

static u32 sd_commands[] = {
        SD_CMD_INDEX(0),
        SD_CMD_RESERVED(1),
        SD_CMD_INDEX(2) | SD_RESP_R2,
        SD_CMD_INDEX(3) | SD_RESP_R6,
        SD_CMD_INDEX(4),
        SD_CMD_INDEX(5) | SD_RESP_R4,
        SD_CMD_INDEX(6) | SD_RESP_R1,
        SD_CMD_INDEX(7) | SD_RESP_R1b,
        SD_CMD_INDEX(8) | SD_RESP_R7,
        SD_CMD_INDEX(9) | SD_RESP_R2,
        SD_CMD_INDEX(10) | SD_RESP_R2,
        SD_CMD_INDEX(11) | SD_RESP_R1,
        SD_CMD_INDEX(12) | SD_RESP_R1b | SD_CMD_TYPE_ABORT,
        SD_CMD_INDEX(13) | SD_RESP_R1,
        SD_CMD_RESERVED(14),
        SD_CMD_INDEX(15),
        SD_CMD_INDEX(16) | SD_RESP_R1,
        SD_CMD_INDEX(17) | SD_RESP_R1 | SD_DATA_READ,
        SD_CMD_INDEX(18) | SD_RESP_R1 | SD_DATA_READ | SD_CMD_MULTI_BLOCK | SD_CMD_BLKCNT_EN,
        SD_CMD_INDEX(19) | SD_RESP_R1 | SD_DATA_READ,
        SD_CMD_INDEX(20) | SD_RESP_R1b,
        SD_CMD_RESERVED(21),
        SD_CMD_RESERVED(22),
        SD_CMD_INDEX(23) | SD_RESP_R1,
        SD_CMD_INDEX(24) | SD_RESP_R1 | SD_DATA_WRITE,
        SD_CMD_INDEX(25) | SD_RESP_R1 | SD_DATA_WRITE | SD_CMD_MULTI_BLOCK | SD_CMD_BLKCNT_EN,
        SD_CMD_RESERVED(26),
        SD_CMD_INDEX(27) | SD_RESP_R1 | SD_DATA_WRITE,
        SD_CMD_INDEX(28) | SD_RESP_R1b,
        SD_CMD_INDEX(29) | SD_RESP_R1b,
        SD_CMD_INDEX(30) | SD_RESP_R1 | SD_DATA_READ,
        SD_CMD_RESERVED(31),
        SD_CMD_INDEX(32) | SD_RESP_R1,
        SD_CMD_INDEX(33) | SD_RESP_R1,
        SD_CMD_RESERVED(34),
        SD_CMD_RESERVED(35),
        SD_CMD_RESERVED(36),
        SD_CMD_RESERVED(37),
        SD_CMD_INDEX(38) | SD_RESP_R1b,
        SD_CMD_RESERVED(39),
        SD_CMD_RESERVED(40),
        SD_CMD_RESERVED(41),
        SD_CMD_RESERVED(42) | SD_RESP_R1,
        SD_CMD_RESERVED(43),
        SD_CMD_RESERVED(44),
        SD_CMD_RESERVED(45),
        SD_CMD_RESERVED(46),
        SD_CMD_RESERVED(47),
        SD_CMD_RESERVED(48),
        SD_CMD_RESERVED(49),
        SD_CMD_RESERVED(50),
        SD_CMD_RESERVED(51),
        SD_CMD_RESERVED(52),
        SD_CMD_RESERVED(53),
        SD_CMD_RESERVED(54),
        SD_CMD_INDEX(55) | SD_RESP_R1,
        SD_CMD_INDEX(56) | SD_RESP_R1 | SD_CMD_ISDATA,
        SD_CMD_RESERVED(57),
        SD_CMD_RESERVED(58),
        SD_CMD_RESERVED(59),
        SD_CMD_RESERVED(60),
        SD_CMD_RESERVED(61),
        SD_CMD_RESERVED(62),
        SD_CMD_RESERVED(63)
};

static u32 sd_acommands[] = {
        SD_CMD_RESERVED(0),
        SD_CMD_RESERVED(1),
        SD_CMD_RESERVED(2),
        SD_CMD_RESERVED(3),
        SD_CMD_RESERVED(4),
        SD_CMD_RESERVED(5),
        SD_CMD_INDEX(6) | SD_RESP_R1,
        SD_CMD_RESERVED(7),
        SD_CMD_RESERVED(8),
        SD_CMD_RESERVED(9),
        SD_CMD_RESERVED(10),
        SD_CMD_RESERVED(11),
        SD_CMD_RESERVED(12),
        SD_CMD_INDEX(13) | SD_RESP_R1,
        SD_CMD_RESERVED(14),
        SD_CMD_RESERVED(15),
        SD_CMD_RESERVED(16),
        SD_CMD_RESERVED(17),
        SD_CMD_RESERVED(18),
        SD_CMD_RESERVED(19),
        SD_CMD_RESERVED(20),
        SD_CMD_RESERVED(21),
        SD_CMD_INDEX(22) | SD_RESP_R1 | SD_DATA_READ,
        SD_CMD_INDEX(23) | SD_RESP_R1,
        SD_CMD_RESERVED(24),
        SD_CMD_RESERVED(25),
        SD_CMD_RESERVED(26),
        SD_CMD_RESERVED(27),
        SD_CMD_RESERVED(28),
        SD_CMD_RESERVED(29),
        SD_CMD_RESERVED(30),
        SD_CMD_RESERVED(31),
        SD_CMD_RESERVED(32),
        SD_CMD_RESERVED(33),
        SD_CMD_RESERVED(34),
        SD_CMD_RESERVED(35),
        SD_CMD_RESERVED(36),
        SD_CMD_RESERVED(37),
        SD_CMD_RESERVED(38),
        SD_CMD_RESERVED(39),
        SD_CMD_RESERVED(40),
        SD_CMD_INDEX(41) | SD_RESP_R3,
        SD_CMD_INDEX(42) | SD_RESP_R1,
        SD_CMD_RESERVED(43),
        SD_CMD_RESERVED(44),
        SD_CMD_RESERVED(45),
        SD_CMD_RESERVED(46),
        SD_CMD_RESERVED(47),
        SD_CMD_RESERVED(48),
        SD_CMD_RESERVED(49),
        SD_CMD_RESERVED(50),
        SD_CMD_INDEX(51) | SD_RESP_R1 | SD_DATA_READ,
        SD_CMD_RESERVED(52),
        SD_CMD_RESERVED(53),
        SD_CMD_RESERVED(54),
        SD_CMD_RESERVED(55),
        SD_CMD_RESERVED(56),
        SD_CMD_RESERVED(57),
        SD_CMD_RESERVED(58),
        SD_CMD_RESERVED(59),
        SD_CMD_RESERVED(60),
        SD_CMD_RESERVED(61),
        SD_CMD_RESERVED(62),
        SD_CMD_RESERVED(63)
};

// The actual command indices
#define GO_IDLE_STATE           0
#define ALL_SEND_CID            2
#define SEND_RELATIVE_ADDR      3
#define SET_DSR                 4
#define IO_SET_OP_COND          5
#define SWITCH_FUNC             6
#define SELECT_CARD             7
#define DESELECT_CARD           7
#define SELECT_DESELECT_CARD    7
#define SEND_IF_COND            8
#define SEND_CSD                9
#define SEND_CID                10
#define VOLTAGE_SWITCH          11
#define STOP_TRANSMISSION       12
#define SEND_STATUS             13
#define GO_INACTIVE_STATE       15
#define SET_BLOCKLEN            16
#define READ_SINGLE_BLOCK       17
#define READ_MULTIPLE_BLOCK     18
#define SEND_TUNING_BLOCK       19
#define SPEED_CLASS_CONTROL     20
#define SET_BLOCK_COUNT         23
#define WRITE_BLOCK             24
#define WRITE_MULTIPLE_BLOCK    25
#define PROGRAM_CSD             27
#define SET_WRITE_PROT          28
#define CLR_WRITE_PROT          29
#define SEND_WRITE_PROT         30
#define ERASE_WR_BLK_START      32
#define ERASE_WR_BLK_END        33
#define ERASE                   38
#define LOCK_UNLOCK             42
#define APP_CMD                 55
#define GEN_CMD                 56

#define IS_APP_CMD              0x80000000
#define ACMD(a)                 (a | IS_APP_CMD)
#define SET_BUS_WIDTH           (6 | IS_APP_CMD)
#define SD_STATUS               (13 | IS_APP_CMD)
#define SEND_NUM_WR_BLOCKS      (22 | IS_APP_CMD)
#define SET_WR_BLK_ERASE_COUNT  (23 | IS_APP_CMD)
#define SD_SEND_OP_COND         (41 | IS_APP_CMD)
#define SET_CLR_CARD_DETECT     (42 | IS_APP_CMD)
#define SEND_SCR                (51 | IS_APP_CMD)

#define SD_RESET_CMD            (1 << 25)
#define SD_RESET_DAT            (1 << 26)
#define SD_RESET_ALL            (1 << 24)

#define SD_GET_CLOCK_DIVIDER_FAIL	0xffffffff

// Get the current base clock rate in Hz
#if SDHCI_IMPLEMENTATION == SDHCI_IMPLEMENTATION_BCM_2708
#include "mbox.h"
#endif

