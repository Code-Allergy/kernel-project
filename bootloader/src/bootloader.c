#include <kernel/uart.h>
#include <kernel/dram.h>
#include <kernel/ccm.h>
#include <kernel/printk.h>
#include <kernel/mmc.h>
#include <kernel/fat32.h>
#include <kernel/boot.h>
#include <kernel/mmu.h>
#include <kernel/board.h>
#include <stdint.h>

// TEMP -- should NOT be included here!!
#include "drivers/bbb/eeprom.h"
#include "drivers/bbb/ccm.h"
#include "drivers/bbb/mmc.h"
#include "drivers/bbb/i2c.h"
#include "drivers/bbb/tps65217.h"
#include "drivers/bbb/watchdog.h"
#include "drivers/bbb/cm_wakeup.h"
#include "drivers/bbb/cm_per.h"
volatile uint8_t dataFromSlave[2];
volatile uint8_t dataToSlave[3];
volatile uint32_t tCount = 0;
volatile uint32_t rCount = 0;
uint32_t device_version;
uint32_t freqMultDDR;
uint32_t oppMaxIdx;

/* MACROS to Configure MPU divider */
#define MPUPLL_N                             (23u)
#define MPUPLL_M2                            (1u)

// i2c 0
/*	System clock fed to I2C module - 48Mhz	*/
#define  I2C_SYSTEM_CLOCK		   (48000000u)
/*	Internal clock used by I2C module - 12Mhz	*/
#define  I2C_INTERNAL_CLOCK		   (12000000u)
/*	I2C bus speed or frequency - 100Khz	*/
#define	 I2C_OUTPUT_CLOCK		   (100000u)
#define  I2C_INTERRUPT_FLAG_TO_CLR         (0x7FF)

#define SOC_WDT_0_REGS                       (0x44E33000)
#define SOC_WDT_1_REGS                       (0x44E35000)

#define REG32(addr) (*(volatile uint32_t*)(addr))

extern uintptr_t init;
// BOARDINFO board_info;
#define EFUSE_OPP_MASK                       (0x00001FFFu)
#define DEVICE_VERSION_1_0                   (0u)
#define DEVICE_VERSION_2_0                   (1u)
#define DEVICE_VERSION_2_1                   (2u)

/* EFUSE OPP bit mask */
#define EFUSE_OPP_MASK                       (0x00001FFFu)

/* EFUSE bit for OPP100 275Mhz - 1.1v */
#define EFUSE_OPP100_275_MASK                (0x00000001u)
#define EFUSE_OPP100_275                     (0x0u)

/* EFUSE bit for OPP100 500Mhz - 1.1v */
#define EFUSE_OPP100_500_MASK                (0x00000002u)
#define EFUSE_OPP100_500                     (0x1u)

/* EFUSE bit for OPP120 600Mhz - 1.2v */
#define EFUSE_OPP120_600_MASK                (0x00000004u)
#define EFUSE_OPP120_600                     (0x2u)

/* EFUSE bit for OPP TURBO 720Mhz - 1.26v */
#define EFUSE_OPPTB_720_MASK                 (0x00000008u)
#define EFUSE_OPPTB_720                      (0x3u)

/* EFUSE bit for OPP50 300Mhz - 950mv */
#define EFUSE_OPP50_300_MASK                 (0x00000010u)
#define EFUSE_OPP50_300                      (0x4u)

/* EFUSE bit for OPP100 300Mhz - 1.1v */
#define EFUSE_OPP100_300_MASK                (0x00000020u)
#define EFUSE_OPP100_300                     (0x5u)

/* EFUSE bit for OPP100 600Mhz - 1.1v */
#define EFUSE_OPP100_600_MASK                (0x00000040u)
#define EFUSE_OPP100_600                     (0x6u)

/* EFUSE bit for OPP120 720Mhz - 1.2v */
#define EFUSE_OPP120_720_MASK                (0x00000050u)
#define EFUSE_OPP120_720                     (0x7u)

/* EFUSE bit for OPP TURBO 800Mhz - 1.26v */
#define EFUSE_OPPTB_800_MASK                 (0x00000100u)
#define EFUSE_OPPTB_800                      (0x8u)

/* EFUSE bit for OPP NITRO 1000Mhz - 1.325v */
#define EFUSE_OPPNT_1000_MASK                (0x00000200u)
#define EFUSE_OPPNT_1000                     (0x9u)

#define EFUSE_OPP_MAX                        (EFUSE_OPPNT_1000 + 1)
/* Types of Opp */
#define OPP_NONE                             (0u)
#define OPP_50                               (1u)
#define OPP_100                              (2u)
#define OPP_120                              (3u)
#define SR_TURBO                             (4u)
#define OPP_NITRO                            (5u)

typedef struct oppConfig
{
    unsigned int pllMult;
    unsigned int pmicVolt;
} tOPPConfig;

#define MPUPLL_M_275_MHZ                     (275u)
#define MPUPLL_M_300_MHZ                     (300u)
#define MPUPLL_M_500_MHZ                     (500u)
#define MPUPLL_M_600_MHZ                     (600u)
#define MPUPLL_M_720_MHZ                     (720u)
#define MPUPLL_M_800_MHZ                     (800u)
#define MPUPLL_M_1000_MHZ                    (1000u)

#define     PMIC_VOLT_SEL_0950MV      DCDC_VOLT_SEL_0950MV
#define     PMIC_VOLT_SEL_1100MV      DCDC_VOLT_SEL_1100MV
#define     PMIC_VOLT_SEL_1200MV      DCDC_VOLT_SEL_1200MV
#define     PMIC_VOLT_SEL_1260MV      DCDC_VOLT_SEL_1275MV
#define     PMIC_VOLT_SEL_1325MV      (0x11u)

tOPPConfig oppTable[] =
{
    {MPUPLL_M_275_MHZ, PMIC_VOLT_SEL_1100MV},  /* OPP100 275Mhz - 1.1v */
    {MPUPLL_M_500_MHZ, PMIC_VOLT_SEL_1100MV},  /* OPP100 500Mhz - 1.1v */
    {MPUPLL_M_600_MHZ, PMIC_VOLT_SEL_1200MV},  /* OPP120 600Mhz - 1.2v */
    {MPUPLL_M_720_MHZ, PMIC_VOLT_SEL_1260MV},  /* OPP TURBO 720Mhz - 1.26v */
    {MPUPLL_M_300_MHZ, PMIC_VOLT_SEL_0950MV},  /* OPP50 300Mhz - 950mv */
    {MPUPLL_M_300_MHZ, PMIC_VOLT_SEL_1100MV},  /* OPP100 300Mhz - 1.1v */
    {MPUPLL_M_600_MHZ, PMIC_VOLT_SEL_1100MV},  /* OPP100 600Mhz - 1.1v */
    {MPUPLL_M_720_MHZ, PMIC_VOLT_SEL_1200MV},  /* OPP120 720Mhz - 1.2v */
    {MPUPLL_M_800_MHZ, PMIC_VOLT_SEL_1260MV},  /* OPP TURBO 800Mhz - 1.26v */
    {MPUPLL_M_1000_MHZ, PMIC_VOLT_SEL_1325MV}  /* OPP NITRO 1000Mhz - 1.325v */
};

/*
**Setting the CORE PLL values at OPP100:
** OSCIN = 24MHz, Fdpll = 2GHz
** HSDM4 = 200MHz, HSDM5 = 250MHz
** HSDM6 = 500MHz
*/
#define COREPLL_M                          1000
#define COREPLL_N                          23
#define COREPLL_HSD_M4                     10
#define COREPLL_HSD_M5                     8
#define COREPLL_HSD_M6                     4

/* Setting the  PER PLL values at OPP100:
** OSCIN = 24MHz, Fdpll = 960MHz
** CLKLDO = 960MHz, CLKOUT = 192MHz
*/
#define PERPLL_M                           960
#define PERPLL_N                           23
#define PERPLL_M2                          5

 /* Setting the Display CLKOUT at 300MHz independently
 ** This is required for full clock 150MHz for LCD
 ** OSCIN = 24MHz, Fdpll = 300MHz
 ** CLKOUT = 150MHz
 */
#define DISPLL_M                           25
#define DISPLL_N                           3
#define DISPLL_M2                          1

// ddr
#define DDRPLL_N		           23
#define DDRPLL_M2		           1

unsigned int SysConfigOppDataGet(void)
{
    return (REG32(CONTROL_MODULE_BASE + CONTROL_EFUSE_SMA) & EFUSE_OPP_MASK);

}

uint32_t device_version_get(void) {
    return (REG32(CONTROL_MODULE_BASE + CONTROL_DEVICE_ID) >> CONTROL_DEVICE_ID_DEVREV_SHIFT);
}

