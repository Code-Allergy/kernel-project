#ifndef MMC_H
#define MMC_H
#include <stdint.h>

#define MMC0_BASE 0x01C0F000

typedef volatile struct {
    uint32_t gctrl;       // 0x00: Global Control
    uint32_t clkcr;       // 0x04: Clock Control
    uint32_t timeout;     // 0x08: Timeout
    uint32_t width;       // 0x0C: Bus Width
    uint32_t blksz;       // 0x10: Block Size
    uint32_t bytecnt;     // 0x14: Byte Count
    uint32_t cmd;         // 0x18: Command
    uint32_t arg;         // 0x1C: Argument
    uint32_t resp[4];     // 0x20-0x2C: Response
    uint32_t imask;       // 0x30: Interrupt Mask
    uint32_t mint;        // 0x34: Masked Interrupt Status
    uint32_t rint;        // 0x38: Raw Interrupt Status
    uint32_t status;      // 0x3C: Status
    uint32_t ftrglevel;   // 0x40: FIFO Water Level
    uint32_t funcsel;     // 0x44: Function Select
    uint32_t cbcr;        // 0x48: CIU Byte Count
    uint32_t bbcr;        // 0x4C: BIU Byte Count
    uint32_t dbgc;        // 0x50: Debug Enable
    uint32_t res[2];
    uint32_t dmac;        // 0x5C: DMA Control
    uint32_t dlba;        // 0x60: Descriptor List Base Address
    uint32_t idst;        // 0x64: Internal DMA Status
    uint32_t idie;        // 0x68: Internal DMA Interrupt Enable
    uint32_t chda;        // 0x6C: Current Host Descriptor Address
    uint32_t cbda;        // 0x70: Current Buffer Descriptor Address
    uint32_t res2[99];    // there are other registers here that I have not included yet
    uint32_t fifo;        // 0x200: FIFO
} MMC_Controller;

#define mmc0 ((MMC_Controller *)MMC0_BASE)
#define SMHC_IDIE ((uint32_t*)MMC0_BASE + 0x2C)
#define SMHC_BLKSZ ((uint32_t*) MMC0_BASE + 0x4C)
#define SMHC_DLBA ((uint32_t*) MMC0_BASE + 0x084)


#define SD_RISR_CMD_COMPLETE (1 << 2)
#define SD_RISR_NO_RESPONSE (1 << 1)

#define SD_GCTL_SOFT_RST (1 << 0)

// From QEMU register definitions
#define REG_SD_CMDR       0x18  /* Command */
#define REG_SD_IDST       0x88  /* Internal DMA Controller Status */
#define REG_SD_DLBA       0x84  /* Descriptor List Base Address */

// Command flags (SD_CMDR)
#define SD_CMDR_LOAD      (1 << 31)
#define SD_CMDR_DATA      (1 << 9)
#define SD_CMDR_AUTOSTOP  (1 << 12)
#define SD_CMDR_RESPONSE  (1 << 6)

// DMA Status flags (SD_IDST)
#define SD_IDST_INT_SUMMARY (1 << 8)
#define SD_IDST_RECEIVE_IRQ (1 << 1)

#define SD_GCTL_DMA_ENB (1 << 5)
#define SD_GCTL_DMA_RST (1 << 2)

#define DESC_STATUS_HOLD (1 << 31)
#define DESC_STATUS_ERROR (1 << 30)
#define DESC_STATUS_LAST (1 << 2)

#define DESC_PHYS_ADDR  0x53000000  // TEMP TODO

int mmc_send_cmd(uint32_t cmd, uint32_t arg);
int mmc_read_sector(uint32_t sector, uint8_t *buffer);

#endif // MMC_H
