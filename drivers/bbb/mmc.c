#include <kernel/mmc.h>
#include <kernel/printk.h>
#include <stdint.h>
#include "mmc.h"
#include "kernel/sd.h"
#include "hw_control.h"
#include "hs_mmcsd.h"
#include "kernel/fat32.h"

mmcsdCtrlInfo ctrlInfo;

// TODO move
#define HS_MMCSD_DATALINE_RESET         (MMCHS_SYSCTL_SRD)
#define HS_MMCSD_CMDLINE_RESET          (MMCHS_SYSCTL_SRC)
#define HS_MMCSD_ALL_RESET              (MMCHS_SYSCTL_SRA)
#define HS_MMCSD_SUPPORT_VOLT_1P8       (MMCHS_CAPA_VS18)
#define HS_MMCSD_SUPPORT_VOLT_3P0       (MMCHS_CAPA_VS30)
#define HS_MMCSD_SUPPORT_VOLT_3P3       (MMCHS_CAPA_VS33)
#define HS_MMCSD_SUPPORT_DMA            (MMCHS_CAPA_DS)
#define HS_MMCSD_SUPPORT_HIGHSPEED      (MMCHS_CAPA_HSS)
#define HS_MMCSD_SUPPORT_BLOCKLEN       (MMCHS_CAPA_MBL)
#define HS_MMCSD_BUS_VOLT_1P8          (MMCHS_HCTL_SDVS_1V8 << MMCHS_HCTL_SDVS_SHIFT)
#define HS_MMCSD_BUS_VOLT_3P0          (MMCHS_HCTL_SDVS_3V0 << MMCHS_HCTL_SDVS_SHIFT)
#define HS_MMCSD_BUS_VOLT_3P3          (MMCHS_HCTL_SDVS_3V3 << MMCHS_HCTL_SDVS_SHIFT)
#define HS_MMCSD_BUS_POWER_ON          (MMCHS_HCTL_SDBP_PWRON << MMCHS_HCTL_SDBP_SHIFT)
#define HS_MMCSD_BUS_POWER_OFF         (MMCHS_HCTL_SDBP_PWROFF << MMCHS_HCTL_SDBP_SHIFT)
#define HS_MMCSD_BUS_HIGHSPEED         (MMCHS_HCTL_HSPE_HIGHSPEED << MMCHS_HCTL_HSPE_SHIFT)
#define HS_MMCSD_BUS_STDSPEED          (MMCHS_HCTL_HSPE_NORMALSPEED << MMCHS_HCTL_HSPE_SHIFT)

#define HS_MMCSD_AUTOIDLE_ENABLE     (MMCHS_SYSCONFIG_AUTOIDLE_ON << MMCHS_SYSCONFIG_AUTOIDLE_SHIFT)
#define HS_MMCSD_AUTOIDLE_DISABLE    (MMCHS_SYSCONFIG_AUTOIDLE_OFF << MMCHS_SYSCONFIG_AUTOIDLE_SHIFT)

#define HS_MMCSD_DATA_TIMEOUT(n)        ((((n) - 13) & 0xF) << MMCHS_SYSCTL_DTO_SHIFT)
#define HS_MMCSD_CLK_DIVIDER(n)         ((n & 0x3FF) << MMCHS_SYSCTL_CLKD_SHIFT)
#define HS_MMCSD_CARDCLOCK_ENABLE       (MMCHS_SYSCTL_CEN_ENABLE << MMCHS_SYSCTL_CEN_SHIFT)
#define HS_MMCSD_CARDCLOCK_DISABLE      (MMCHS_SYSCTL_CEN_DISABLE << MMCHS_SYSCTL_CEN_SHIFT)
#define HS_MMCSD_INTCLOCK_ON            (MMCHS_SYSCTL_ICE_OSCILLATE << MMCHS_SYSCTL_ICE_SHIFT)
#define HS_MMCSD_INTCLOCK_OFF           (MMCHS_SYSCTL_ICE_STOP << MMCHS_SYSCTL_ICE_SHIFT)

#define HS_MMCSD_INTR_BADACCESS         (MMCHS_IE_BADA_ENABLE)
#define HS_MMCSD_INTR_CARDERROR         (MMCHS_IE_CERR_ENABLE)
#define HS_MMCSD_INTR_ADMAERROR         (MMCHS_IE_ADMAE_ENABLE)
#define HS_MMCSD_INTR_ACMD12ERR         (MMCHS_IE_ACE_ENABLE)
#define HS_MMCSD_INTR_DATABITERR        (MMCHS_IE_DEB_ENABLE)
#define HS_MMCSD_INTR_DATACRCERR        (MMCHS_IE_DCRC_ENABLE)
#define HS_MMCSD_INTR_DATATIMEOUT       (MMCHS_IE_DTO_ENABLE)
#define HS_MMCSD_INTR_CMDINDXERR        (MMCHS_IE_CIE_ENABLE)
#define HS_MMCSD_INTR_CMDBITERR         (MMCHS_IE_CEB_ENABLE)
#define HS_MMCSD_INTR_CMDCRCERR         (MMCHS_IE_CCRC_ENABLE)
#define HS_MMCSD_INTR_CMDTIMEOUT        (MMCHS_IE_CTO_ENABLE)
#define HS_MMCSD_INTR_CARDINS           (MMCHS_IE_CINS_ENABLE)
#define HS_MMCSD_INTR_BUFRDRDY          (MMCHS_IE_BRR_ENABLE)
#define HS_MMCSD_INTR_BUFWRRDY          (MMCHS_IE_BWR_ENABLE)
#define HS_MMCSD_INTR_TRNFCOMP          (MMCHS_IE_TC_ENABLE)
#define HS_MMCSD_INTR_CMDCOMP           (MMCHS_IE_CC_ENABLE)