void CleanupInterrupts(void)
{
    i2c_master_int_clear_ex(I2C_BASE_ADDR,  I2C_INTERRUPT_FLAG_TO_CLR);
}

void SetupI2CTransmit(unsigned int dcount)
{
    i2c_set_data_count(I2C_BASE_ADDR, dcount);

    CleanupInterrupts();

    i2c_master_control(I2C_BASE_ADDR, I2C_CFG_MST_TX);

    i2c_master_start(I2C_BASE_ADDR);

    while(i2c_master_bus_busy(I2C_BASE_ADDR) == 0);

    while((I2C_INT_TRANSMIT_READY == (i2c_master_int_raw_status(I2C_BASE_ADDR)
                                     & I2C_INT_TRANSMIT_READY)) && dcount--)
    {
        i2c_master_data_put(I2C_BASE_ADDR, dataToSlave[tCount++]);

        i2c_master_int_clear_ex(I2C_BASE_ADDR, I2C_INT_TRANSMIT_READY);
    }

    i2c_master_stop(I2C_BASE_ADDR);

    while(0 == (i2c_master_int_raw_status(I2C_BASE_ADDR) & I2C_INT_STOP_CONDITION));

    i2c_master_int_clear_ex(I2C_BASE_ADDR, I2C_INT_STOP_CONDITION);
}

uint32_t boot_max_opp_get(void) {
    unsigned int oppIdx;
    unsigned int oppSupport = SysConfigOppDataGet();

    if(DEVICE_VERSION_1_0 == device_version)
    {
        oppIdx = EFUSE_OPPTB_720;
    }
    else if(DEVICE_VERSION_2_0 == device_version)
    {
        oppIdx = EFUSE_OPPTB_800;
    }
    else if(DEVICE_VERSION_2_1 == device_version)
    {
        if(!(oppSupport & EFUSE_OPPNT_1000_MASK))
        {
            oppIdx = EFUSE_OPPNT_1000;
        }
        else if(!(oppSupport & EFUSE_OPPTB_800_MASK))
        {
            oppIdx = EFUSE_OPPTB_800;
        }
        else if(!(oppSupport & EFUSE_OPP120_720_MASK))
        {
            oppIdx = EFUSE_OPP120_720;
        }
        else if(!(oppSupport & EFUSE_OPP100_600_MASK))
        {
            oppIdx = EFUSE_OPP100_600;
        }
        else if(!(oppSupport & EFUSE_OPP100_300_MASK))
        {
            oppIdx = EFUSE_OPP100_300;
        }
        else
        {
            oppIdx = EFUSE_OPP50_300;
        }
    }
    else
    {
        return OPP_NONE;
    }

    return oppIdx;
}

void SetupReception(unsigned int dcount)
{
    i2c_set_data_count(I2C_BASE_ADDR, 1);

    CleanupInterrupts();

    i2c_master_control(I2C_BASE_ADDR, I2C_CFG_MST_TX);

    i2c_master_start(I2C_BASE_ADDR);

    while(i2c_master_bus_busy(I2C_BASE_ADDR) == 0);

    i2c_master_data_put(I2C_BASE_ADDR, dataToSlave[tCount]);

    i2c_master_int_clear_ex(I2C_BASE_ADDR, I2C_INT_TRANSMIT_READY);

    while(0 == (i2c_master_int_raw_status(I2C_BASE_ADDR) & I2C_INT_ADRR_READY_ACESS));

    i2c_set_data_count(I2C_BASE_ADDR, dcount);

    CleanupInterrupts();

    i2c_master_control(I2C_BASE_ADDR, I2C_CFG_MST_RX);

    i2c_master_start(I2C_BASE_ADDR);
    printk("here1\n");
    /* Wait till the bus if free */
    while(i2c_master_bus_busy(I2C_BASE_ADDR) == 0);
    printk("here2 dcount: %d\n", dcount);

    /* Read the data from slave of dcount */
    while((dcount--))
    {
        while(0 == (i2c_master_int_raw_status(I2C_BASE_ADDR) & I2C_INT_RECV_READY));
        printk("here loop dcount: %d\n", dcount);
        printk("here loop rcount: %d\n", rCount);
        dataFromSlave[rCount++] = i2c_master_data_get(I2C_BASE_ADDR);

        i2c_master_int_clear_ex(I2C_BASE_ADDR, I2C_INT_RECV_READY);
    }

    i2c_master_stop(I2C_BASE_ADDR);
    printk("here3\n");

    while(0 == (i2c_master_int_raw_status(I2C_BASE_ADDR) & I2C_INT_STOP_CONDITION));

    i2c_master_int_clear_ex(I2C_BASE_ADDR, I2C_INT_STOP_CONDITION);
}

void TPS65217RegRead(uint32_t regOffset, uint32_t* dest)
{
    dataToSlave[0] = regOffset;
    tCount = 0;
    SetupReception(1);

    *dest = dataFromSlave[0];
}

/**
 *  \brief            - Generic function that can write a TPS65217 PMIC
 *                      register or bit field regardless of protection
 *                      level.
 *
 * \param prot_level  - Register password protection.
 *                      use PROT_LEVEL_NONE, PROT_LEVEL_1, or PROT_LEVEL_2
 * \param regOffset:  - Register address to write.
 *
 * \param dest_val    - Value to write.
 *
 * \param mask        - Bit mask (8 bits) to be applied.  Function will only
 *                      change bits that are set in the bit mask.
 *
 * \return:            None.
 */
void TPS65217RegWrite(uint32_t port_level, uint32_t regOffset,
                      uint32_t dest_val, uint32_t mask)
{
    unsigned char read_val;
    unsigned xor_reg;

    dataToSlave[0] = regOffset;
    tCount = 0;
    rCount = 0;

    if(mask != MASK_ALL_BITS)
    {
         SetupReception(1);

         read_val = dataFromSlave[0];
         read_val &= (~mask);
         read_val |= (dest_val & mask);
         dest_val = read_val;
    }

    if(port_level > 0)
    {
         xor_reg = regOffset ^ PASSWORD_UNLOCK;

         dataToSlave[0] = PASSWORD;
         dataToSlave[1] = xor_reg;
         tCount = 0;

         SetupI2CTransmit(2);
    }

    dataToSlave[0] = regOffset;
    dataToSlave[1] = dest_val;
    tCount = 0;

    SetupI2CTransmit(2);

    if(port_level == PROT_LEVEL_2)
    {
         dataToSlave[0] = PASSWORD;
         dataToSlave[1] = xor_reg;
         tCount = 0;

         SetupI2CTransmit(2);

         dataToSlave[0] = regOffset;
         dataToSlave[1] = dest_val;
         tCount = 0;

         SetupI2CTransmit(2);
    }
}

void setup_i2c(void) {
    i2c_init_clocks();
    i2c_pin_mux_setup(0);

    i2c_master_disable(I2C_BASE_ADDR);
    i2c_soft_reset(I2C_BASE_ADDR);

    i2c_auto_idle_disable(I2C_BASE_ADDR);
    i2c_master_init_clock(I2C_BASE_ADDR, I2C_SYSTEM_CLOCK, I2C_INTERNAL_CLOCK,
							   I2C_OUTPUT_CLOCK);
    i2c_master_enable(I2C_BASE_ADDR);

    while(!i2c_system_status_get(I2C_BASE_ADDR));
}

void TPS65217VoltageUpdate(unsigned char dc_cntrl_reg, unsigned char volt_sel)
{
    /* set voltage level */
    TPS65217RegWrite(PROT_LEVEL_2, dc_cntrl_reg, volt_sel, MASK_ALL_BITS);

    /* set GO bit to initiate voltage transition */
    TPS65217RegWrite(PROT_LEVEL_2, DEFSLEW, DCDC_GO, DCDC_GO);
}

void config_vdd_op_voltage(void) {
    uint32_t pmic_status = 0;
    setup_i2c();
    printk("I2C setup done\n");

    i2c_master_slave_addr_set(I2C_BASE_ADDR, PMIC_TPS65217_I2C_SLAVE_ADDR);
    rCount = 0;
    printk("Set i2c slave\n");
    TPS65217RegRead(STATUS, &pmic_status);
    printk("PMIC:%p\n", pmic_status);

    /* Increase USB current limit to 1300mA */
    TPS65217RegWrite(PROT_LEVEL_NONE, POWER_PATH, USB_INPUT_CUR_LIMIT_1300MA,
                       USB_INPUT_CUR_LIMIT_MASK);

    /* Set DCDC2 (MPU) voltage to 1.275V */
    TPS65217VoltageUpdate(DEFDCDC2, DCDC_VOLT_SEL_1275MV);

    /* Set LDO3, LDO4 output voltage to 3.3V */
    TPS65217RegWrite(PROT_LEVEL_2, DEFLS1, LDO_VOLTAGE_OUT_3_3, LDO_MASK);


    TPS65217RegWrite(PROT_LEVEL_2, DEFLS2, LDO_VOLTAGE_OUT_3_3, LDO_MASK);

}

