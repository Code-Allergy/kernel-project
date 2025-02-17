#ifndef MMC_H
#define MMC_H
#include <stdint.h>

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

// various bits
#define MADMA_EN			(0x1 << 0)
#define MMC_SOFTRESET			(0x1 << 1)
#define RESETDONE			(0x1 << 0)
#define NOOPENDRAIN			(0x0 << 0)
#define OPENDRAIN			(0x1 << 0)
#define OD				(0x1 << 0)
#define INIT_NOINIT			(0x0 << 1)
#define INIT_INITSTREAM			(0x1 << 1)
#define HR_NOHOSTRESP			(0x0 << 2)
#define STR_BLOCK			(0x0 << 3)
#define MODE_FUNC			(0x0 << 4)
#define DW8_1_4BITMODE			(0x0 << 5)
#define MIT_CTO				(0x0 << 6)
#define CDP_ACTIVEHIGH			(0x0 << 7)
#define WPP_ACTIVEHIGH			(0x0 << 8)
#define RESERVED_MASK			(0x3 << 9)
#define CTPL_MMC_SD			(0x0 << 11)
#define DDR				(0x1 << 19)
#define DMA_MASTER			(0x1 << 20)
#define BLEN_512BYTESLEN		(0x200 << 0)
#define NBLK_STPCNT			(0x0 << 16)
#define DE_ENABLE			(0x1 << 0)
#define BCE_ENABLE			(0x1 << 1)
#define ACEN_ENABLE			(0x1 << 2)
#define DDIR_OFFSET			(4)
#define DDIR_MASK			(0x1 << 4)
#define DDIR_WRITE			(0x0 << 4)
#define DDIR_READ			(0x1 << 4)
#define MSBS_SGLEBLK			(0x0 << 5)
#define MSBS_MULTIBLK			(0x1 << 5)
#define RSP_TYPE_OFFSET			(16)
#define RSP_TYPE_MASK			(0x3 << 16)
#define RSP_TYPE_NORSP			(0x0 << 16)
#define RSP_TYPE_LGHT136		(0x1 << 16)
#define RSP_TYPE_LGHT48			(0x2 << 16)
#define RSP_TYPE_LGHT48B		(0x3 << 16)
#define CCCE_NOCHECK			(0x0 << 19)
#define CCCE_CHECK			(0x1 << 19)
#define CICE_NOCHECK			(0x0 << 20)
#define CICE_CHECK			(0x1 << 20)
#define DP_OFFSET			(21)
#define DP_MASK				(0x1 << 21)
#define DP_NO_DATA			(0x0 << 21)
#define DP_DATA				(0x1 << 21)
#define CMD_TYPE_NORMAL			(0x0 << 22)
#define INDEX_OFFSET			(24)
#define INDEX_MASK			(0x3f << 24)
#define INDEX(i)			(i << 24)
#define DATI_MASK			(0x1 << 1)
#define CMDI_MASK			(0x1 << 0)
#define DTW_1_BITMODE			(0x0 << 1)
#define DTW_4_BITMODE			(0x1 << 1)
#define DTW_8_BITMODE                   (0x1 << 5) /* CON[DW8]*/
#define SDBP_PWROFF			(0x0 << 8)
#define SDBP_PWRON			(0x1 << 8)
#define SDVS_MASK			(0x7 << 9)
#define SDVS_1V8			(0x5 << 9)
#define SDVS_3V0			(0x6 << 9)
#define SDVS_3V3			(0x7 << 9)
#define DMA_SELECT			(0x2 << 3)
#define ICE_MASK			(0x1 << 0)
#define ICE_STOP			(0x0 << 0)
#define ICS_MASK			(0x1 << 1)
#define ICS_NOTREADY			(0x0 << 1)
#define ICE_OSCILLATE			(0x1 << 0)
#define CEN_MASK			(0x1 << 2)
#define CEN_ENABLE			(0x1 << 2)
#define CLKD_OFFSET			(6)
#define CLKD_MASK			(0x3FF << 6)
#define DTO_MASK			(0xF << 16)
#define DTO_15THDTO			(0xE << 16)
#define SOFTRESETALL			(0x1 << 24)
#define CC_MASK				(0x1 << 0)
#define TC_MASK				(0x1 << 1)
#define BWR_MASK			(0x1 << 4)
#define BRR_MASK			(0x1 << 5)
#define ERRI_MASK			(0x1 << 15)
#define IE_CC				(0x01 << 0)
#define IE_TC				(0x01 << 1)
#define IE_BWR				(0x01 << 4)
#define IE_BRR				(0x01 << 5)
#define IE_CTO				(0x01 << 16)
#define IE_CCRC				(0x01 << 17)
#define IE_CEB				(0x01 << 18)
#define IE_CIE				(0x01 << 19)
#define IE_DTO				(0x01 << 20)
#define IE_DCRC				(0x01 << 21)
#define IE_DEB				(0x01 << 22)
#define IE_ADMAE			(0x01 << 25)
#define IE_CERR				(0x01 << 28)
#define IE_BADA				(0x01 << 29)