#define HS_MMCSD_STAT_BADACCESS         (MMCHS_STAT_BADA)
#define HS_MMCSD_STAT_CARDERROR         (MMCHS_STAT_CERR)
#define HS_MMCSD_STAT_ADMAERROR         (MMCHS_STAT_ADMAE)
#define HS_MMCSD_STAT_ACMD12ERR         (MMCHS_STAT_ACE)
#define HS_MMCSD_STAT_DATABITERR        (MMCHS_STAT_DEB)
#define HS_MMCSD_STAT_DATACRCERR        (MMCHS_STAT_DCRC)
#define HS_MMCSD_STAT_ERR				(MMCHS_STAT_ERRI)
#define HS_MMCSD_STAT_DATATIMEOUT       (MMCHS_STAT_DTO)
#define HS_MMCSD_STAT_CMDINDXERR        (MMCHS_STAT_CIE)
#define HS_MMCSD_STAT_CMDBITERR         (MMCHS_STAT_CEB)
#define HS_MMCSD_STAT_CMDCRCERR         (MMCHS_STAT_CCRC)
#define HS_MMCSD_STAT_CMDTIMEOUT        (MMCHS_STAT_CTO)
#define HS_MMCSD_STAT_CARDINS           (MMCHS_STAT_CINS)
#define HS_MMCSD_STAT_BUFRDRDY          (MMCHS_STAT_BRR)
#define HS_MMCSD_STAT_BUFWRRDY          (MMCHS_STAT_BWR)
#define HS_MMCSD_STAT_TRNFCOMP          (MMCHS_STAT_TC)
#define HS_MMCSD_STAT_CMDCOMP           (MMCHS_STAT_CC)

#define HS_MMCSD_SIGEN_BADACCESS        (MMCHS_ISE_BADA_SIGEN)
#define HS_MMCSD_SIGEN_CARDERROR        (MMCHS_ISE_CERR_SIGEN)
#define HS_MMCSD_SIGEN_ADMAERROR        (MMCHS_ISE_ADMAE_SIGEN)
#define HS_MMCSD_SIGEN_ACMD12ERR        (MMCHS_ISE_ACE_SIGEN)
#define HS_MMCSD_SIGEN_DATABITERR       (MMCHS_ISE_DEB_SIGEN)
#define HS_MMCSD_SIGEN_DATACRCERR       (MMCHS_ISE_DCRC_SIGEN)
#define HS_MMCSD_SIGEN_DATATIMEOUT      (MMCHS_ISE_DTO_SIGEN)
#define HS_MMCSD_SIGEN_CMDINDXERR       (MMCHS_ISE_CIE_SIGEN)
#define HS_MMCSD_SIGEN_CMDBITERR        (MMCHS_ISE_CEB_SIGEN)
#define HS_MMCSD_SIGEN_CMDCRCERR        (MMCHS_ISE_CCRC_SIGEN)
#define HS_MMCSD_SIGEN_CMDTIMEOUT       (MMCHS_ISE_CTO_SIGEN)
#define HS_MMCSD_SIGEN_CARDINS          (MMCHS_ISE_CINS_SIGEN)
#define HS_MMCSD_SIGEN_BUFRDRDY         (MMCHS_ISE_BRR_SIGEN)
#define HS_MMCSD_SIGEN_BUFWRRDY         (MMCHS_ISE_BWR_SIGEN)
#define HS_MMCSD_SIGEN_TRNFCOMP         (MMCHS_ISE_TC_SIGEN)
#define HS_MMCSD_SIGEN_CMDCOMP          (MMCHS_ISE_CC_SIGEN)

static void mmc_init(void) {
    printk("Skipping setting clocks on BBB for now!\n");
}

// void set_init_clock(void) {
//     uint32_t target_freq_khz = 40;  // 80 kHz ensures 80 clocks = 1ms

//     uint32_t sysctl = MMC0_BASE + MMC_SYSCTL;
//     uint32_t val = REG32(sysctl);

//     // Disable clock
//     val &= ~(1 << 2);
//     REG32(sysctl) = val;

//     // Set divider (96MHz / 80kHz / 2) - 1 = 599
//     uint32_t div = (48000 / (2 * target_freq_khz)) - 1;

//     // Set divider and enable internal clock
//     val &= ~(0x3FF << 6);  // Clear existing divider bits
//     val |= (div << 6);     // Set new divider
//     val |= (1 << 0);       // Enable internal clock (ICE)
//     REG32(sysctl) = val;

//     // Wait for clock to stabilize
//     uint32_t timeout = 100000;
//     while (!(REG32(sysctl) & (1 << 1))) {
//         if (--timeout == 0) {
//             printk("Timeout waiting for clock stabilization\n");
//             return;
//         }
//     }

//     // Enable clock to card
//     val |= (1 << 2);
//     REG32(sysctl) = val;

//     printk("Init clock set to %d kHz (div=%d)\n", target_freq_khz, div);
// }