void SetVdd1OpVoltage(uint32_t vol_selector) {
    TPS65217VoltageUpdate(DEFDCDC2, vol_selector);
}

#define DDRPLL_M_DDR3                     (303)

void MPUPLLInit(unsigned int freqMult)
{
    volatile unsigned int regVal = 0;

    /* Put the PLL in bypass mode */
    regVal = REG32(CM_WKUP_BASE + CM_CLKMODE_DPLL_MPU) &
                ~CM_WKUP_CM_CLKMODE_DPLL_MPU_DPLL_EN;

    regVal |= CM_WKUP_CM_CLKMODE_DPLL_MPU_DPLL_EN_DPLL_MN_BYP_MODE;

    REG32(CM_WKUP_BASE + CM_CLKMODE_DPLL_MPU) = regVal;

    /* Wait for DPLL to go in to bypass mode */
    while(!(REG32(CM_WKUP_BASE + CM_WKUP_CM_IDLEST_DPLL_MPU) &
                CM_WKUP_CM_IDLEST_DPLL_MPU_ST_MN_BYPASS));

    /* Clear the MULT and DIV field of DPLL_MPU register */
    REG32(CM_WKUP_BASE + CM_WKUP_CM_CLKSEL_DPLL_MPU) &=
                      ~(CM_WKUP_CM_CLKSEL_DPLL_MPU_DPLL_MULT |
                              CM_WKUP_CM_CLKSEL_DPLL_MPU_DPLL_DIV);

    /* Set the multiplier and divider values for the PLL */
    REG32(CM_WKUP_BASE + CM_WKUP_CM_CLKSEL_DPLL_MPU) |=
                     ((freqMult << CM_WKUP_CM_CLKSEL_DPLL_MPU_DPLL_MULT_SHIFT) |
                      (MPUPLL_N << CM_WKUP_CM_CLKSEL_DPLL_MPU_DPLL_DIV_SHIFT));

    regVal = REG32(CM_WKUP_BASE + CM_WKUP_CM_DIV_M2_DPLL_MPU);

    regVal = regVal & ~CM_WKUP_CM_DIV_M2_DPLL_MPU_DPLL_CLKOUT_DIV;

    regVal = regVal | MPUPLL_M2;

    /* Set the CLKOUT2 divider */
    REG32(CM_WKUP_BASE + CM_WKUP_CM_DIV_M2_DPLL_MPU) = regVal;

    /* Now LOCK the PLL by enabling it */
    regVal = REG32(CM_WKUP_BASE + CM_WKUP_CM_CLKMODE_DPLL_MPU) &
                ~CM_WKUP_CM_CLKMODE_DPLL_MPU_DPLL_EN;

    regVal |= CM_WKUP_CM_CLKMODE_DPLL_MPU_DPLL_EN;

    REG32(CM_WKUP_BASE + CM_WKUP_CM_CLKMODE_DPLL_MPU) = regVal;

    while(!(REG32(CM_WKUP_BASE + CM_WKUP_CM_IDLEST_DPLL_MPU) &
                             CM_WKUP_CM_IDLEST_DPLL_MPU_ST_DPLL_CLK));
}

void CorePLLInit(void)
{
    volatile unsigned int regVal = 0;

    /* Enable the Core PLL */

    /* Put the PLL in bypass mode */
    regVal = REG32(CM_WKUP_BASE + CM_WKUP_CM_CLKMODE_DPLL_CORE) &
                ~CM_WKUP_CM_CLKMODE_DPLL_CORE_DPLL_EN;

    regVal |= CM_WKUP_CM_CLKMODE_DPLL_CORE_DPLL_EN_DPLL_MN_BYP_MODE;

    REG32(CM_WKUP_BASE + CM_WKUP_CM_CLKMODE_DPLL_CORE) = regVal;

    while(!(REG32(CM_WKUP_BASE + CM_WKUP_CM_IDLEST_DPLL_CORE) &
                      CM_WKUP_CM_IDLEST_DPLL_CORE_ST_MN_BYPASS));

    /* Set the multipler and divider values for the PLL */
    REG32(CM_WKUP_BASE + CM_WKUP_CM_CLKSEL_DPLL_CORE) =
        ((COREPLL_M << CM_WKUP_CM_CLKSEL_DPLL_CORE_DPLL_MULT_SHIFT) |
         (COREPLL_N << CM_WKUP_CM_CLKSEL_DPLL_CORE_DPLL_DIV_SHIFT));

    /* Configure the High speed dividers */
    /* Set M4 divider */
    regVal = REG32(CM_WKUP_BASE + CM_WKUP_CM_DIV_M4_DPLL_CORE);
    regVal = regVal & ~CM_WKUP_CM_DIV_M4_DPLL_CORE_HSDIVIDER_CLKOUT1_DIV;
    regVal = regVal | (COREPLL_HSD_M4 <<
                CM_WKUP_CM_DIV_M4_DPLL_CORE_HSDIVIDER_CLKOUT1_DIV_SHIFT);
    REG32(CM_WKUP_BASE + CM_WKUP_CM_DIV_M4_DPLL_CORE) = regVal;

    /* Set M5 divider */
    regVal = REG32(CM_WKUP_BASE + CM_WKUP_CM_DIV_M5_DPLL_CORE);
    regVal = regVal & ~CM_WKUP_CM_DIV_M5_DPLL_CORE_HSDIVIDER_CLKOUT2_DIV;
    regVal = regVal | (COREPLL_HSD_M5 <<
                CM_WKUP_CM_DIV_M5_DPLL_CORE_HSDIVIDER_CLKOUT2_DIV_SHIFT);
    REG32(CM_WKUP_BASE + CM_WKUP_CM_DIV_M5_DPLL_CORE) = regVal;

    /* Set M6 divider */
    regVal = REG32(CM_WKUP_BASE + CM_WKUP_CM_DIV_M6_DPLL_CORE);
    regVal = regVal & ~CM_WKUP_CM_DIV_M6_DPLL_CORE_HSDIVIDER_CLKOUT3_DIV;
    regVal = regVal | (COREPLL_HSD_M6 <<
                CM_WKUP_CM_DIV_M6_DPLL_CORE_HSDIVIDER_CLKOUT3_DIV_SHIFT);
    REG32(CM_WKUP_BASE + CM_WKUP_CM_DIV_M6_DPLL_CORE) = regVal;

    /* Now LOCK the PLL by enabling it */
    regVal = REG32(CM_WKUP_BASE + CM_WKUP_CM_CLKMODE_DPLL_CORE) &
                ~CM_WKUP_CM_CLKMODE_DPLL_CORE_DPLL_EN;

    regVal |= CM_WKUP_CM_CLKMODE_DPLL_CORE_DPLL_EN;

    REG32(CM_WKUP_BASE + CM_WKUP_CM_CLKMODE_DPLL_CORE) = regVal;

    while(!(REG32(CM_WKUP_BASE + CM_WKUP_CM_IDLEST_DPLL_CORE) &
                        CM_WKUP_CM_IDLEST_DPLL_CORE_ST_DPLL_CLK));
}