#define VS33_3V3SUP			(1 << 24)
#define VS30_3V0SUP			(1 << 25)
#define VS18_1V8SUP			(1 << 26)

#define IOV_3V3				3300000
#define IOV_3V0				3000000
#define IOV_1V8				1800000

#define AC12_V1V8_SIGEN		(1 << 19)
#define AC12_SCLK_SEL		(1 << 23)
#define AC12_UHSMC_MASK			(7 << 16)
#define AC12_UHSMC_SDR104		(3 << 16)
#define AC12_UHSMC_SDR12		(0 << 16)
#define AC12_UHSMC_SDR25		(1 << 16)
#define AC12_UHSMC_SDR50		(2 << 16)
#define AC12_UHSMC_SDR104		(3 << 16)
#define AC12_UHSMC_DDR50		(4 << 16)
#define AC12_UHSMC_RES			(0x7 << 16)


#define REG32(addr) (*(volatile uint32_t*)(addr))

#define mmc_reg_out(addr, mask, val)\
	REG32(addr) = (REG32(addr) & (~(mask))) | ((val) & (mask))

// SD Card Commands
#define CMD0    0x00000000  // GO_IDLE_STATE
#define CMD2    0x00000002  // ALL_SEND_CID
#define CMD3    0x00000003  // SEND_RELATIVE_ADDR
#define CMD7    0x00000007  // SELECT/DESELECT_CARD
#define CMD8    0x00000008  // SEND_IF_COND
#define CMD55   0x00000037  // APP_CMD
#define ACMD41  0x00000029  // SD_SEND_OP_COND

/* Structure for SD Card information */
typedef struct _mmcsdCardInfo {
    struct _mmcsdCtrlInfo *ctrl;
	uint32_t cardType;
	uint32_t rca;
	uint32_t raw_scr[2];
	uint32_t raw_csd[4];
	uint32_t raw_cid[4];
	uint32_t ocr;
	uint8_t sd_ver;
	uint8_t busWidth;
	uint8_t tranSpeed;
	uint8_t highCap;
	uint32_t blkLen;
	uint32_t nBlks;
	uint32_t size;

}mmcsdCardInfo;

/* Structure for command */
typedef struct _mmcsdCmd {
	uint32_t idx;
	uint32_t flags;
	uint32_t arg;
	signed char *data;
	uint32_t nblks;
	uint32_t rsp[4];
}mmcsdCmd;

/* Structure for controller information */
typedef struct _mmcsdCtrlInfo {
	uint32_t memBase;
	uint32_t ipClk;
	uint32_t opClk;
	uint32_t (*ctrlInit) (struct _mmcsdCtrlInfo *ctrl);
	uint32_t (*cmdSend) (struct _mmcsdCtrlInfo *ctrl, mmcsdCmd *c);
    void (*busWidthConfig) (struct _mmcsdCtrlInfo *ctrl, uint32_t busWidth);
    int (*busFreqConfig) (struct _mmcsdCtrlInfo *ctrl, uint32_t busFreq);
	uint32_t (*cmdStatusGet) (struct _mmcsdCtrlInfo *ctrl);
	uint32_t (*xferStatusGet) (struct _mmcsdCtrlInfo *ctrl);
	void (*xferSetup) (struct _mmcsdCtrlInfo *ctrl, uint8_t rwFlag,
					       void *ptr, uint32_t blkSize, uint32_t nBlks);
    uint32_t (*cardPresent) (struct _mmcsdCtrlInfo *ctrl);
    void (*intrEnable) (struct _mmcsdCtrlInfo *ctrl);
    uint32_t intrMask;
	uint32_t dmaEnable;
	uint32_t busWidth;
	uint32_t highspeed;
	uint32_t ocr;
        uint32_t cdPinNum;
        uint32_t wpPinNum;
	mmcsdCardInfo *card;
}mmcsdCtrlInfo;

#define MMCSD_IN_FREQ                  96000000 /* 96MHz */
#define MMCSD_INIT_FREQ                400000   /* 400kHz */

#define MMCSD_DMA_CHA_TX               24
#define MMCSD_DMA_CHA_RX               25
#define MMCSD_DMA_QUE_NUM              0
#define MMCSD_DMA_REGION_NUM           0
#define MMCSD_BLK_SIZE                 512
#define MMCSD_OCR                      (SD_OCR_VDD_3P0_3P1 | SD_OCR_VDD_3P1_3P2)


#endif // MMC_H