// Function to configure pin muxing
void configure_mmc_pins(void) {
    REG32(CONTROL_MODULE_BASE + CONTROL_CONF_MMC0_DAT3) =
                   (0 << CONTROL_CONF_MMC0_DAT3_CONF_MMC0_DAT3_MMODE_SHIFT)    |
                   (0 << CONTROL_CONF_MMC0_DAT3_CONF_MMC0_DAT3_PUDEN_SHIFT)    |
                   (1 << CONTROL_CONF_MMC0_DAT3_CONF_MMC0_DAT3_PUTYPESEL_SHIFT)|
                   (1 << CONTROL_CONF_MMC0_DAT3_CONF_MMC0_DAT3_RXACTIVE_SHIFT);

    REG32(CONTROL_MODULE_BASE + CONTROL_CONF_MMC0_DAT2) =
                   (0 << CONTROL_CONF_MMC0_DAT2_CONF_MMC0_DAT2_MMODE_SHIFT)    |
                   (0 << CONTROL_CONF_MMC0_DAT2_CONF_MMC0_DAT2_PUDEN_SHIFT)    |
                   (1 << CONTROL_CONF_MMC0_DAT2_CONF_MMC0_DAT2_PUTYPESEL_SHIFT)|
                   (1 << CONTROL_CONF_MMC0_DAT2_CONF_MMC0_DAT2_RXACTIVE_SHIFT);

    REG32(CONTROL_MODULE_BASE + CONTROL_CONF_MMC0_DAT1) =
                   (0 << CONTROL_CONF_MMC0_DAT1_CONF_MMC0_DAT1_MMODE_SHIFT)    |
                   (0 << CONTROL_CONF_MMC0_DAT1_CONF_MMC0_DAT1_PUDEN_SHIFT)    |
                   (1 << CONTROL_CONF_MMC0_DAT1_CONF_MMC0_DAT1_PUTYPESEL_SHIFT)|
                   (1 << CONTROL_CONF_MMC0_DAT1_CONF_MMC0_DAT1_RXACTIVE_SHIFT);

    REG32(CONTROL_MODULE_BASE + CONTROL_CONF_MMC0_DAT0) =
                   (0 << CONTROL_CONF_MMC0_DAT0_CONF_MMC0_DAT0_MMODE_SHIFT)    |
                   (0 << CONTROL_CONF_MMC0_DAT0_CONF_MMC0_DAT0_PUDEN_SHIFT)    |
                   (1 << CONTROL_CONF_MMC0_DAT0_CONF_MMC0_DAT0_PUTYPESEL_SHIFT)|
                   (1 << CONTROL_CONF_MMC0_DAT0_CONF_MMC0_DAT0_RXACTIVE_SHIFT);

    REG32(CONTROL_MODULE_BASE + CONTROL_CONF_MMC0_CLK) =
                   (0 << CONTROL_CONF_MMC0_CLK_CONF_MMC0_CLK_MMODE_SHIFT)    |
                   (0 << CONTROL_CONF_MMC0_CLK_CONF_MMC0_CLK_PUDEN_SHIFT)    |
                   (1 << CONTROL_CONF_MMC0_CLK_CONF_MMC0_CLK_PUTYPESEL_SHIFT)|
                   (1 << CONTROL_CONF_MMC0_CLK_CONF_MMC0_CLK_RXACTIVE_SHIFT);

    REG32(CONTROL_MODULE_BASE + CONTROL_CONF_MMC0_CMD) =
                   (0 << CONTROL_CONF_MMC0_CMD_CONF_MMC0_CMD_MMODE_SHIFT)    |
                   (0 << CONTROL_CONF_MMC0_CMD_CONF_MMC0_CMD_PUDEN_SHIFT)    |
                   (1 << CONTROL_CONF_MMC0_CMD_CONF_MMC0_CMD_PUTYPESEL_SHIFT)|
                   (1 << CONTROL_CONF_MMC0_CMD_CONF_MMC0_CMD_RXACTIVE_SHIFT);

     REG32(CONTROL_MODULE_BASE + CONTROL_CONF_SPI0_CS1) =
                   (5 << CONTROL_CONF_SPI0_CS1_CONF_SPI0_CS1_MMODE_SHIFT)    |
                   (0 << CONTROL_CONF_SPI0_CS1_CONF_SPI0_CS1_PUDEN_SHIFT)    |
                   (1 << CONTROL_CONF_SPI0_CS1_CONF_SPI0_CS1_PUTYPESEL_SHIFT)|
                   (1 << CONTROL_CONF_SPI0_CS1_CONF_SPI0_CS1_RXACTIVE_SHIFT);
}

// Function to set MMC clock frequency
void set_mmc_clock(uint32_t target_freq_khz) {
    printk("About to set clock\n");
    uint32_t sysctl = MMC0_BASE + MMC_SYSCTL;
    uint32_t val = REG32(sysctl);

    // Disable clock
    val &= ~(1 << 2);
    REG32(sysctl) = val;

    // Set divider (assuming 96MHz source clock)
    uint32_t div = (96000 / (2 * target_freq_khz)) - 1;  // 96 MHz = 96000 KHz
    if (div > 0x3FF) div = 0x3FF;  // Clamp to 10-bit max
    if (div < 1) div = 1;  // Ensure divider is at least 1

    // Set divider and enable internal clock
    val &= ~(0x3FF << 6);  // Clear existing divider bits
    val |= (div << 6);      // Set new divider
    val |= (1 << 0);        // Enable internal clock (ICE)
    REG32(sysctl) = val;

    // Wait for clock to stabilize
    while (!(REG32(sysctl) & (1 << 1)));  // Wait for bit 1 (ICS)

    // Enable clock to card
    val |= (1 << 2);
    REG32(sysctl) = val;
}

// Function to send MMC command
uint32_t mmc_send_cmd(uint32_t cmd, uint32_t arg) {
    // Wait if command line is busy
    while (REG32(MMC0_BASE + MMC_PSTATE) & 0x1) {
        printk("Waiting for command line to be free\n");
    }

    REG32(MMC0_BASE + MMC_STAT) = 0xFFFFFFFF;  // Clear status
    REG32(MMC0_BASE + MMC_ARG) = arg;
    REG32(MMC0_BASE + MMC_CMD) = cmd;

    // Wait for command completion
    while (!(REG32(MMC0_BASE + MMC_STAT) & 0x1));

    return REG32(MMC0_BASE + MMC_RSP10);
}