void PerPLLInit(void)
{
    volatile unsigned int regVal = 0;

    /* Put the PLL in bypass mode */
    regVal = REG32(CM_WKUP_BASE + CM_WKUP_CM_CLKMODE_DPLL_PER) &
                ~CM_WKUP_CM_CLKMODE_DPLL_PER_DPLL_EN;

    regVal |= CM_WKUP_CM_CLKMODE_DPLL_PER_DPLL_EN_DPLL_MN_BYP_MODE;

    REG32(CM_WKUP_BASE + CM_WKUP_CM_CLKMODE_DPLL_PER) = regVal;

    while(!(REG32(CM_WKUP_BASE + CM_WKUP_CM_IDLEST_DPLL_PER) &
                      CM_WKUP_CM_IDLEST_DPLL_PER_ST_MN_BYPASS));

    REG32(CM_WKUP_BASE + CM_WKUP_CM_CLKSEL_DPLL_PERIPH) &=
                             ~(CM_WKUP_CM_CLKSEL_DPLL_PERIPH_DPLL_MULT |
                                    CM_WKUP_CM_CLKSEL_DPLL_PERIPH_DPLL_DIV);

    /* Set the multipler and divider values for the PLL */
    REG32(CM_WKUP_BASE + CM_WKUP_CM_CLKSEL_DPLL_PERIPH) |=
        ((PERPLL_M << CM_WKUP_CM_CLKSEL_DPLL_PERIPH_DPLL_MULT_SHIFT) |
         (PERPLL_N << CM_WKUP_CM_CLKSEL_DPLL_PERIPH_DPLL_DIV_SHIFT));

    regVal = REG32(CM_WKUP_BASE + CM_WKUP_CM_DIV_M2_DPLL_PER);
    regVal = regVal & ~CM_WKUP_CM_DIV_M2_DPLL_PER_DPLL_CLKOUT_DIV;
    regVal = regVal | PERPLL_M2;

    /* Set the CLKOUT2 divider */
    REG32(CM_WKUP_BASE + CM_WKUP_CM_DIV_M2_DPLL_PER) = regVal;

    /* Now LOCK the PLL by enabling it */
    regVal = REG32(CM_WKUP_BASE + CM_WKUP_CM_CLKMODE_DPLL_PER) &
                ~CM_WKUP_CM_CLKMODE_DPLL_PER_DPLL_EN;

    regVal |= CM_WKUP_CM_CLKMODE_DPLL_PER_DPLL_EN;

    REG32(CM_WKUP_BASE + CM_WKUP_CM_CLKMODE_DPLL_PER) = regVal;

    while(!(REG32(CM_WKUP_BASE + CM_WKUP_CM_IDLEST_DPLL_PER) &
                           CM_WKUP_CM_IDLEST_DPLL_PER_ST_DPLL_CLK));
}

void DDRPLLInit(unsigned int freqMult)
{
    volatile unsigned int regVal = 0;

    /* Put the PLL in bypass mode */
    regVal = REG32(CM_WKUP_BASE + CM_WKUP_CM_CLKMODE_DPLL_DDR) &
                 ~CM_WKUP_CM_CLKMODE_DPLL_DDR_DPLL_EN;

    regVal |= CM_WKUP_CM_CLKMODE_DPLL_MPU_DPLL_EN_DPLL_MN_BYP_MODE;

    REG32(CM_WKUP_BASE + CM_WKUP_CM_CLKMODE_DPLL_DDR) = regVal;

    while(!(REG32(CM_WKUP_BASE + CM_WKUP_CM_IDLEST_DPLL_DDR) &
                      CM_WKUP_CM_IDLEST_DPLL_DDR_ST_MN_BYPASS));

    REG32(CM_WKUP_BASE + CM_WKUP_CM_CLKSEL_DPLL_DDR) &=
                     ~(CM_WKUP_CM_CLKSEL_DPLL_DDR_DPLL_MULT |
                           CM_WKUP_CM_CLKSEL_DPLL_DDR_DPLL_DIV);

    REG32(CM_WKUP_BASE + CM_WKUP_CM_CLKSEL_DPLL_DDR) |=
                     ((freqMult << CM_WKUP_CM_CLKSEL_DPLL_DDR_DPLL_MULT_SHIFT) |
                      (DDRPLL_N << CM_WKUP_CM_CLKSEL_DPLL_DDR_DPLL_DIV_SHIFT));

    regVal = REG32(CM_WKUP_BASE + CM_WKUP_CM_DIV_M2_DPLL_DDR);
    regVal = regVal & ~CM_WKUP_CM_DIV_M2_DPLL_DDR_DPLL_CLKOUT_DIV;
    regVal = regVal | DDRPLL_M2;

    /* Set the CLKOUT2 divider */
    REG32(CM_WKUP_BASE + CM_WKUP_CM_DIV_M2_DPLL_DDR) = regVal;

    /* Now LOCK the PLL by enabling it */
    regVal = REG32(CM_WKUP_BASE + CM_WKUP_CM_CLKMODE_DPLL_DDR) &
                ~CM_WKUP_CM_CLKMODE_DPLL_DDR_DPLL_EN;

    regVal |= CM_WKUP_CM_CLKMODE_DPLL_DDR_DPLL_EN;

    REG32(CM_WKUP_BASE + CM_WKUP_CM_CLKMODE_DPLL_DDR) = regVal;

    while(!(REG32(CM_WKUP_BASE + CM_WKUP_CM_IDLEST_DPLL_DDR) &
                           CM_WKUP_CM_IDLEST_DPLL_DDR_ST_DPLL_CLK));
}

void InterfaceClkInit(void)
{
    REG32(CM_PER_BASE + CM_PER_L3_CLKCTRL) |=
                                   CM_PER_L3_CLKCTRL_MODULEMODE_ENABLE;

    while((REG32(CM_PER_BASE + CM_PER_L3_CLKCTRL) &
        CM_PER_L3_CLKCTRL_MODULEMODE) != CM_PER_L3_CLKCTRL_MODULEMODE_ENABLE);

    REG32(CM_PER_BASE + CM_PER_L4LS_CLKCTRL) |=
                                       CM_PER_L4LS_CLKCTRL_MODULEMODE_ENABLE;

    while((REG32(CM_PER_BASE + CM_PER_L4LS_CLKCTRL) &
      CM_PER_L4LS_CLKCTRL_MODULEMODE) != CM_PER_L4LS_CLKCTRL_MODULEMODE_ENABLE);

    REG32(CM_PER_BASE + CM_PER_L4FW_CLKCTRL) |=
                                 CM_PER_L4FW_CLKCTRL_MODULEMODE_ENABLE;

    while((REG32(CM_PER_BASE + CM_PER_L4FW_CLKCTRL) &
      CM_PER_L4FW_CLKCTRL_MODULEMODE) != CM_PER_L4FW_CLKCTRL_MODULEMODE_ENABLE);

    REG32(CM_WKUP_BASE + CM_WKUP_L4WKUP_CLKCTRL) |=
                                          CM_PER_L4FW_CLKCTRL_MODULEMODE_ENABLE;
    while((REG32(CM_WKUP_BASE + CM_WKUP_L4WKUP_CLKCTRL) &
                        CM_WKUP_L4WKUP_CLKCTRL_MODULEMODE) !=
                                         CM_PER_L4FW_CLKCTRL_MODULEMODE_ENABLE);

    REG32(CM_PER_BASE + CM_PER_L3_INSTR_CLKCTRL) |=
                                      CM_PER_L3_INSTR_CLKCTRL_MODULEMODE_ENABLE;

    while((REG32(CM_PER_BASE + CM_PER_L3_INSTR_CLKCTRL) &
                        CM_PER_L3_INSTR_CLKCTRL_MODULEMODE) !=
                        CM_PER_L3_INSTR_CLKCTRL_MODULEMODE_ENABLE);

    REG32(CM_PER_BASE + CM_PER_L4HS_CLKCTRL) |=
                              CM_PER_L4HS_CLKCTRL_MODULEMODE_ENABLE;

    while((REG32(CM_PER_BASE + CM_PER_L4HS_CLKCTRL) &
                        CM_PER_L4HS_CLKCTRL_MODULEMODE) !=
                  CM_PER_L4HS_CLKCTRL_MODULEMODE_ENABLE);
}

void PowerDomainTransitionInit(void)
{
    REG32(CM_PER_BASE + CM_PER_L3_CLKSTCTRL) |=
                             CM_PER_L3_CLKSTCTRL_CLKTRCTRL_SW_WKUP;

     REG32(CM_PER_BASE + CM_PER_L4LS_CLKSTCTRL) |=
                             CM_PER_L4LS_CLKSTCTRL_CLKTRCTRL_SW_WKUP;

    REG32(CM_PER_BASE + CM_WKUP_CLKSTCTRL) |=
                             CM_WKUP_CLKSTCTRL_CLKTRCTRL_SW_WKUP;

    REG32(CM_PER_BASE + CM_PER_L4FW_CLKSTCTRL) |=
                              CM_PER_L4FW_CLKSTCTRL_CLKTRCTRL_SW_WKUP;

    REG32(CM_PER_BASE + CM_PER_L3S_CLKSTCTRL) |=
                                CM_PER_L3S_CLKSTCTRL_CLKTRCTRL_SW_WKUP;
}

