#ifndef MMC_H
#define MMC_H

// Control Module Registers for Pin Muxing
#define CONTROL_MODULE_BASE    0x44E10000
#define CONF_MMC0_DAT3        (CONTROL_MODULE_BASE + 0x8F0)
#define CONF_MMC0_DAT2        (CONTROL_MODULE_BASE + 0x8F4)
#define CONF_MMC0_DAT1        (CONTROL_MODULE_BASE + 0x8F8)
#define CONF_MMC0_DAT0        (CONTROL_MODULE_BASE + 0x8FC)
#define CONF_MMC0_CLK         (CONTROL_MODULE_BASE + 0x900)
#define CONF_MMC0_CMD         (CONTROL_MODULE_BASE + 0x904)

// Clock Module Registers
#define CM_PER_BASE           0x44E00000
#define CM_PER_MMC0_CLKCTRL   (CM_PER_BASE + 0x3C)

// MMC Controller Registers
#define MMC0_BASE             0x48060000
#define MMC_SYSCONFIG         0x110
#define MMC_SYSSTATUS         0x114
#define MMC_CON               0x12C
#define MMC_BLK               0x204
#define MMC_ARG               0x208
#define MMC_CMD               0x20C
#define MMC_RSP10             0x210
#define MMC_RSP32             0x214
#define MMC_RSP54             0x218
#define MMC_RSP76             0x21C
#define MMC_DATA              0x220
#define MMC_PSTATE            0x224
#define MMC_HCTL              0x228
#define MMC_SYSCTL            0x22C
#define MMC_STAT              0x230
#define MMC_IE                0x234
#define MMC_ISE               0x238
#define MMC_CAPA              0x240

#define REG32(addr) (*(volatile uint32_t*)(addr))

// SD Card Commands
#define CMD0    0x00000000  // GO_IDLE_STATE
#define CMD2    0x00000002  // ALL_SEND_CID
#define CMD3    0x00000003  // SEND_RELATIVE_ADDR
#define CMD7    0x00000007  // SELECT/DESELECT_CARD
#define CMD8    0x00000008  // SEND_IF_COND
#define CMD55   0x00000037  // APP_CMD
#define ACMD41  0x00000029  // SD_SEND_OP_COND


#endif // MMC_H