#define HS_MMCSD_INTR_BADACCESS         (MMCHS_IE_BADA_ENABLE)
#define HS_MMCSD_INTR_CARDERROR         (MMCHS_IE_CERR_ENABLE)
#define HS_MMCSD_INTR_ADMAERROR         (MMCHS_IE_ADMAE_ENABLE)
#define HS_MMCSD_INTR_ACMD12ERR         (MMCHS_IE_ACE_ENABLE)
#define HS_MMCSD_INTR_DATABITERR        (MMCHS_IE_DEB_ENABLE)
#define HS_MMCSD_INTR_DATACRCERR        (MMCHS_IE_DCRC_ENABLE)
#define HS_MMCSD_INTR_DATATIMEOUT       (MMCHS_IE_DTO_ENABLE)
#define HS_MMCSD_INTR_CMDINDXERR        (MMCHS_IE_CIE_ENABLE)
#define HS_MMCSD_INTR_CMDBITERR         (MMCHS_IE_CEB_ENABLE)
#define HS_MMCSD_INTR_CMDCRCERR         (MMCHS_IE_CCRC_ENABLE)
#define HS_MMCSD_INTR_CMDTIMEOUT        (MMCHS_IE_CTO_ENABLE)
#define HS_MMCSD_INTR_CARDINS           (MMCHS_IE_CINS_ENABLE)
#define HS_MMCSD_INTR_BUFRDRDY          (MMCHS_IE_BRR_ENABLE)
#define HS_MMCSD_INTR_BUFWRRDY          (MMCHS_IE_BWR_ENABLE)
#define HS_MMCSD_INTR_TRNFCOMP          (MMCHS_IE_TC_ENABLE)
#define HS_MMCSD_INTR_CMDCOMP           (MMCHS_IE_CC_ENABLE)

/* SD SCR related macros */
#define SD_VERSION_1P0		0
#define SD_VERSION_1P1		1
#define SD_VERSION_2P0		2
#define SD_BUS_WIDTH_1BIT	1
#define SD_BUS_WIDTH_4BIT	4

/* SD OCR register definitions */
/* High capacity */
#define BIT(x) (1 << x)
#define SD_OCR_HIGH_CAPACITY    	BIT(30)
/* Voltage */
#define SD_OCR_VDD_2P7_2P8		BIT(15)
#define SD_OCR_VDD_2P8_2P9		BIT(16)
#define SD_OCR_VDD_2P9_3P0		BIT(17)
#define SD_OCR_VDD_3P0_3P1		BIT(18)
#define SD_OCR_VDD_3P1_3P2		BIT(19)
#define SD_OCR_VDD_3P2_3P3		BIT(20)
#define SD_OCR_VDD_3P3_3P4		BIT(21)
#define SD_OCR_VDD_3P4_3P5		BIT(22)
#define SD_OCR_VDD_3P5_3P6		BIT(23)

unsigned int HSMMCSDIsCardInserted(unsigned int baseAddr)
{
    return (REG32(baseAddr + MMCHS_PSTATE) & MMCHS_PSTATE_CINS) >>
                MMCHS_PSTATE_CINS_SHIFT;
}

uint32_t HSMMCSDCardPresent(mmcsdCtrlInfo *ctrl)
{
    return HSMMCSDIsCardInserted(ctrl->memBase);
}

/**
 * \brief   Soft reset the MMC/SD controller
 *
 * \param   baseAddr      Base Address of the MMC/SD controller Registers.
 *
 * \return   0   on reset success
 *          -1   on reset fail
 *
 **/