void DisplayPLLInit(void)
{
    volatile unsigned int regVal = 0;

    /* Put the PLL in bypass mode */
    regVal = REG32(CM_WKUP_BASE + CM_WKUP_CM_CLKMODE_DPLL_DISP) &
                ~CM_WKUP_CM_CLKMODE_DPLL_DISP_DPLL_EN;

    regVal |= CM_WKUP_CM_CLKMODE_DPLL_DISP_DPLL_EN_DPLL_MN_BYP_MODE;

    REG32(CM_WKUP_BASE + CM_WKUP_CM_CLKMODE_DPLL_DISP) = regVal;

    while(!(REG32(CM_WKUP_BASE + CM_WKUP_CM_IDLEST_DPLL_DISP) &
                        CM_WKUP_CM_IDLEST_DPLL_DISP_ST_MN_BYPASS));

    REG32(CM_WKUP_BASE + CM_WKUP_CM_CLKSEL_DPLL_DISP) &=
                           ~(CM_WKUP_CM_CLKSEL_DPLL_DISP_DPLL_DIV |
                             CM_WKUP_CM_CLKSEL_DPLL_DISP_DPLL_MULT);

    /* Set the multipler and divider values for the PLL */
    REG32(CM_WKUP_BASE + CM_WKUP_CM_CLKSEL_DPLL_DISP) |=
        ((DISPLL_M << CM_WKUP_CM_CLKSEL_DPLL_DISP_DPLL_MULT_SHIFT) |
         (DISPLL_N << CM_WKUP_CM_CLKSEL_DPLL_DISP_DPLL_DIV_SHIFT));

    regVal = REG32(CM_WKUP_BASE + CM_WKUP_CM_DIV_M2_DPLL_DISP);
    regVal = regVal & ~CM_WKUP_CM_DIV_M2_DPLL_DISP_DPLL_CLKOUT_DIV;
    regVal = regVal | DISPLL_M2;

    /* Set the CLKOUT2 divider */
    REG32(CM_WKUP_BASE + CM_WKUP_CM_DIV_M2_DPLL_DISP) = regVal;

    /* Now LOCK the PLL by enabling it */
    regVal = REG32(CM_WKUP_BASE + CM_WKUP_CM_CLKMODE_DPLL_DISP) &
                ~CM_WKUP_CM_CLKMODE_DPLL_DISP_DPLL_EN;

    regVal |= CM_WKUP_CM_CLKMODE_DPLL_DISP_DPLL_EN;

    REG32(CM_WKUP_BASE + CM_WKUP_CM_CLKMODE_DPLL_DISP) = regVal;

    while(!(REG32(CM_WKUP_BASE + CM_WKUP_CM_IDLEST_DPLL_DISP) &
                         CM_WKUP_CM_IDLEST_DPLL_DISP_ST_DPLL_CLK));
}

void PLLInit(void)
{
    MPUPLLInit(oppTable[oppMaxIdx].pllMult);
    CorePLLInit();
    PerPLLInit();
    DDRPLLInit(freqMultDDR);
    InterfaceClkInit();
    PowerDomainTransitionInit();
    DisplayPLLInit();
}

void EMIFInit(void)
{
    volatile unsigned int regVal;

    /* Enable the clocks for EMIF */
    regVal = REG32(CM_PER_BASE + CM_PER_EMIF_FW_CLKCTRL) &
                ~(CM_PER_EMIF_FW_CLKCTRL_MODULEMODE);

    regVal |= CM_PER_EMIF_FW_CLKCTRL_MODULEMODE_ENABLE;

    REG32(CM_PER_BASE + CM_PER_EMIF_FW_CLKCTRL) = regVal;

    regVal = REG32(CM_PER_BASE + CM_PER_EMIF_CLKCTRL) &
                ~(CM_PER_EMIF_CLKCTRL_MODULEMODE);

    regVal |= CM_PER_EMIF_CLKCTRL_MODULEMODE_ENABLE;

    REG32(CM_PER_BASE + CM_PER_EMIF_CLKCTRL) = regVal;

    while((REG32(CM_PER_BASE + CM_PER_L3_CLKSTCTRL) &
          (CM_PER_L3_CLKSTCTRL_CLKACTIVITY_EMIF_GCLK |
           CM_PER_L3_CLKSTCTRL_CLKACTIVITY_L3_GCLK)) !=
          (CM_PER_L3_CLKSTCTRL_CLKACTIVITY_EMIF_GCLK |
           CM_PER_L3_CLKSTCTRL_CLKACTIVITY_L3_GCLK));
}

#define CONTROL_VTP_CTRL   (0xe0c)
#define CONTROL_VREF_CTRL   (0xe14)
/* VTP_CTRL */
#define CONTROL_VTP_CTRL_CLRZ   (0x00000001u)
#define CONTROL_VTP_CTRL_CLRZ_SHIFT   (0x00000000u)

#define CONTROL_VTP_CTRL_ENABLE   (0x00000040u)
#define CONTROL_VTP_CTRL_ENABLE_SHIFT   (0x00000006u)

#define CONTROL_VTP_CTRL_FILTER   (0x0000000Eu)
#define CONTROL_VTP_CTRL_FILTER_SHIFT   (0x00000001u)

#define CONTROL_VTP_CTRL_LOCK   (0x00000010u)
#define CONTROL_VTP_CTRL_LOCK_SHIFT   (0x00000004u)

#define CONTROL_VTP_CTRL_NCIN   (0x00007F00u)
#define CONTROL_VTP_CTRL_NCIN_SHIFT   (0x00000008u)

#define CONTROL_VTP_CTRL_PCIN   (0x007F0000u)
#define CONTROL_VTP_CTRL_PCIN_SHIFT   (0x00000010u)

#define CONTROL_VTP_CTRL_READY   (0x00000020u)
#define CONTROL_VTP_CTRL_READY_SHIFT   (0x00000005u)

#define CONTROL_VTP_CTRL_RSVD2   (0x00008000u)
#define CONTROL_VTP_CTRL_RSVD2_SHIFT   (0x0000000Fu)

#define CONTROL_VTP_CTRL_RSVD3   (0xFF800000u)
#define CONTROL_VTP_CTRL_RSVD3_SHIFT   (0x00000017u)

#define DDR_PHY_CTRL_REGS                  (CONTROL_MODULE_BASE + 0x2000)
#define CMD0_SLAVE_RATIO_0                 (DDR_PHY_CTRL_REGS + 0x1C)
#define CMD0_SLAVE_FORCE_0                 (DDR_PHY_CTRL_REGS + 0x20)
#define CMD0_SLAVE_DELAY_0                 (DDR_PHY_CTRL_REGS + 0x24)
#define CMD0_LOCK_DIFF_0                   (DDR_PHY_CTRL_REGS + 0x28)
#define CMD0_INVERT_CLKOUT_0               (DDR_PHY_CTRL_REGS + 0x2C)
#define CMD1_SLAVE_RATIO_0                 (DDR_PHY_CTRL_REGS + 0x50)
#define CMD1_SLAVE_FORCE_0                 (DDR_PHY_CTRL_REGS + 0x54)
#define CMD1_SLAVE_DELAY_0                 (DDR_PHY_CTRL_REGS + 0x58)
#define CMD1_LOCK_DIFF_0                   (DDR_PHY_CTRL_REGS + 0x5C)
#define CMD1_INVERT_CLKOUT_0               (DDR_PHY_CTRL_REGS + 0x60)
#define CMD2_SLAVE_RATIO_0                 (DDR_PHY_CTRL_REGS + 0x84)
#define CMD2_SLAVE_FORCE_0                 (DDR_PHY_CTRL_REGS + 0x88)
#define CMD2_SLAVE_DELAY_0                 (DDR_PHY_CTRL_REGS + 0x8C)
#define CMD2_LOCK_DIFF_0                   (DDR_PHY_CTRL_REGS + 0x90)
#define CMD2_INVERT_CLKOUT_0               (DDR_PHY_CTRL_REGS + 0x94)
#define DATA0_RD_DQS_SLAVE_RATIO_0         (DDR_PHY_CTRL_REGS + 0xC8)
#define DATA0_RD_DQS_SLAVE_RATIO_1         (DDR_PHY_CTRL_REGS + 0xCC)
#define DATA0_WR_DQS_SLAVE_RATIO_0         (DDR_PHY_CTRL_REGS + 0xDC)
#define DATA0_WR_DQS_SLAVE_RATIO_1         (DDR_PHY_CTRL_REGS + 0xE0)
#define DATA0_WRLVL_INIT_RATIO_0           (DDR_PHY_CTRL_REGS + 0xF0)
#define DATA0_WRLVL_INIT_RATIO_1           (DDR_PHY_CTRL_REGS + 0xF4)
#define DATA0_GATELVL_INIT_RATIO_0         (DDR_PHY_CTRL_REGS + 0xFC)
#define DATA0_GATELVL_INIT_RATIO_1         (DDR_PHY_CTRL_REGS + 0x100)
#define DATA0_FIFO_WE_SLAVE_RATIO_0        (DDR_PHY_CTRL_REGS + 0x108)
#define DATA0_FIFO_WE_SLAVE_RATIO_1        (DDR_PHY_CTRL_REGS + 0x10C)
#define DATA0_WR_DATA_SLAVE_RATIO_0        (DDR_PHY_CTRL_REGS + 0x120)
#define DATA0_WR_DATA_SLAVE_RATIO_1        (DDR_PHY_CTRL_REGS + 0x124)
#define DATA0_USE_RANK0_DELAYS_0           (DDR_PHY_CTRL_REGS + 0x134)
#define DATA0_LOCK_DIFF_0                  (DDR_PHY_CTRL_REGS + 0x138)
#define DATA1_RD_DQS_SLAVE_RATIO_0         (DDR_PHY_CTRL_REGS + 0x16c)
#define DATA1_RD_DQS_SLAVE_RATIO_1         (DDR_PHY_CTRL_REGS + 0x170)
#define DATA1_WR_DQS_SLAVE_RATIO_0         (DDR_PHY_CTRL_REGS + 0x180)
#define DATA1_WR_DQS_SLAVE_RATIO_1         (DDR_PHY_CTRL_REGS + 0x184)
#define DATA1_WRLVL_INIT_RATIO_0           (DDR_PHY_CTRL_REGS + 0x194)
#define DATA1_WRLVL_INIT_RATIO_1           (DDR_PHY_CTRL_REGS + 0x198)
#define DATA1_GATELVL_INIT_RATIO_0         (DDR_PHY_CTRL_REGS + 0x1a0)
#define DATA1_GATELVL_INIT_RATIO_1         (DDR_PHY_CTRL_REGS + 0x1a4)
#define DATA1_FIFO_WE_SLAVE_RATIO_0        (DDR_PHY_CTRL_REGS + 0x1ac)
#define DATA1_FIFO_WE_SLAVE_RATIO_1        (DDR_PHY_CTRL_REGS + 0x1b0)
#define DATA1_WR_DATA_SLAVE_RATIO_0        (DDR_PHY_CTRL_REGS + 0x1c4)
#define DATA1_WR_DATA_SLAVE_RATIO_1        (DDR_PHY_CTRL_REGS + 0x1c8)
#define DATA1_USE_RANK0_DELAYS_0           (DDR_PHY_CTRL_REGS + 0x1d8)
#define DATA1_LOCK_DIFF_0                  (DDR_PHY_CTRL_REGS + 0x1dc)


#define DDR3_CMD0_SLAVE_RATIO_0            (0x80)
#define DDR3_CMD0_INVERT_CLKOUT_0          (0x0)
#define DDR3_CMD1_SLAVE_RATIO_0            (0x80)
#define DDR3_CMD1_INVERT_CLKOUT_0          (0x0)
#define DDR3_CMD2_SLAVE_RATIO_0            (0x80)
#define DDR3_CMD2_INVERT_CLKOUT_0          (0x0)

#define DDR3_DATA0_RD_DQS_SLAVE_RATIO_0    (0x38)
#define DDR3_DATA0_WR_DQS_SLAVE_RATIO_0    (0x44)
#define DDR3_DATA0_FIFO_WE_SLAVE_RATIO_0   (0x94)
#define DDR3_DATA0_WR_DATA_SLAVE_RATIO_0   (0x7D)

#define DDR3_DATA0_RD_DQS_SLAVE_RATIO_1    (0x38)
#define DDR3_DATA0_WR_DQS_SLAVE_RATIO_1    (0x44)
#define DDR3_DATA0_FIFO_WE_SLAVE_RATIO_1   (0x94)
#define DDR3_DATA0_WR_DATA_SLAVE_RATIO_1   (0x7D)

#define DDR3_CONTROL_DDR_CMD_IOCTRL_0      (0x18B)
#define DDR3_CONTROL_DDR_CMD_IOCTRL_1      (0x18B)
#define DDR3_CONTROL_DDR_CMD_IOCTRL_2      (0x18B)

#define DDR3_CONTROL_DDR_DATA_IOCTRL_0      (0x18B)
#define DDR3_CONTROL_DDR_DATA_IOCTRL_1      (0x18B)

#define DDR3_CONTROL_DDR_IO_CTRL           (0xefffffff)

#define DDR3_EMIF_DDR_PHY_CTRL_1           (0x06)
#define DDR3_EMIF_DDR_PHY_CTRL_1_DY_PWRDN         (0x00100000)
#define DDR3_EMIF_DDR_PHY_CTRL_1_SHDW      (0x06)
#define DDR3_EMIF_DDR_PHY_CTRL_1_SHDW_DY_PWRDN    (0x00100000)
#define DDR3_EMIF_DDR_PHY_CTRL_2           (0x06)

#define DDR3_EMIF_SDRAM_TIM_1              (0x0AAAD4DB)
#define DDR3_EMIF_SDRAM_TIM_1_SHDW         (0x0AAAD4DB)

#define DDR3_EMIF_SDRAM_TIM_2              (0x266B7FDA)
#define DDR3_EMIF_SDRAM_TIM_2_SHDW         (0x266B7FDA)

#define DDR3_EMIF_SDRAM_TIM_3              (0x501F867F)
#define DDR3_EMIF_SDRAM_TIM_3_SHDM         (0x501F867F)

#define DDR3_EMIF_SDRAM_REF_CTRL_VAL1      (0x00000C30)
#define DDR3_EMIF_SDRAM_REF_CTRL_SHDW_VAL1 (0x00000C30)

#define DDR3_EMIF_ZQ_CONFIG_VAL            (0x50074BE4)

/*
** termination = 1 (RZQ/4)
** dynamic ODT = 2 (RZQ/2)
** SDRAM drive = 0 (RZQ/6)
** CWL = 0 (CAS write latency = 5)
** CL = 2 (CAS latency = 5)
** ROWSIZE = 7 (16 row bits)
** PAGESIZE = 2 (10 column bits)
*/
#define DDR3_EMIF_SDRAM_CONFIG             (0x61C04BB2)

#define CONTROL_IPC_MSG_REG(n)   (0x1328 + (n * 4))
#define CONTROL_DDR_CMD_IOCTRL(n)   (0x1404 + (n * 4))
#define CONTROL_DDR_DATA_IOCTRL(n)   (0x1440 + (n * 4))

/* DDR_IO_CTRL */
#define CONTROL_DDR_IO_CTRL_DDR3_RST_DEF_VAL   (0x80000000u)
#define CONTROL_DDR_IO_CTRL_DDR3_RST_DEF_VAL_SHIFT   (0x0000001Fu)

#define CONTROL_DDR_IO_CTRL_DDR_WUCLK_DISABLE   (0x40000000u)
#define CONTROL_DDR_IO_CTRL_DDR_WUCLK_DISABLE_SHIFT   (0x0000001Eu)

#define CONTROL_DDR_IO_CTRL_MDDR_SEL   (0x10000000u)
#define CONTROL_DDR_IO_CTRL_MDDR_SEL_SHIFT   (0x0000001Cu)

#define CONTROL_DDR_IO_CTRL_RSVD2   (0x20000000u)
#define CONTROL_DDR_IO_CTRL_RSVD2_SHIFT   (0x0000001Du)

#define CONTROL_DDR_IO_CTRL   (0xe04)

/* DDR_CKE_CTRL */
#define CONTROL_DDR_CKE_CTRL_DDR_CKE_CTRL   (0x00000001u)
#define CONTROL_DDR_CKE_CTRL_DDR_CKE_CTRL_SHIFT   (0x00000000u)

#define CONTROL_DDR_CKE_CTRL_SMA1   (0xFFFFFFFEu)
#define CONTROL_DDR_CKE_CTRL_SMA1_SHIFT   (0x00000001u)