int HSMMCSDSoftReset(unsigned int baseAddr)
{
    volatile unsigned int timeout = 0xFFFF;

    REG32(baseAddr + MMCHS_SYSCONFIG) |= MMCHS_SYSCONFIG_SOFTRESET;

    do {
        if ((REG32(baseAddr + MMCHS_SYSSTATUS) & MMCHS_SYSSTATUS_RESETDONE) ==
                                               MMCHS_SYSSTATUS_RESETDONE)
        {
            break;
        }
    } while(timeout--);

    if (0 == timeout)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

int HSMMCSDLinesReset(unsigned int baseAddr, unsigned int flag)
{
    volatile unsigned int timeout = 0xFFFF;

    REG32(baseAddr + MMCHS_SYSCTL) |= flag;

    do {
        if ((REG32(baseAddr + MMCHS_SYSCTL) & flag) == flag)
        {
            break;
        }
    } while(timeout--);

    if (0 == timeout)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

/**
 * \brief   Set the supported voltage list
 *
 * \param   baseAddr      Base Address of the MMC/SD controller Registers.
 * \param   volt          Supported bus voltage
 *
 * volt can take the values (or a combination of)\n
 *     HS_MMCSD_SUPPORT_VOLT_1P8 \n
 *     HS_MMCSD_SUPPORT_VOLT_3P0 \n
 *     HS_MMCSD_SUPPORT_VOLT_3P3 \n
 *
 * \return  None.
 *
 **/
void HSMMCSDSupportedVoltSet(unsigned int baseAddr, unsigned int volt)
{
    REG32(baseAddr + MMCHS_CAPA) &= ~(MMCHS_CAPA_VS18 | MMCHS_CAPA_VS30 |
                                      MMCHS_CAPA_VS33);
    REG32(baseAddr + MMCHS_CAPA) |= volt;
}

/**
 * \brief   Configure the MMC/SD controller standby, idle and wakeup modes
 *
 * \param   baseAddr      Base Address of the MMC/SD controller Registers.
 * \param   config        The standby, idle and wakeup modes
 *
 * flag can take the values (or a combination of the following)\n
 *     HS_MMCSD_STANDBY_xxx - Standby mode configuration \n
 *     HS_MMCSD_CLOCK_xxx - Clock mode configuration \n
 *     HS_MMCSD_SMARTIDLE_xxx - Smart IDLE mode configuration \n
 *     HS_MMCSD_WAKEUP_xxx - Wake up configuration \n
 *     HS_MMCSD_AUTOIDLE_xxx - Auto IDLE mode configuration \n
 *
 * \return  None.
 *
 **/
void HSMMCSDSystemConfig(unsigned int baseAddr, unsigned int config)
{
    REG32(baseAddr + MMCHS_SYSCONFIG) &= ~(MMCHS_SYSCONFIG_STANDBYMODE |
                                          MMCHS_SYSCONFIG_CLOCKACTIVITY |
                                          MMCHS_SYSCONFIG_SIDLEMODE |
                                          MMCHS_SYSCONFIG_ENAWAKEUP |
                                          MMCHS_SYSCONFIG_AUTOIDLE);

    REG32(baseAddr + MMCHS_SYSCONFIG) |= config;
}

#define HS_MMCSD_BUS_WIDTH_8BIT    (0x8)
#define HS_MMCSD_BUS_WIDTH_4BIT    (0x4)
#define HS_MMCSD_BUS_WIDTH_1BIT    (0x1)


/**
 * \brief   Configure the MMC/SD bus width
 *
 * \param   baseAddr      Base Address of the MMC/SD controller Registers.
 * \param   width         SD/MMC bus width
 *
 * width can take the values \n
 *     HS_MMCSD_BUS_WIDTH_8BIT \n
 *     HS_MMCSD_BUS_WIDTH_4BIT \n
 *     HS_MMCSD_BUS_WIDTH_1BIT \n
 *
 * \return  None.
 *
 **/
void HSMMCSDBusWidthSet(unsigned int baseAddr, unsigned int width)
{
    switch (width)
    {
        case HS_MMCSD_BUS_WIDTH_8BIT:
            REG32(baseAddr + MMCHS_CON) |= MMCHS_CON_DW8;
        break;

        case HS_MMCSD_BUS_WIDTH_4BIT:
            REG32(baseAddr + MMCHS_CON) &= ~MMCHS_CON_DW8;
            REG32(baseAddr + MMCHS_HCTL) |=
                    (MMCHS_HCTL_DTW_4_BITMODE << MMCHS_HCTL_DTW_SHIFT);
        break;

        case HS_MMCSD_BUS_WIDTH_1BIT:
            REG32(baseAddr + MMCHS_CON) &= ~MMCHS_CON_DW8;
            REG32(baseAddr + MMCHS_HCTL) &= ~MMCHS_HCTL_DTW;
            REG32(baseAddr + MMCHS_HCTL) |=
                    (MMCHS_HCTL_DTW_1_BITMODE << MMCHS_HCTL_DTW_SHIFT);
        break;
    }
}

/**
 * \brief   Configure the MMC/SD bus voltage
 *
 * \param   baseAddr      Base Address of the MMC/SD controller Registers.
 * \param   volt          SD/MMC bus voltage
 *
 * volt can take the values \n
 *     HS_MMCSD_BUS_VOLT_1P8 \n
 *     HS_MMCSD_BUS_VOLT_3P0 \n
 *     HS_MMCSD_BUS_VOLT_3P3 \n
 *
 * \return  None.
 *
 **/
void HSMMCSDBusVoltSet(unsigned int baseAddr, unsigned int volt)
{
    REG32(baseAddr + MMCHS_HCTL) &= ~MMCHS_HCTL_SDVS;
    REG32(baseAddr + MMCHS_HCTL) |= volt;
}

int HSMMCSDBusPower(unsigned int baseAddr, unsigned int pwr)
{
    volatile unsigned int timeout = 0xFFFFF;

    REG32(baseAddr + MMCHS_HCTL) =
            (REG32(baseAddr + MMCHS_HCTL) & ~MMCHS_HCTL_SDBP) | pwr;

    if (pwr == HS_MMCSD_BUS_POWER_ON)
    {
        do {
                if ((REG32(baseAddr + MMCHS_HCTL) & MMCHS_HCTL_SDBP) != 0)
                {
                    break;
                }
        } while(timeout--);
    }

    if (timeout == 0)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

unsigned int HSMMCSDIsIntClockStable(unsigned int baseAddr, unsigned int retry)
{
    volatile unsigned int status = 0;

    do {
            status = (REG32(baseAddr + MMCHS_SYSCTL) & MMCHS_SYSCTL_ICS) >>
                                            MMCHS_SYSCTL_ICS_SHIFT;
            if ((status == 1) || (retry == 0))
            {
                break;
            }
    } while (retry--);

    return status;
}

int HSMMCSDIntClock(unsigned int baseAddr, unsigned int pwr)
{
    REG32(baseAddr + MMCHS_SYSCTL) =
            (REG32(baseAddr + MMCHS_SYSCTL) & ~MMCHS_SYSCTL_ICE) | pwr;

    if (pwr == HS_MMCSD_INTCLOCK_ON)
    {
        if(HSMMCSDIsIntClockStable(baseAddr, 0xFFFF) == 0)
        {
            return -1;
        }
    }

    return 0;
}

/**
 * \brief   Set output bus frequency
 *
 * \param   baseAddr      Base Address of the MMC/SD controller Registers.
 * \param   freq_in       The input/ref frequency to the controller
 * \param   freq_out      The required output frequency on the bus
 * \param   bypass        If the reference clock is to be bypassed
 *
 * \return   0  on clock enable success
 *          -1  on clock enable fail
 *
 * \note: If the clock is set properly, the clocks are enabled to the card with
 * the return of this function
 **/
int HSMMCSDBusFreqSet(unsigned int baseAddr, unsigned int freq_in,
                      unsigned int freq_out, unsigned int bypass)
{
    volatile unsigned int clkd = 0;
	volatile unsigned int regVal = 0;

    /* First enable the internal clocks */
    if (HSMMCSDIntClock(baseAddr, HS_MMCSD_INTCLOCK_ON) == -1)
    {
        return -1;
    }

    if (bypass == 0)
    {
        /* Calculate and program the divisor */
        clkd = freq_in / freq_out;
        clkd = (clkd < 2) ? 2 : clkd;
        clkd = (clkd > 1023) ? 1023 : clkd;

		/* Do not cross the required freq */
		while((freq_in/clkd) > freq_out)
		{
			if (clkd == 1023)
			{
				/* Return we we cannot set the clock freq */
			   return -1;
			}

			clkd++;
		}

        regVal = REG32(baseAddr + MMCHS_SYSCTL) & ~MMCHS_SYSCTL_CLKD;
        REG32(baseAddr + MMCHS_SYSCTL) = regVal | (clkd << MMCHS_SYSCTL_CLKD_SHIFT);

        /* Wait for the interface clock stabilization */
        if(HSMMCSDIsIntClockStable(baseAddr, 0xFFFF) == 0)
        {
            return -1;
        }

        /* Enable clock to the card */
        REG32(baseAddr + MMCHS_SYSCTL) |= MMCHS_SYSCTL_CEN;
    }

    return 0;
}

void HSMMCSDIntrStatusEnable(unsigned int baseAddr, unsigned int flag)
{
    REG32(baseAddr + MMCHS_IE) |= flag;
}

void HSMMCSDIntrStatusClear(unsigned int baseAddr, unsigned int flag)
{
    REG32(baseAddr + MMCHS_STAT) = flag;
}

/**
 * \brief    Checks if the command is complete
 *
 * \param    baseAddr    Base Address of the MMC/SD controller Registers
 * \param    retry       retry times to poll for completion
 *
 * \return   1 if the command is complete
 *           0 if the command is not complete
 **/
unsigned int HSMMCSDIsCmdComplete(unsigned int baseAddr, unsigned int retry)
{
    volatile unsigned int status = 0;

    do {
        status = (REG32(baseAddr + MMCHS_STAT) & MMCHS_STAT_CC) >>
                                    MMCHS_STAT_CC_SHIFT;
        if (( 1 == status) || (0  == retry))
        {
            break;
        }
    } while (retry--);

    return status;
}


/**
 * \brief   Sends INIT stream to the card
 *
 * \param   baseAddr    Base Address of the MMC/SD controller Registers.
 *
 * \return   0  If INIT command could be initiated
 *          -1  If INIT command could not be completed/initiated
 *
 **/
int HSMMCSDInitStreamSend(unsigned int baseAddr)
{
    unsigned int status;

    /* Enable the command completion status to be set */
    HSMMCSDIntrStatusEnable(baseAddr, HS_MMCSD_SIGEN_CMDCOMP);

    /* Initiate the INIT command */
    REG32(baseAddr + MMCHS_CON) |= MMCHS_CON_INIT;
    REG32(baseAddr + MMCHS_CMD) = 0x00;

    status = HSMMCSDIsCmdComplete(baseAddr, 0xFFFF);

    REG32(baseAddr + MMCHS_CON) &= ~MMCHS_CON_INIT;
    /* Clear all status */
    HSMMCSDIntrStatusClear(baseAddr, 0xFFFFFFFF);

    return status;
}



unsigned int HSMMCSDControllerInit(mmcsdCtrlInfo *ctrl)
{
    int status = 0;

    /*Refer to the MMC Host and Bus configuration steps in TRM */
    /* controller Reset */
    status = HSMMCSDSoftReset(ctrl->memBase);

    if (status != 0)
    {
        printk("HS MMC/SD Reset failed\n");
        return -1;
    }

    /* Lines Reset */
    HSMMCSDLinesReset(ctrl->memBase, HS_MMCSD_ALL_RESET);

    /* Set supported voltage list */
    HSMMCSDSupportedVoltSet(ctrl->memBase, HS_MMCSD_SUPPORT_VOLT_1P8 |
                                                HS_MMCSD_SUPPORT_VOLT_3P0);

    HSMMCSDSystemConfig(ctrl->memBase, HS_MMCSD_AUTOIDLE_ENABLE);

    /* Set the bus width */
    HSMMCSDBusWidthSet(ctrl->memBase, HS_MMCSD_BUS_WIDTH_1BIT );

    /* Set the bus voltage */
    HSMMCSDBusVoltSet(ctrl->memBase, HS_MMCSD_BUS_VOLT_3P0);

    /* Bus power on */
    status = HSMMCSDBusPower(ctrl->memBase, HS_MMCSD_BUS_POWER_ON);

    if (status != 0)
    {
        printk("HS MMC/SD Power on failed\n");
        return -1;
    }

    /* Set the initialization frequency */
    status = HSMMCSDBusFreqSet(ctrl->memBase, ctrl->ipClk, ctrl->opClk, 0);
    if (status != 0)
    {
        printk("HS MMC/SD Bus Frequency set failed\n");
        return -1;
    }

    int32_t res = HSMMCSDInitStreamSend(ctrl->memBase);

    status = (status == 0) ? 1 : 0;
    printk("HS MMC/SD Controller Init complete, status: %d res: %d\n", status, res);
    return status;
}

static void HSMMCSDControllerSetup(void)
{
    ctrlInfo.memBase = MMC0_BASE;
    ctrlInfo.ctrlInit = HSMMCSDControllerInit;
    // ctrlInfo.xferSetup = HSMMCSDXferSetup;
    // ctrlInfo.cmdStatusGet = HSMMCSDCmdStatusGet;
    // ctrlInfo.xferStatusGet = HSMMCSDXferStatusGet;
    ctrlInfo.cardPresent = HSMMCSDCardPresent;
    // ctrlInfo.cmdSend = HSMMCSDCmdSend;
    // ctrlInfo.busWidthConfig = HSMMCSDBusWidthConfig;
    // ctrlInfo.busFreqConfig = HSMMCSDBusFreqConfig;
    ctrlInfo.intrMask = (HS_MMCSD_INTR_CMDCOMP | HS_MMCSD_INTR_CMDTIMEOUT |
                            HS_MMCSD_INTR_DATATIMEOUT | HS_MMCSD_INTR_TRNFCOMP);
    // ctrlInfo.intrEnable = HSMMCSDIntEnable;
    ctrlInfo.busWidth = (SD_BUS_WIDTH_1BIT | SD_BUS_WIDTH_4BIT);
    ctrlInfo.highspeed = 1;
    ctrlInfo.ocr = MMCSD_OCR;
    ctrlInfo.dmaEnable = 0;
    // ctrlInfo.card = &sdCard;
    ctrlInfo.ipClk = MMCSD_IN_FREQ;
    ctrlInfo.opClk = MMCSD_INIT_FREQ;
    // sdCard.ctrl = &ctrlInfo;

    // cmdCompFlag = 0;
    // cmdTimeout = 0;
}

uint32_t MMCSDCardPresent(mmcsdCtrlInfo *ctrl)
{
    return ctrl->cardPresent(ctrl);
}

#define HS_MMCSD_NO_RESPONSE            (MMCHS_CMD_RSP_TYPE_NORSP << MMCHS_CMD_RSP_TYPE_SHIFT)
#define HS_MMCSD_136BITS_RESPONSE       (MMCHS_CMD_RSP_TYPE_136BITS << MMCHS_CMD_RSP_TYPE_SHIFT)
#define HS_MMCSD_48BITS_RESPONSE        (MMCHS_CMD_RSP_TYPE_48BITS << MMCHS_CMD_RSP_TYPE_SHIFT)
#define HS_MMCSD_48BITS_BUSY_RESPONSE   (MMCHS_CMD_RSP_TYPE_48BITS_BUSY << MMCHS_CMD_RSP_TYPE_SHIFT)

#define HS_MMCSD_CMD_TYPE_NORMAL        (MMCHS_CMD_CMD_TYPE_NORMAL << MMCHS_CMD_CMD_TYPE_SHIFT)
#define HS_MMCSD_CMD_TYPE_SUSPEND       (MMCHS_CMD_CMD_TYPE_SUSPEND << MMCHS_CMD_CMD_TYPE_SHIFT)
#define HS_MMCSD_CMD_TYPE_FUNCSEL       (MMCHS_CMD_CMD_TYPE_FUNC_SELECT << MMCHS_CMD_CMD_TYPE_SHIFT)
#define HS_MMCSD_CMD_TYPE_ABORT         (MMCHS_CMD_CMD_TYPE_ABORT << MMCHS_CMD_CMD_TYPE_SHIFT)

#define HS_MMCSD_CMD_DIR_READ           (MMCHS_CMD_DDIR_READ <<  MMCHS_CMD_DDIR_SHIFT)
#define HS_MMCSD_CMD_DIR_WRITE          (MMCHS_CMD_DDIR_WRITE <<  MMCHS_CMD_DDIR_SHIFT)
#define HS_MMCSD_CMD_DIR_DONTCARE       (MMCHS_CMD_DDIR_WRITE <<  MMCHS_CMD_DDIR_SHIFT)

#define SD_CMDRSP_NONE			BIT(0)
#define SD_CMDRSP_STOP			BIT(1)
#define SD_CMDRSP_FS			BIT(2)
#define SD_CMDRSP_ABORT			BIT(3)
#define SD_CMDRSP_BUSY			BIT(4)
#define SD_CMDRSP_136BITS		BIT(5)
#define SD_CMDRSP_DATA			BIT(6)
#define SD_CMDRSP_READ			BIT(7)
#define SD_CMDRSP_WRITE			BIT(8)
#define HS_MMCSD_CMD(cmd, type, restype, rw)    (cmd << MMCHS_CMD_INDX_SHIFT | type | restype | rw )

void HSMMCSDDataTimeoutSet(unsigned int baseAddr, unsigned int timeout)
{
    REG32(baseAddr + MMCHS_SYSCTL) &= ~(MMCHS_SYSCTL_DTO);
    REG32(baseAddr + MMCHS_SYSCTL) |= timeout;
}

/**
 * \brief    Pass the MMC/SD command to the controller/card
 *
 * \param   baseAddr    Base Address of the MMC/SD controller Registers
 * \param   cmd         Command to be passed to the controller/card
 * \param   cmdArg      argument for the command
 * \param   data        data pointer, if it is a data command, else must be null
 * \param   nblks       data length in number of blocks (multiple of BLEN)
 * \param   dmaEn       Should dma be enabled (1) or disabled (0)
 *
 * \note: Please use HS_MMCSD_CMD(cmd, type, restype, rw) to form command
 *
 * \return   none
 **/
void HSMMCSDCommandSend(unsigned int baseAddr, unsigned int cmd,
                        unsigned int cmdarg, void *data,
                        unsigned int nblks, unsigned int dmaEn)
{
    if (NULL != data)
    {
        cmd |= (MMCHS_CMD_DP | MMCHS_CMD_MSBS | MMCHS_CMD_BCE);
    }

    if (1 == dmaEn)
    {
        cmd |= MMCHS_CMD_DE;
    }

    /* Set the block information; block length is specified separately */
    REG32(baseAddr + MMCHS_BLK) &= ~MMCHS_BLK_NBLK;
    REG32(baseAddr + MMCHS_BLK) |= nblks << MMCHS_BLK_NBLK_SHIFT;

    /* Set the command/command argument */
    REG32(baseAddr + MMCHS_ARG) = cmdarg;
    REG32(baseAddr + MMCHS_CMD) = cmd;

}

void HSMMCSDResponseGet(unsigned int baseAddr, unsigned int *rsp)
{
    unsigned int i;

    for (i = 0; i <=3; i++)
    {
        rsp[i] = REG32(baseAddr + MMCHS_RSP(i));
    }
}

/**
 * \brief   This function sends the command to MMCSD.
 *
 * \param    mmcsdCtrlInfo It holds the mmcsd control information.
 *
 * \param    mmcsdCmd It determines the mmcsd cmd
 *
 * \return   status of the command.
 *
 **/
unsigned int HSMMCSDCmdSend(mmcsdCtrlInfo *ctrl, mmcsdCmd *c)
{
    unsigned int cmdType = HS_MMCSD_CMD_TYPE_NORMAL;
    unsigned int dataPresent;
    unsigned int status = 0;
    unsigned int rspType;
    unsigned int cmdDir;
    unsigned int nblks;
    unsigned int cmd;

    if (c->flags & SD_CMDRSP_STOP)
    {
        cmdType = HS_MMCSD_CMD_TYPE_SUSPEND;
    }
    else if (c->flags & SD_CMDRSP_FS)
    {
        cmdType = HS_MMCSD_CMD_TYPE_FUNCSEL;
    }
    else if (c->flags & SD_CMDRSP_ABORT)
    {
        cmdType = HS_MMCSD_CMD_TYPE_ABORT;
    }

    cmdDir = (c->flags & SD_CMDRSP_READ) ? \
              HS_MMCSD_CMD_DIR_READ : HS_MMCSD_CMD_DIR_WRITE;

    dataPresent = (c->flags & SD_CMDRSP_DATA) ? 1 : 0;
    nblks = (dataPresent == 1) ? c->nblks : 0;

    if (c->flags & SD_CMDRSP_NONE)
    {
        rspType = HS_MMCSD_NO_RESPONSE;
    }
    else if (c->flags & SD_CMDRSP_136BITS)
    {
        rspType = HS_MMCSD_136BITS_RESPONSE;
    }
    else if (c->flags & SD_CMDRSP_BUSY)
    {
        rspType = HS_MMCSD_48BITS_BUSY_RESPONSE;
    }
    else
    {
        rspType = HS_MMCSD_48BITS_RESPONSE;
    }

    cmd = HS_MMCSD_CMD(c->idx, cmdType, rspType, cmdDir);

    if (dataPresent)
    {
        HSMMCSDIntrStatusClear(ctrl->memBase, HS_MMCSD_STAT_TRNFCOMP);

        HSMMCSDDataTimeoutSet(ctrl->memBase, HS_MMCSD_DATA_TIMEOUT(27));
    }

    HSMMCSDCommandSend(ctrl->memBase, cmd, c->arg, (void*)dataPresent,
                       nblks, ctrl->dmaEnable);

    if (ctrl->cmdStatusGet)
    {
        status = ctrl->cmdStatusGet(ctrl);
    }

    if (status == 1)
    {
        HSMMCSDResponseGet(ctrl->memBase, c->rsp);
    }

    return status;
}

#define SD_CMDRSP_DATA			BIT(6)

// Initialize SD card
void sd_card_init(void) {
    mmcsdCmd cmd;
    uint32_t response;
    uint32_t timeout;
    // 1. Configure pins
    configure_mmc_pins();

    // 2. Enable MMC module clock
    REG32(CM_PER_MMC0_CLKCTRL) = 0x2;
    timeout = 100000;
    while ((REG32(CM_PER_MMC0_CLKCTRL) & 0x3) != 0x2) {
        if (--timeout == 0) {
            printk("Timeout waiting for MMC module clock\n");
            return;
        }
    }


    // new from here
    HSMMCSDControllerSetup();
    HSMMCSDControllerInit(&ctrlInfo);
    printk("OK\n");

    // cmd.idx = 0;
    // cmd.flags = SD_CMDRSP_NONE;
    // cmd.arg = 0;
    // uint32_t status = HSMMCSDCmdSend(&ctrlInfo, &cmd);
    // printk("INIT STATUS: %d\n", status);
 //    // 6. SD Card Initialization sequence
 //    // Send CMD0 (GO_IDLE_STATE)
    mmc_send_cmd(CMD0, 0);
    // REG32(MMC0_BASE + MMC_CMD) = CMD0;
    // timeout = 100000;
    // while (!(REG32(MMC0_BASE + MMC_STAT) & CC_MASK)) {
    //     if (--timeout == 0) {
    //         printk("Timeout waiting for MMC CMD 0\n");
    //         return;
    //     }
    // }
 //    printk("Done!\n");
 //    // Send CMD8 (SEND_IF_COND) - Required for SD 2.0
    response = mmc_send_cmd(CMD8, 0x000001AA);
    if ((response & 0xFF) != 0xAA) {
        return;  // Card doesn't support 2.7-3.6V
    }
    printk("Controller power check OK\n");

    // Send ACMD41 until card is ready
    int retry = 1000;
    do {
        mmc_send_cmd(CMD55, 0);  // APP_CMD
        response = mmc_send_cmd(ACMD41, 0x40FF8000);  // HCS=1, voltage window
        retry--;
        if (retry == 0) return;
    } while (!(response & (1 << 31)));



    // Send CMD2 (ALL_SEND_CID)
    mmc_send_cmd(CMD2, 0);

    // Send CMD3 (SEND_RELATIVE_ADDR)
    response = mmc_send_cmd(CMD3, 0);
    uint32_t rca = response >> 16;

    // Select card (CMD7)
    mmc_send_cmd(CMD7, rca << 16);

    // set 4 bit mode
    mmcsdCmd cmd1 = {
        .idx = ACMD6,
        .arg = 2, // 4-bit mode
        .flags = SD_CMDRSP_DATA | SD_CMDRSP_READ
    };
    mmc_send_cmd(CMD55, rca << 16);
    HSMMCSDCmdSend(&ctrlInfo, &cmd1);
    HSMMCSDBusWidthSet(MMC0_BASE, HS_MMCSD_BUS_WIDTH_4BIT);

    // 7. Set final clock speed (25MHz for standard cards)
    set_mmc_clock(25000);
}

// Example block read function
int sd_read_block(uint32_t sector, uint8_t* buffer) {
    // Set block length
    REG32(MMC0_BASE + MMC_BLK) = 512;  // 512 bytes

    // Send read command
    mmc_send_cmd(17, sector * 512);  // READ_SINGLE_BLOCK

    // Read data
    volatile uint32_t* data_reg = (volatile uint32_t*)(MMC0_BASE + MMC_DATA);
    for (int i = 0; i < 512/4; i++) {
        while (!(REG32(MMC0_BASE + MMC_STAT) & HS_MMCSD_STAT_BUFRDRDY));
        ((uint32_t*)buffer)[i] = *data_reg;
    }

    return 0;
}

int test_fn(uint32_t sector, uint8_t* buffer) {
    printk("Hello world!\n");
    return 0;
}

const fat32_diskio_t mmc_fat32_diskio = {
    .read_sector = sd_read_block,
};

mmc_driver_t mmc_driver = {
    .init = sd_card_init,
};