static void BBBlack_DDR3PhyInit(void)
{
    /* Enable VTP */
    REG32(CONTROL_MODULE_BASE + CONTROL_VTP_CTRL) |= CONTROL_VTP_CTRL_ENABLE;
    REG32(CONTROL_MODULE_BASE + CONTROL_VTP_CTRL) &= ~CONTROL_VTP_CTRL_CLRZ;
    REG32(CONTROL_MODULE_BASE + CONTROL_VTP_CTRL) |= CONTROL_VTP_CTRL_CLRZ;
    while((REG32(CONTROL_MODULE_BASE + CONTROL_VTP_CTRL) & CONTROL_VTP_CTRL_READY) !=
                CONTROL_VTP_CTRL_READY);

    /* DDR PHY CMD0 Register configuration */
    REG32(CMD0_SLAVE_RATIO_0)   = DDR3_CMD0_SLAVE_RATIO_0;
    REG32(CMD0_INVERT_CLKOUT_0) = DDR3_CMD0_INVERT_CLKOUT_0;

    /* DDR PHY CMD1 Register configuration */
    REG32(CMD1_SLAVE_RATIO_0)   = DDR3_CMD1_SLAVE_RATIO_0;
    REG32(CMD1_INVERT_CLKOUT_0) = DDR3_CMD1_INVERT_CLKOUT_0;

    /* DDR PHY CMD2 Register configuration */
    REG32(CMD2_SLAVE_RATIO_0)   = DDR3_CMD2_SLAVE_RATIO_0;
    REG32(CMD2_INVERT_CLKOUT_0) = DDR3_CMD2_INVERT_CLKOUT_0;

    /* DATA macro configuration */
    REG32(DATA0_RD_DQS_SLAVE_RATIO_0)  = DDR3_DATA0_RD_DQS_SLAVE_RATIO_0;
    REG32(DATA0_WR_DQS_SLAVE_RATIO_0)  = DDR3_DATA0_WR_DQS_SLAVE_RATIO_0;
    REG32(DATA0_FIFO_WE_SLAVE_RATIO_0) = DDR3_DATA0_FIFO_WE_SLAVE_RATIO_0;
    REG32(DATA0_WR_DATA_SLAVE_RATIO_0) = DDR3_DATA0_WR_DATA_SLAVE_RATIO_0;
    REG32(DATA1_RD_DQS_SLAVE_RATIO_0)  = DDR3_DATA0_RD_DQS_SLAVE_RATIO_1;
    REG32(DATA1_WR_DQS_SLAVE_RATIO_0)  = DDR3_DATA0_WR_DQS_SLAVE_RATIO_1;
    REG32(DATA1_FIFO_WE_SLAVE_RATIO_0) = DDR3_DATA0_FIFO_WE_SLAVE_RATIO_1;
    REG32(DATA1_WR_DATA_SLAVE_RATIO_0) = DDR3_DATA0_WR_DATA_SLAVE_RATIO_1;

}

#define CONTROL_DDR_CKE_CTRL   (0x131c)
#define SOC_EMIF_0_REGS                      (0x4C000000)
#define EMIF_DDR_PHY_CTRL_1   (0xE4)
#define EMIF_DDR_PHY_CTRL_1_SHDW  (0xE8)
#define EMIF_DDR_PHY_CTRL_2   (0xEC)


/* SDRAM_REF_CTRL */
#define EMIF_SDRAM_REF_CTRL_REG_INITREF_DIS   (0x80000000u)
#define EMIF_SDRAM_REF_CTRL_REG_INITREF_DIS_SHIFT   (0x0000001Fu)

#define EMIF_SDRAM_REF_CTRL_REG_REFRESH_RATE   (0x0000FFFFu)
#define EMIF_SDRAM_REF_CTRL_REG_REFRESH_RATE_SHIFT   (0x00000000u)


/* SDRAM_REF_CTRL_SHDW */
#define EMIF_SDRAM_REF_CTRL_SHDW_REG_REFRESH_RATE_SHDW   (0x0000FFFFu)
#define EMIF_SDRAM_REF_CTRL_SHDW_REG_REFRESH_RATE_SHDW_SHIFT   (0x00000000u)


/* SDRAM_TIM_1 */
#define EMIF_SDRAM_TIM_1_REG_T_RAS   (0x0001F000u)
#define EMIF_SDRAM_TIM_1_REG_T_RAS_SHIFT   (0x0000000Cu)

#define EMIF_SDRAM_TIM_1_REG_T_RC   (0x00000FC0u)
#define EMIF_SDRAM_TIM_1_REG_T_RC_SHIFT   (0x00000006u)

#define EMIF_SDRAM_TIM_1_REG_T_RCD   (0x01E00000u)
#define EMIF_SDRAM_TIM_1_REG_T_RCD_SHIFT   (0x00000015u)

#define EMIF_SDRAM_TIM_1_REG_T_RP   (0x1E000000u)
#define EMIF_SDRAM_TIM_1_REG_T_RP_SHIFT   (0x00000019u)

#define EMIF_SDRAM_TIM_1_REG_T_RRD   (0x00000038u)
#define EMIF_SDRAM_TIM_1_REG_T_RRD_SHIFT   (0x00000003u)

#define EMIF_SDRAM_TIM_1_REG_T_WR   (0x001E0000u)
#define EMIF_SDRAM_TIM_1_REG_T_WR_SHIFT   (0x00000011u)

#define EMIF_SDRAM_TIM_1_REG_T_WTR   (0x00000007u)
#define EMIF_SDRAM_TIM_1_REG_T_WTR_SHIFT   (0x00000000u)


/* SDRAM_TIM_1_SHDW */
#define EMIF_SDRAM_TIM_1_SHDW_REG_T_RAS_SHDW   (0x0001F000u)
#define EMIF_SDRAM_TIM_1_SHDW_REG_T_RAS_SHDW_SHIFT   (0x0000000Cu)








/* SDRAM_TIM_2 */
#define EMIF_SDRAM_TIM_2_REG_T_CKE   (0x00000007u)
#define EMIF_SDRAM_TIM_2_REG_T_CKE_SHIFT   (0x00000000u)

#define EMIF_SDRAM_TIM_2_REG_T_RTP   (0x00000038u)
#define EMIF_SDRAM_TIM_2_REG_T_RTP_SHIFT   (0x00000003u)

#define EMIF_SDRAM_TIM_2_REG_T_XP   (0x70000000u)
#define EMIF_SDRAM_TIM_2_REG_T_XP_SHIFT   (0x0000001Cu)

#define EMIF_SDRAM_TIM_2_REG_T_XSNR   (0x01FF0000u)
#define EMIF_SDRAM_TIM_2_REG_T_XSNR_SHIFT   (0x00000010u)

#define EMIF_SDRAM_TIM_2_REG_T_XSRD   (0x0000FFC0u)
#define EMIF_SDRAM_TIM_2_REG_T_XSRD_SHIFT   (0x00000006u)


/* SDRAM_TIM_2_SHDW */
#define EMIF_SDRAM_TIM_2_SHDW_REG_T_CKE_SHDW   (0x00000007u)
#define EMIF_SDRAM_TIM_2_SHDW_REG_T_CKE_SHDW_SHIFT   (0x00000000u)







/* SDRAM_TIM_3 */
#define EMIF_SDRAM_TIM_3_REG_T_CKESR   (0x00E00000u)
#define EMIF_SDRAM_TIM_3_REG_T_CKESR_SHIFT   (0x00000015u)

#define EMIF_SDRAM_TIM_3_REG_T_RAS_MAX   (0x0000000Fu)
#define EMIF_SDRAM_TIM_3_REG_T_RAS_MAX_SHIFT   (0x00000000u)

#define EMIF_SDRAM_TIM_3_REG_T_RFC   (0x00001FF0u)
#define EMIF_SDRAM_TIM_3_REG_T_RFC_SHIFT   (0x00000004u)

#define EMIF_SDRAM_TIM_3_REG_T_TDQSCKMAX   (0x00006000u)
#define EMIF_SDRAM_TIM_3_REG_T_TDQSCKMAX_SHIFT   (0x0000000Du)

#define EMIF_SDRAM_TIM_3_REG_ZQ_ZQCS   (0x001F8000u)
#define EMIF_SDRAM_TIM_3_REG_ZQ_ZQCS_SHIFT   (0x0000000Fu)


/* SDRAM_TIM_3_SHDW */
#define EMIF_SDRAM_TIM_3_SHDW_REG_T_CKESR_SHDW   (0x00E00000u)
#define EMIF_SDRAM_TIM_3_SHDW_REG_T_CKESR_SHDW_SHIFT   (0x00000015u)

#define EMIF_SDRAM_TIM_1   (0x18)
#define EMIF_SDRAM_TIM_1_SHDW  (0x1C)
#define EMIF_SDRAM_TIM_2   (0x20)
#define EMIF_SDRAM_TIM_2_SHDW  (0x24)
#define EMIF_SDRAM_TIM_3   (0x28)
#define EMIF_SDRAM_TIM_3_SHDW  (0x2C)
#define EMIF_SDRAM_REF_CTRL   (0x10)
#define EMIF_SDRAM_REF_CTRL_SHDW   (0x14)
#define EMIF_ZQ_CONFIG   (0xC8)
#define EMIF_SDRAM_CONFIG   (0x8)
#define CONTROL_SECURE_EMIF_SDRAM_CONFIG   (0x110)
#define CONTROL_SECURE_EMIF_SDRAM_CONFIG_2   (0x114)

void BBBlack_DDR3Init(void)
{
    /* DDR3 Phy Initialization */
    BBBlack_DDR3PhyInit();

    REG32(CONTROL_MODULE_BASE + CONTROL_DDR_CMD_IOCTRL(0)) =
                                                 DDR3_CONTROL_DDR_CMD_IOCTRL_0;
    REG32(CONTROL_MODULE_BASE + CONTROL_DDR_CMD_IOCTRL(1)) =
                                                 DDR3_CONTROL_DDR_CMD_IOCTRL_1;
    REG32(CONTROL_MODULE_BASE + CONTROL_DDR_CMD_IOCTRL(2)) =
                                                 DDR3_CONTROL_DDR_CMD_IOCTRL_2;
    REG32(CONTROL_MODULE_BASE + CONTROL_DDR_DATA_IOCTRL(0)) =
                                                 DDR3_CONTROL_DDR_DATA_IOCTRL_0;
    REG32(CONTROL_MODULE_BASE + CONTROL_DDR_DATA_IOCTRL(1)) =
                                                 DDR3_CONTROL_DDR_DATA_IOCTRL_1;

    /* IO to work for DDR3 */
    REG32(CONTROL_MODULE_BASE + CONTROL_DDR_IO_CTRL) &= DDR3_CONTROL_DDR_IO_CTRL;

    REG32(CONTROL_MODULE_BASE + CONTROL_DDR_CKE_CTRL) |= CONTROL_DDR_CKE_CTRL_DDR_CKE_CTRL;

    REG32(SOC_EMIF_0_REGS + EMIF_DDR_PHY_CTRL_1) = DDR3_EMIF_DDR_PHY_CTRL_1;

    /* Dynamic Power Down */
    if((DEVICE_VERSION_2_0 == device_version) ||
       (DEVICE_VERSION_2_1 == device_version))
    {
        REG32(SOC_EMIF_0_REGS + EMIF_DDR_PHY_CTRL_1) |=
                                              DDR3_EMIF_DDR_PHY_CTRL_1_DY_PWRDN;
    }

    REG32(SOC_EMIF_0_REGS + EMIF_DDR_PHY_CTRL_1_SHDW) =
                                                 DDR3_EMIF_DDR_PHY_CTRL_1_SHDW;

    /* Dynamic Power Down */
    if((DEVICE_VERSION_2_0 == device_version) ||
       (DEVICE_VERSION_2_1 == device_version))
    {
        REG32(SOC_EMIF_0_REGS + EMIF_DDR_PHY_CTRL_1_SHDW) |=
                                         DDR3_EMIF_DDR_PHY_CTRL_1_SHDW_DY_PWRDN;
    }

    REG32(SOC_EMIF_0_REGS + EMIF_DDR_PHY_CTRL_2) = DDR3_EMIF_DDR_PHY_CTRL_2;

    REG32(SOC_EMIF_0_REGS + EMIF_SDRAM_TIM_1)      = DDR3_EMIF_SDRAM_TIM_1;
    REG32(SOC_EMIF_0_REGS + EMIF_SDRAM_TIM_1_SHDW) = DDR3_EMIF_SDRAM_TIM_1_SHDW;
    REG32(SOC_EMIF_0_REGS + EMIF_SDRAM_TIM_2)      = DDR3_EMIF_SDRAM_TIM_2;
    REG32(SOC_EMIF_0_REGS + EMIF_SDRAM_TIM_2_SHDW) = DDR3_EMIF_SDRAM_TIM_2_SHDW;
    REG32(SOC_EMIF_0_REGS + EMIF_SDRAM_TIM_3)      = DDR3_EMIF_SDRAM_TIM_3;
    REG32(SOC_EMIF_0_REGS + EMIF_SDRAM_TIM_3_SHDW) = DDR3_EMIF_SDRAM_TIM_3_SHDM;

    REG32(SOC_EMIF_0_REGS + EMIF_SDRAM_REF_CTRL)   = DDR3_EMIF_SDRAM_REF_CTRL_VAL1;
    REG32(SOC_EMIF_0_REGS + EMIF_SDRAM_REF_CTRL_SHDW) =
                                                 DDR3_EMIF_SDRAM_REF_CTRL_SHDW_VAL1;

    REG32(SOC_EMIF_0_REGS + EMIF_ZQ_CONFIG)     = DDR3_EMIF_ZQ_CONFIG_VAL;
    REG32(SOC_EMIF_0_REGS + EMIF_SDRAM_CONFIG)     = DDR3_EMIF_SDRAM_CONFIG;

    /* The CONTROL_SECURE_EMIF_SDRAM_CONFIG register exports SDRAM configuration
       information to the EMIF */
    REG32(CONTROL_MODULE_BASE + CONTROL_SECURE_EMIF_SDRAM_CONFIG) = DDR3_EMIF_SDRAM_CONFIG;

}

/* bootloader C entry point */
void loader(void){
    fat32_fs_t boot_fs;
    fat32_file_t kernel;
    int res = 0;
    uart_driver.init();
    printk("Board init: %p\n", board_info.init);
    board_info.init();
    device_version = device_version_get();

    printk("UART Active\n");
    printk("Loader loaded at %p\n", (void*)init);
    printk("Build time (UTC): %s\n", BUILD_DATE);

    printk("Board name: %s\n", board_info.name);
    printk("Version: %s\n", board_info.version);
    printk("Serial Number %s\n", board_info.serial_number);
    // printk("Device version: %p\n", version);

    config_vdd_op_voltage();

    oppMaxIdx = boot_max_opp_get();
    printk("BM: %p\n", oppMaxIdx);
    SetVdd1OpVoltage(oppTable[oppMaxIdx].pmicVolt);
    // ccm_driver.init();

    REG32(SOC_WDT_1_REGS + WDT_WSPR) = 0xAAAAu;
    while(REG32(SOC_WDT_1_REGS + WDT_WWPS) != 0x00);

    REG32(SOC_WDT_1_REGS + WDT_WSPR) = 0x5555u;
    while(REG32(SOC_WDT_1_REGS + WDT_WWPS) != 0x00);

    freqMultDDR = DDRPLL_M_DDR3;
    PLLInit();

    // enable CM
    REG32(CM_WKUP_BASE + CM_WKUP_CONTROL_CLKCTRL) =
            CM_WKUP_CONTROL_CLKCTRL_MODULEMODE_ENABLE;

    EMIFInit();
    BBBlack_DDR3Init();

    // dram_driver.init();
    // printk("DRAM Active\n");
    mmc_driver.init();

    // try and write to memory 0x80000000
    volatile uint32_t* test = (uint32_t*)0x80000000;
    *test = 0xdeadbeef;
    printk("Value of test %p\n", *test);

    printk("Done on BBB\n");

#ifndef PLATFORM_BBB

    CHECK_FAIL(fat32_mount(&boot_fs, &mmc_fat32_diskio), "Failed to mount FAT32 filesystem");
    CHECK_FAIL(fat32_open(&boot_fs, KERNEL_PATH, &kernel), "Failed to open kernel image");
    CHECK_FAIL(kernel.file_size == 0, "Kernel image is empty");



    // enable mmu, enable ram
    mmu_driver.init();
    // identity map all of dram, so we can access the kernel


    // map the rest of the memory into kernel space for the jump to kernel.
    for (uintptr_t i = DRAM_BASE; i < DRAM_BASE + DRAM_SIZE; i += PAGE_SIZE) {
        mmu_driver.map_page(NULL, (void*)(KERNEL_ENTRY + (i - DRAM_BASE)), (void*)i, L2_KERNEL_DATA_PAGE);
    }

    // identity map DRAM, so we can access the bootloader (unneeded on BBB, we are on other memory)
    for (uintptr_t i = 0; i < DRAM_SIZE; i += PAGE_SIZE) {
        mmu_driver.map_page(NULL, (void*)(i + DRAM_BASE), (void*)(i + DRAM_BASE), L2_KERNEL_DATA_PAGE);
    }

    mmu_driver.enable();

    /* Read kernel into DRAM */
    if ((res = fat32_read(&kernel, (void*)KERNEL_ENTRY, kernel.file_size)) < 0
        || res != (int)kernel.file_size) {
        printk("Bootloader failed: Failed to read entire kernel into memory! (Read %d bytes, expected %d)\n", res, kernel.file_size);
        goto bootloader_fail;
    }

    /* jump to kernel memory space */
    JUMP_KERNEL(kernel);
#endif
    // If we get here, the kernel failed to load, or bailed out
    printk("Failed to load kernel with error code %d! Halting!\n", res);
    while(1);

bootloader_fail:
    printk("Failed to init bootloader! Halting!\n");
    while(1);
}
