#include <kernel/uart.h>
#include <kernel/dram.h>
#include <kernel/ccm.h>
#include <kernel/printk.h>
#include <kernel/mmc.h>
#include <kernel/fat32.h>
#include <kernel/boot.h>
#include <kernel/mmu.h>
#include <kernel/board.h>
#include <kernel/i2c.h>
#include <stdint.h>

// TEMP -- should NOT be included here!!
#include "drivers/bbb/ccm.h"
#include "drivers/bbb/mmc.h"
#include "drivers/bbb/i2c.h"
#include "drivers/bbb/tps65217.h"
#include "drivers/bbb/watchdog.h"
#include "drivers/bbb/cm_wakeup.h"
#include "drivers/bbb/cm_per.h"
#include "drivers/bbb/hw_mcspi.h"
volatile uint8_t dataFromSlave[2];
volatile uint8_t dataToSlave[3];
volatile uint32_t tCount = 0;
volatile uint32_t rCount = 0;
uint32_t device_version;
uint32_t freqMultDDR;
uint32_t oppMaxIdx;

#define SOC_WDT_0_REGS                       (0x44E33000)
#define SOC_WDT_1_REGS                       (0x44E35000)

#ifdef BOOTLOADER
uint32_t kernel_end;
#endif

/* MACROS to Configure MPU divider */
#define MPUPLL_N                             (23u)
#define MPUPLL_M2                            (1u)

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
    // i2c_set_data_count(I2C_BASE_ADDR, 1);

    // CleanupInterrupts();

    // i2c_master_control(I2C_BASE_ADDR, I2C_CFG_MST_TX);

    // i2c_master_start(I2C_BASE_ADDR);

    // while(i2c_master_bus_busy(I2C_BASE_ADDR) == 0);

    // i2c_master_data_put(I2C_BASE_ADDR, dataToSlave[tCount]);

    // i2c_master_int_clear_ex(I2C_BASE_ADDR, I2C_INT_TRANSMIT_READY);

    // while(0 == (i2c_master_int_raw_status(I2C_BASE_ADDR) & I2C_INT_ADRR_READY_ACESS));

    i2c_set_data_count(I2C_BASE_ADDR, dcount);

    CleanupInterrupts();

    i2c_master_control(I2C_BASE_ADDR, I2C_CFG_MST_RX);

    i2c_master_start(I2C_BASE_ADDR);
    /* Wait till the bus if free */
    while(i2c_master_bus_busy(I2C_BASE_ADDR) == 0);

    /* Read the data from slave of dcount */
    while((dcount--))
    {
        while(0 == (i2c_master_int_raw_status(I2C_BASE_ADDR) & I2C_INT_RECV_READY));
        dataFromSlave[rCount++] = i2c_master_data_get(I2C_BASE_ADDR);

        i2c_master_int_clear_ex(I2C_BASE_ADDR, I2C_INT_RECV_READY);
    }

    i2c_master_stop(I2C_BASE_ADDR);

    while(0 == (i2c_master_int_raw_status(I2C_BASE_ADDR) & I2C_INT_STOP_CONDITION));

    i2c_master_int_clear_ex(I2C_BASE_ADDR, I2C_INT_STOP_CONDITION);
}

void TPS65217RegRead(uint32_t regOffset, uint32_t* dest)
{
    dataToSlave[0] = regOffset;
    tCount = 0;
    // i2c_driver.write(PMIC_TPS65217_I2C_SLAVE_ADDR, dataToSlave, 1);

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

         // SetupI2CTransmit(2);

        i2c_driver.write(PMIC_TPS65217_I2C_SLAVE_ADDR, dataToSlave, 2);
    }

    dataToSlave[0] = regOffset;
    dataToSlave[1] = dest_val;
    tCount = 0;

    // SetupI2CTransmit(2);
    i2c_driver.write(PMIC_TPS65217_I2C_SLAVE_ADDR, dataToSlave, 2);

    if(port_level == PROT_LEVEL_2)
    {
         dataToSlave[0] = PASSWORD;
         dataToSlave[1] = xor_reg;
         tCount = 0;

         // SetupI2CTransmit(2);
         i2c_driver.write(PMIC_TPS65217_I2C_SLAVE_ADDR, dataToSlave, 2);


         dataToSlave[0] = regOffset;
         dataToSlave[1] = dest_val;
         tCount = 0;

         // SetupI2CTransmit(2);
         i2c_driver.write(PMIC_TPS65217_I2C_SLAVE_ADDR, dataToSlave, 2);
    }
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
    // setup_i2c();
    i2c_driver.init();
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
    printk("Done write\n");
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


/// SPI

static void I2C0PinMux(void)
{
    REG32(CONTROL_MODULE_BASE + CONTROL_CONF_I2C0_SDA)  =
         (CONTROL_CONF_I2C0_SDA_CONF_I2C0_SDA_RXACTIVE  |
          CONTROL_CONF_I2C0_SDA_CONF_I2C0_SDA_SLEWCTRL  |
          CONTROL_CONF_I2C0_SDA_CONF_I2C0_SDA_PUTYPESEL   );

    REG32(CONTROL_MODULE_BASE + CONTROL_CONF_I2C0_SCL)  =
         (CONTROL_CONF_I2C0_SCL_CONF_I2C0_SCL_RXACTIVE  |
          CONTROL_CONF_I2C0_SCL_CONF_I2C0_SCL_SLEWCTRL  |
          CONTROL_CONF_I2C0_SCL_CONF_I2C0_SCL_PUTYPESEL );
}

/* Clear the status of all interrupts */
static void cleanupInterrupts(void)
{
    i2c_master_int_clear_ex(I2C_BASE_ADDR,  0x7FF);
}


/**
 * \brief   Reads data from the CPLD register offset
 *
 * \return  none
 *
 * \note    interrupts are not used for simplicity. In case interrupts are
 *          desired, it should be handled by the application
 */
void CPLDI2CRead(uint8_t *data, unsigned int length, unsigned char offset)
{
    unsigned int i = 0;

    /* First send the register offset - TX operation */
    i2c_set_data_count(I2C_BASE_ADDR, 1);

    cleanupInterrupts();

    i2c_master_control(I2C_BASE_ADDR, I2C_CFG_MST_TX);

    i2c_master_start(I2C_BASE_ADDR);
    printk("I2C: Waiting for bus to be free\n");
    /* Wait for the START to actually occur on the bus */
    while (0 == i2c_master_bus_busy(I2C_BASE_ADDR));

    printk("I2C: Bus is free, waiting for tx reg to be empty\n");
    /* Wait for the Tx register to be empty */
    while (0 == i2c_master_int_raw_status_ex(I2C_BASE_ADDR, I2C_INT_TRANSMIT_READY));
    printk("I2C: tx reg is empty\n");

    /* Push offset out and tell CPLD from where we intend to read the data */
    i2c_master_data_put(I2C_BASE_ADDR, offset);

    i2c_master_int_clear_ex(I2C_BASE_ADDR, I2C_INT_TRANSMIT_READY);

    printk("I2C: Waiting for address ready\n");
    while(0 == (i2c_master_int_raw_status(I2C_BASE_ADDR) & I2C_INT_ADRR_READY_ACESS));

    i2c_set_data_count(I2C_BASE_ADDR, length);

    cleanupInterrupts();

    /* Now that we have sent the register offset, start a RX operation*/
    i2c_master_control(I2C_BASE_ADDR, I2C_CFG_MST_RX);

    /* Repeated start condition */
    i2c_master_start(I2C_BASE_ADDR);
    printk("I2C: Waiting for bus to be free\n");
    while(i2c_master_bus_busy(I2C_BASE_ADDR) == 0);

    while (length--)
    {
        while (0 == i2c_master_int_raw_status_ex(I2C_BASE_ADDR, I2C_INT_RECV_READY));
        data[i] = (uint32_t)i2c_master_data_get(I2C_BASE_ADDR);
        i2c_master_int_clear_ex(I2C_BASE_ADDR, I2C_INT_RECV_READY);
    }

    i2c_master_stop(I2C_BASE_ADDR);

    while(0 == (i2c_master_int_raw_status(I2C_BASE_ADDR) & I2C_INT_STOP_CONDITION));

    i2c_master_int_clear_ex(I2C_BASE_ADDR, I2C_INT_STOP_CONDITION);

}

/**
 * \brief   Configures the I2C Instance
 *
 * \return  none
 */
void CPLDI2CSetup(void)
{
    /* Configuring system clocks for I2C0 instance. */
    i2c_init_clocks();

    /* Performing Pin Multiplexing for I2C0. */
    I2C0PinMux();

    /* Put i2c in reset/disabled state */
    i2c_master_disable(I2C_BASE_ADDR);

    /* Auto Idle functionality is Disabled */
    i2c_auto_idle_disable(I2C_BASE_ADDR);

    /* Configure i2c bus speed to 100khz */
    i2c_master_init_clock(I2C_BASE_ADDR, 48000000, 12000000, 100000);

    /* Set i2c slave address */
    i2c_master_slave_addr_set(I2C_BASE_ADDR, 0x35);

    /* Disable all I2C interrupts */
    i2c_master_int_disable_ex(I2C_BASE_ADDR, 0xFFFFFFFF);

    /* Bring I2C module out of reset */
    i2c_master_enable(I2C_BASE_ADDR);

    while(!i2c_system_status_get(I2C_BASE_ADDR));
}

#define CPLD_I2C_ADDR        0x35
#define CPLD_I2C_BASE        SOC_I2C_0_REGS
#define CPLD_CTRLREG_OFFSET    0x10
/* EVM Profile setting for McSPI */
#define MCSPI_EVM_PROFILE              (2)
#define CONTROL_CONF_SPI0_SCLK   (0x950)
#define CONTROL_CONF_SPI0_D0   (0x954)

/* CONF_SPI0_SCLK */
#define CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_MMODE   (0x00000007u)
#define CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_MMODE_SHIFT   (0x00000000u)

#define CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_PUDEN   (0x00000008u)
#define CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_PUDEN_SHIFT   (0x00000003u)

#define CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_PUTYPESEL   (0x00000010u)
#define CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_PUTYPESEL_SHIFT   (0x00000004u)

#define CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_RSVD   (0x000FFF80u)
#define CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_RSVD_SHIFT   (0x00000007u)

#define CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_RXACTIVE   (0x00000020u)
#define CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_RXACTIVE_SHIFT   (0x00000005u)

#define CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_SLEWCTRL   (0x00000040u)
#define CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_SLEWCTRL_SHIFT   (0x00000006u)


/* CONF_SPI0_D0 */
#define CONTROL_CONF_SPI0_D0_CONF_SPI0_D0_MMODE   (0x00000007u)
#define CONTROL_CONF_SPI0_D0_CONF_SPI0_D0_MMODE_SHIFT   (0x00000000u)

#define CONTROL_CONF_SPI0_D0_CONF_SPI0_D0_PUDEN   (0x00000008u)
#define CONTROL_CONF_SPI0_D0_CONF_SPI0_D0_PUDEN_SHIFT   (0x00000003u)

#define CONTROL_CONF_SPI0_D0_CONF_SPI0_D0_PUTYPESEL   (0x00000010u)
#define CONTROL_CONF_SPI0_D0_CONF_SPI0_D0_PUTYPESEL_SHIFT   (0x00000004u)

#define CONTROL_CONF_SPI0_D0_CONF_SPI0_D0_RSVD   (0x000FFF80u)
#define CONTROL_CONF_SPI0_D0_CONF_SPI0_D0_RSVD_SHIFT   (0x00000007u)

#define CONTROL_CONF_SPI0_D0_CONF_SPI0_D0_RXACTIVE   (0x00000020u)
#define CONTROL_CONF_SPI0_D0_CONF_SPI0_D0_RXACTIVE_SHIFT   (0x00000005u)

#define CONTROL_CONF_SPI0_D0_CONF_SPI0_D0_SLEWCTRL   (0x00000040u)
#define CONTROL_CONF_SPI0_D0_CONF_SPI0_D0_SLEWCTRL_SHIFT   (0x00000006u)
#define SPI_BASE                       (0x48030000)
/**
 * \brief   Reads the profile configured for the EVM from CPLD
 *
 * \return  profile number
 */
unsigned int EVMProfileGet(void)
{
    unsigned int data = 0;

    CPLDI2CSetup();
    printk("I2C setup done\n");
    CPLDI2CRead((uint8_t*)&data, 1, CPLD_CTRLREG_OFFSET);
    printk("Done reading\n");
    return (data & 0x07);
}

/**
 * \brief   This function selects the McSPI pins for use. The McSPI pins
 *          are multiplexed with pins of other peripherals in the SoC
 *
 * \param   instanceNum       The instance number of the McSPI instance to be
 *                            used.
 * \return  Returns the value S_PASS if the desired functionality is met else
 *          returns the appropriate error value. Error values can be
 *          1) E_INST_NOT_SUPP - McSPI instance not supported
 *          2) E_INVALID_PROFILE - Invalid profile setting of EVM for McSPI
 *
 * \note    This muxing depends on the profile in which the EVM is configured.
 */
int McSPIPinMuxSetup(unsigned int instanceNum)
{
    unsigned int profile = 0;
    int status = -1;

    if(0 == instanceNum)
    {
        profile = EVMProfileGet();
        printk("Back from profile\n");
        switch (profile)
        {
            case MCSPI_EVM_PROFILE:
                REG32(CONTROL_MODULE_BASE + CONTROL_CONF_SPI0_SCLK) =
                     (CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_PUTYPESEL |
                      CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_RXACTIVE);
                REG32(CONTROL_MODULE_BASE + CONTROL_CONF_SPI0_D0) =
                     (CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_PUTYPESEL |
                      CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_RXACTIVE);
                REG32(CONTROL_MODULE_BASE + CONTROL_CONF_SPI0_D1) =
                     (CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_PUTYPESEL |
                      CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_RXACTIVE);
                status = 0;
            break;
            default:
                status = -2;
            break;
        }
    }
    return status;
}

int McSPI0CSPinMuxSetup(unsigned int csPinNum)
{
    unsigned int profile = 0;
    int status = -1;

    if(0 == csPinNum)
    {
        profile = EVMProfileGet();

        switch(profile)
        {
            case MCSPI_EVM_PROFILE:
                REG32(CONTROL_MODULE_BASE + CONTROL_CONF_SPI0_CS0) =
                     (CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_PUTYPESEL |
                      CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_RXACTIVE);
                status = 0;
            break;
            default:
                status = -2;
            break;
        }
    }
    return status;
}

/**
* \brief  This API will reset the McSPI peripheral.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
*
* \return none.
**/
void McSPIReset(unsigned int baseAdd)
{
    /* Set the SOFTRESET field of MCSPI_SYSCONFIG register. */
    REG32(baseAdd + MCSPI_SYSCONFIG) |= MCSPI_SYSCONFIG_SOFTRESET;

    /* Stay in the loop until reset is done. */
    while(!(MCSPI_SYSSTATUS_RESETDONE_COMPLETED &
            REG32(baseAdd + MCSPI_SYSSTATUS)));
}

/**
* \brief  This API will enable the chip select pin.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
*
* \return none.
*
* \note:  Modification of CS polarity, SPI clock phase and polarity
          is not allowed when CS is enabled.
**/
void McSPICSEnable(unsigned int baseAdd)
{
    /* CLear PIN34 of MCSPI_MODULCTRL register. */
    REG32(baseAdd + MCSPI_MODULCTRL) &= ~MCSPI_MODULCTRL_PIN34;
}

/**
* \brief  This API will enable the McSPI controller in master mode.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
*
* \return none.
**/
void McSPIMasterModeEnable(unsigned int baseAdd)
{
    /* Clear the MS field of MCSPI_MODULCTRL register. */
    REG32(baseAdd + MCSPI_MODULCTRL) &= ~MCSPI_MODULCTRL_MS;
}

#define MCSPI_TX_RX_MODE                    (MCSPI_CH0CONF_TRM_TXRX)
#define MCSPI_RX_ONLY_MODE                  (MCSPI_CH0CONF_TRM_RXONLY << \
                                             MCSPI_CH0CONF_TRM_SHIFT)
#define MCSPI_TX_ONLY_MODE                  (MCSPI_CH0CONF_TRM_TXONLY << \
                                             MCSPI_CH0CONF_TRM_SHIFT)

/*
** Values used to configure communication on data line pins.
*/
#define MCSPI_DATA_LINE_COMM_MODE_0        ((MCSPI_CH0CONF_IS_LINE0) | \
                                            (MCSPI_CH0CONF_DPE1_ENABLED) | \
                                            (MCSPI_CH0CONF_DPE0_ENABLED))
#define MCSPI_DATA_LINE_COMM_MODE_1        ((MCSPI_CH0CONF_IS_LINE0)| \
                                            (MCSPI_CH0CONF_DPE1_ENABLED)| \
                                            (MCSPI_CH0CONF_DPE0_DISABLED << \
                                             MCSPI_CH0CONF_DPE0_SHIFT))
#define MCSPI_DATA_LINE_COMM_MODE_2        ((MCSPI_CH0CONF_IS_LINE0) | \
                                            (MCSPI_CH0CONF_DPE1_DISABLED << \
                                             MCSPI_CH0CONF_DPE1_SHIFT) | \
                                            (MCSPI_CH0CONF_DPE0_ENABLED))
#define MCSPI_DATA_LINE_COMM_MODE_3        ((MCSPI_CH0CONF_IS_LINE0) | \
                                            (MCSPI_CH0CONF_DPE1_DISABLED << \
                                             MCSPI_CH0CONF_DPE1_SHIFT) | \
                                            (MCSPI_CH0CONF_DPE0_DISABLED << \
                                             MCSPI_CH0CONF_DPE0_SHIFT))
#define MCSPI_DATA_LINE_COMM_MODE_4         ((MCSPI_CH0CONF_IS_LINE1 << \
                                              MCSPI_CH0CONF_IS_SHIFT) | \
                                             (MCSPI_CH0CONF_DPE1_ENABLED) | \
                                             (MCSPI_CH0CONF_DPE0_ENABLED))
#define MCSPI_DATA_LINE_COMM_MODE_5         ((MCSPI_CH0CONF_IS_LINE1 << \
                                              MCSPI_CH0CONF_IS_SHIFT) | \
                                             (MCSPI_CH0CONF_DPE1_ENABLED) | \
                                             (MCSPI_CH0CONF_DPE0_DISABLED << \
                                              MCSPI_CH0CONF_DPE0_SHIFT))
#define MCSPI_DATA_LINE_COMM_MODE_6         ((MCSPI_CH0CONF_IS_LINE1 << \
                                              MCSPI_CH0CONF_IS_SHIFT) | \
                                             (MCSPI_CH0CONF_DPE1_DISABLED << \
                                              MCSPI_CH0CONF_DPE1_SHIFT) | \
                                             (MCSPI_CH0CONF_DPE0_ENABLED))
#define MCSPI_DATA_LINE_COMM_MODE_7         ((MCSPI_CH0CONF_IS_LINE1 << \
                                              MCSPI_CH0CONF_IS_SHIFT) | \
                                             (MCSPI_CH0CONF_DPE1_DISABLED << \
                                              MCSPI_CH0CONF_DPE1_SHIFT) | \
                                             (MCSPI_CH0CONF_DPE0_DISABLED << \
                                              MCSPI_CH0CONF_DPE0_SHIFT))

#define MCSPI_SINGLE_CH                (MCSPI_MODULCTRL_SINGLE_SINGLE)
#define MCSPI_MULTI_CH                 (MCSPI_MODULCTRL_SINGLE_MULTI)

/**
* \brief  This API will enable the McSPI controller in master mode and
*         configure other parameters required for master mode.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  channelMode    Single/Multi channel.
* \param  trMode         Transmit/Receive mode used in master
*                        configuration.
* \param  pinMode        Interface mode and pin assignment.
* \param  chNum          Channel number of the McSPI instance used.\n
*
*         'channelMode' can take the following values.\n
*         MCSPI_SINGLE_CH - Single channel mode is used.\n
*         MCSPI_MULTI_CH  - Multi channel mode is used.\n
*
*         'trMode' can take the following values.\n
*         MCSPI_TX_RX_MODE   - Enable McSPI in TX and RX modes.\n
*         MCSPI_RX_ONLY_MODE - Enable McSPI only in RX mode.\n
*         MCSPI_TX_ONLY_MODE - Enable McSPI only in TX mode.\n
*
*         'pinMode' can take the following values.\n
*         MCSPI_DATA_LINE_COMM_MODE_n - Mode n configuration for SPIDAT[1:0].\n
*
*         For pinMode 0 <= n <= 7.\n
*
*         'chNum' can take the following values.\n
*         MCSPI_CHANNEL_n - Channel n is used for communication.\n
*
*         For chNum 0 <= n <= 3.\n
*
* \return 'retVal' which states if the combination of trMode and pinMode chosen
*          by the user is supported for communication on SPIDAT[1:0] pins.\n
*          TRUE  - Communication supported by SPIDAT[1:0].\n
*          FALSE - Communication not supported by SPIDAT[1:0].\n
*
* \note   Please refer the description about IS,DPE1,DPE0 and TRM bits for
*         proper configuration of SPIDAT[1:0].\n
**/
unsigned int McSPIMasterModeConfig(unsigned int baseAdd,
                                   unsigned int channelMode,
                                   unsigned int trMode,
                                   unsigned int pinMode,
                                   unsigned int chNum)
{
    unsigned int retVal = 0;

    /* Clear the MS field of MCSPI_MODULCTRL register. */
    REG32(baseAdd + MCSPI_MODULCTRL) &= ~MCSPI_MODULCTRL_SINGLE;

    /* Set the MS field with the user sent value. */
    REG32(baseAdd + MCSPI_MODULCTRL) |= (channelMode & MCSPI_MODULCTRL_SINGLE);

    /* Clear the TRM field of MCSPI_CHCONF register. */
    REG32(baseAdd + MCSPI_CHCONF(chNum)) &= ~MCSPI_CH0CONF_TRM;

    /* Set the TRM field with the user sent value. */
    REG32(baseAdd + MCSPI_CHCONF(chNum)) |= (trMode & MCSPI_CH0CONF_TRM);

    if(((MCSPI_TX_RX_MODE == trMode) &&
       (MCSPI_DATA_LINE_COMM_MODE_3 == pinMode)) ||
       ((MCSPI_TX_ONLY_MODE == trMode) &&
       (MCSPI_DATA_LINE_COMM_MODE_3 == pinMode)) ||
       ((MCSPI_TX_RX_MODE == trMode) &&
       (MCSPI_DATA_LINE_COMM_MODE_7 == pinMode)) ||
       ((MCSPI_TX_ONLY_MODE == trMode) &&
       (MCSPI_DATA_LINE_COMM_MODE_7 == pinMode)))
    {
        retVal = 0;
    }
    else
    {
        /* Clear the IS, DPE0, DPE1 fields of MCSPI_CHCONF register. */
        REG32(baseAdd + MCSPI_CHCONF(chNum)) &= ~(MCSPI_CH0CONF_IS |
                                                   MCSPI_CH0CONF_DPE1 |
                                                   MCSPI_CH0CONF_DPE0);

        /* Set the IS, DPE0, DPE1 fields with the user sent values. */
        REG32(baseAdd + MCSPI_CHCONF(chNum)) |= (pinMode & (MCSPI_CH0CONF_IS |
                                                            MCSPI_CH0CONF_DPE1 |
                                                            MCSPI_CH0CONF_DPE0));

        retVal = 1;
    }

    return retVal;
}

#define MCSPI_CLKD_MASK       (0xF)

/**
* \brief  This API will configure the clkD and extClk fields to generate
*         required spi clock depending on the type of granularity. It will
*         also set the phase and polarity of spiClk by the clkMode field.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  spiInClk       Clock frequency given to the McSPI module.
* \param  spiOutClk      Clock frequency on the McSPI bus.
* \param  chNum          Channel number of the McSPI instance used.
* \param  clkMode        Clock mode used.\n
*
*         'chNum' can take the following values.\n
*         MCSPI_CHANNEL_n - Channel n is used for communication.\n
*
*         For chNum 0 <= n <= 3.\n
*
*         'clkMode' can take the following values.\n
*         MCSPI_CLK_MODE_n - McSPI clock mode n.\n
*
*         For clkMode 0 <= n <= 3.\n
*
* \return none.
*
* \note:  1) clkMode depends on phase and polarity of McSPI clock.\n
*         2) To pass the desired value for clkMode please refer the
*            McSPI_CH(i)CONF register.\n
*         3) Please understand the polarity and phase of the slave device
*            connected and accordingly set the clkMode.\n
*         4) McSPIClkConfig does not have any significance in slave mode
*            because the clock signal required for communication is generated
*            by the master device.
**/
void McSPIClkConfig(unsigned int baseAdd, unsigned int spiInClk,
                    unsigned int spiOutClk, unsigned int chNum,
                    unsigned int clkMode)
{
    unsigned int fRatio = 0;
    unsigned int extClk = 0;
    unsigned int clkD = 0;

    /* Calculate the value of fRatio. */
    fRatio = (spiInClk / spiOutClk);

    /* If fRatio is not a power of 2, set granularity of 1 clock cycle */
    if(0 != (fRatio & (fRatio - 1)))
    {
        /* Set the clock granularity to 1 clock cycle.*/
        REG32(baseAdd + MCSPI_CHCONF(chNum)) |= MCSPI_CH0CONF_CLKG;

        /* Calculate the ratios clkD and extClk based on fClk */
        extClk = (fRatio - 1) >> 4;
        clkD = (fRatio - 1) & MCSPI_CLKD_MASK;

        /*Clear the extClk field of MCSPI_CHCTRL register.*/
        REG32(baseAdd + MCSPI_CHCTRL(chNum)) &= ~MCSPI_CH0CTRL_EXTCLK;

        /* Set the extClk field of MCSPI_CHCTRL register.*/
        REG32(baseAdd + MCSPI_CHCTRL(chNum)) |= (extClk <<
                                                 MCSPI_CH0CTRL_EXTCLK_SHIFT);
    }
    else
    {
        /* Clock granularity of power of 2.*/
        REG32(baseAdd + MCSPI_CHCONF(chNum)) &= ~MCSPI_CH0CONF_CLKG;

        while(1 != fRatio)
        {
            fRatio /= 2;
            clkD++;
        }
    }

    /*Clearing the clkD field of MCSPI_CHCONF register.*/
    REG32(baseAdd + MCSPI_CHCONF(chNum)) &= ~MCSPI_CH0CONF_CLKD;

    /* Configure the clkD field of MCSPI_CHCONF register.*/
    REG32(baseAdd + MCSPI_CHCONF(chNum)) |= (clkD << MCSPI_CH0CONF_CLKD_SHIFT);

    /*Clearing the clkMode field of MCSPI_CHCONF register.*/
    REG32(baseAdd + MCSPI_CHCONF(chNum)) &= ~(MCSPI_CH0CONF_PHA |
                                               MCSPI_CH0CONF_POL);

    /* Configure the clkMode of MCSPI_CHCONF register.*/
    REG32(baseAdd + MCSPI_CHCONF(chNum)) |= (clkMode & (MCSPI_CH0CONF_PHA |
                                                         MCSPI_CH0CONF_POL));
}

#define CHAR_LENGTH             0x8
#define MCSPI_IN_CLK            48000000
#define MCSPI_OUT_CLK           24000000

/* flash data read command */
#define SPI_FLASH_READ          0x03
#define SPI_CHAN                       0x0

#define MCSPI_CLK_MODE_0              ((MCSPI_CH0CONF_POL_ACTIVEHIGH << \
                                        MCSPI_CH0CONF_POL_SHIFT) | \
                                        MCSPI_CH0CONF_PHA_ODD)
#define MCSPI_CLK_MODE_1               ((MCSPI_CH0CONF_POL_ACTIVEHIGH << \
                                         MCSPI_CH0CONF_POL_SHIFT) | \
                                         MCSPI_CH0CONF_PHA_EVEN)
#define MCSPI_CLK_MODE_2               ((MCSPI_CH0CONF_POL_ACTIVELOW << \
                                         MCSPI_CH0CONF_POL_SHIFT) | \
                                         MCSPI_CH0CONF_PHA_ODD)
#define MCSPI_CLK_MODE_3               ((MCSPI_CH0CONF_POL_ACTIVELOW << \
                                         MCSPI_CH0CONF_POL_SHIFT) | \
                                         MCSPI_CH0CONF_PHA_EVEN)
#define MCSPI_WORD_LENGTH(n)           ((n - 1) << MCSPI_CH0CONF_WL_SHIFT)

/*
** Values used to set CS polarity for the McSPI peripheral.
*/
#define MCSPI_CS_POL_HIGH            (MCSPI_CH0CONF_EPOL_ACTIVEHIGH)
#define MCSPI_CS_POL_LOW             (MCSPI_CH0CONF_EPOL)

/*
** Values used to enable/disable the read/write DMA events of McSPI peripheral.
*/
#define MCSPI_DMA_RX_EVENT           (MCSPI_CH0CONF_DMAR)
#define MCSPI_DMA_TX_EVENT           (MCSPI_CH0CONF_DMAW)

/**
* \brief  This API will configure the length of McSPI word used for
*         communication.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  wordLength     Length of a data word used for McSPI communication.
* \param  chNum          Channel number of the McSPI instance used.\n
*
*         'wordLength' can take the following values.\n
*          MCSPI_WORD_LENGTH(n)  - McSPI word length is n bits long.\n
*
*          For wordLength 4 <= n <= 32.\n
*
*          'chNum' can take the following values.\n
*          MCSPI_CHANNEL_n - Channel n is used for communication.\n
*
*          For chNum n can vary from 0-3.\n
*
* \return none.
*
* \note:  wordLength can vary from 4-32 bits length. To program the
*         required value of wordLength please refer the MCSPI_CH(i)CONF
*         register.\n
**/
void McSPIWordLengthSet(unsigned int baseAdd, unsigned int wordLength,
                        unsigned int chNum)
{
    /*Clearing the wordLength field of MCSPI_CHCONF register.*/
    REG32(baseAdd + MCSPI_CHCONF(chNum)) &= ~MCSPI_CH0CONF_WL;

    /* Setting the wordlength field. */
    REG32(baseAdd + MCSPI_CHCONF(chNum)) |= (wordLength & MCSPI_CH0CONF_WL);
}

/*
** Values used to enable/disable the Tx/Rx FIFOs of McSPI peripheral.
*/
#define MCSPI_RX_FIFO_ENABLE         (MCSPI_CH0CONF_FFER)
#define MCSPI_RX_FIFO_DISABLE        (MCSPI_CH0CONF_FFER_FFDISABLED)
#define MCSPI_TX_FIFO_ENABLE         (MCSPI_CH0CONF_FFEW)
#define MCSPI_TX_FIFO_DISABLE        (MCSPI_CH0CONF_FFEW_FFDISABLED)

/**
* \brief  This API will configure the chip select polarity.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  spiEnPol       Polarity of CS line.
* \param  chNum          Channel number of the McSPI instance used.\n
*
*        'spiEnPol' can take the following values.\n
*         MCSPI_CS_POL_HIGH - SPIEN pin is held high during the active state.\n
*         MCSPI_CS_POL_LOW - SPIEN pin is held low during the active state.\n
*
*         'chNum' can take the following values.\n
*         MCSPI_CHANNEL_n - Channel n is used for communication.\n
*
*         For chNum 0 <= n <= 3.\n
*
* \return none.
**/
void McSPICSPolarityConfig(unsigned int baseAdd, unsigned int spiEnPol,
                           unsigned int chNum)
{
    /* Clear the EPOL field of MCSPI_CHCONF register. */
    REG32(baseAdd + MCSPI_CHCONF(chNum)) &= ~MCSPI_CH0CONF_EPOL;

    /* Set the EPOL field with the user sent value. */
    REG32(baseAdd + MCSPI_CHCONF(chNum)) |= (spiEnPol & MCSPI_CH0CONF_EPOL);
}


/**
* \brief  This API will enable/disable the Tx FIFOs of McSPI peripheral.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  txFifo         FIFO used for transmit mode.
* \param  chNum          Channel number of the McSPI instance used.\n
*
*         'txFifo' can take the following values.\n
*         MCSPI_TX_FIFO_ENABLE  - Enables the transmitter FIFO of McSPI.\n
*         MCSPI_TX_FIFO_DISABLE - Disables the transmitter FIFO of McSPI.\n
*
*         'chNum' can take the following values.\n
*         MCSPI_CHANNEL_n - Channel n is used for communication.\n
*
*         For chNum n can range from 0-3.\n
*
* \return none.
*
* \note:  Enabling FIFO is restricted to only 1 channel.
**/
void McSPITxFIFOConfig(unsigned int baseAdd, unsigned int txFifo,
                       unsigned int chNum)
{
    /* Clear the FFEW field of MCSPI_CHCONF register. */
    REG32(baseAdd + MCSPI_CHCONF(chNum)) &= ~MCSPI_CH0CONF_FFEW;

    /* Set the FFEW field with user sent value. */
    REG32(baseAdd + MCSPI_CHCONF(chNum)) |= (txFifo & MCSPI_CH0CONF_FFEW);
}

/**
* \brief  This API will enable/disable the Rx FIFOs of McSPI peripheral.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  rxFifo         FIFO used for receive mode.
* \param  chNum          Channel number of the McSPI instance used.\n
*
*         'rxFifo' can take the following values.\n
*         MCSPI_RX_FIFO_ENABLE - Enables the receiver FIFO of McSPI.\n
*         MCSPI_RX_FIFO_DISABLE - Disables the receiver FIFO of McSPI.\n
*
*         'chNum' can take the following values.\n
*         MCSPI_CHANNEL_n - Channel n is used for communication.\n
*
*         For chNum n can range from 0-3.\n
*
* \return none.
*
* \note:  Enabling FIFO is restricted to only 1 channel.
**/
void McSPIRxFIFOConfig(unsigned int baseAdd, unsigned int rxFifo,
                       unsigned int chNum)
{
    /* Clear the FFER field of MCSPI_CHCONF register. */
    REG32(baseAdd + MCSPI_CHCONF(chNum)) &= ~MCSPI_CH0CONF_FFER;

    /* Set the FFER field with the user sent value. */
    REG32(baseAdd + MCSPI_CHCONF(chNum)) |= (rxFifo & MCSPI_CH0CONF_FFER);
}

/*
* \brief - SPI Configures.
* \param - none.
*
* \return  none.
*/

void SPIConfigure(void)
{
    unsigned int retVal = 0;

    /* Reset the McSPI instance.*/
    McSPIReset(SPI_BASE);

    /* Enable chip select pin.*/
    McSPICSEnable(SPI_BASE);

    /* Enable master mode of operation.*/
    McSPIMasterModeEnable(SPI_BASE);

    /* Perform the necessary configuration for master mode.*/
    retVal = McSPIMasterModeConfig(SPI_BASE, MCSPI_SINGLE_CH, MCSPI_TX_RX_MODE,\
                                   MCSPI_DATA_LINE_COMM_MODE_1, SPI_CHAN);

    /*
    ** If combination of trm and IS,DPE0 and DPE1 is not valid then retVal is
    ** false.
    */
    if(!retVal)
    {
        printk("Invalid McSPI config \n");
        printk("Aborting Boot\n");
        while(1);
    }

    /*
    ** Default granularity is used. Also as per my understanding clock mode
    ** 0 is proper.
    */
    McSPIClkConfig(SPI_BASE, MCSPI_IN_CLK, MCSPI_OUT_CLK,
                   SPI_CHAN, MCSPI_CLK_MODE_0);

    /* Configure the word length.*/
    McSPIWordLengthSet(SPI_BASE, MCSPI_WORD_LENGTH(8), SPI_CHAN);

    /* Set polarity of SPIEN to low.*/
    McSPICSPolarityConfig(SPI_BASE, MCSPI_CS_POL_LOW, SPI_CHAN);

    /* Enable the transmitter FIFO of McSPI peripheral.*/
    McSPITxFIFOConfig(SPI_BASE, MCSPI_TX_FIFO_ENABLE, SPI_CHAN);

    /* Enable the receiver FIFO of McSPI peripheral.*/
    McSPIRxFIFOConfig(SPI_BASE, MCSPI_RX_FIFO_ENABLE, SPI_CHAN);
}

extern int xmodemReceive(unsigned char *dest, int destsz);
#define BL_UART_MAX_IMAGE_SIZE    (128*1024*1024)
static unsigned int UARTBootCopy(void)
{
    unsigned int retVal = 1;
    int32_t read = 0;
    printk("<<<UART_READY>>>\n"); // special character maybe

    if( 0 > xmodemReceive((unsigned char *)DRAM_BASE,
                          BL_UART_MAX_IMAGE_SIZE))
    {
        printk("\nXmodem receive error\n", -1);
        retVal = 0;
    }

    printk("\nCopying application image from UART to RAM is  done\n");

    // entryPoint  = 0x80000000; // DDR_START_ADDR;

    /*
    ** Dummy return.
    */
    return retVal;
}


void spi_setup(void) {
    unsigned int regVal;

    /* Enable clock for SPI */
    regVal = (REG32(CM_PER_BASE + CM_PER_SPI0_CLKCTRL) &
                    ~(CM_PER_SPI0_CLKCTRL_MODULEMODE));

    regVal |= CM_PER_SPI0_CLKCTRL_MODULEMODE_ENABLE;

    REG32(CM_PER_BASE + CM_PER_SPI0_CLKCTRL) = regVal;

    printk("Clock enabled\n");
    /* Setup SPI PINMUX */
    McSPIPinMuxSetup(0);
    printk("done spimix\n");
    McSPI0CSPinMuxSetup(0);
    printk("Done muxing\n");
    SPIConfigure();
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
    printk("Loader init loaded at %p\n", (void*)init);
    printk("Loader loaded at %p\n", (void*)loader);
    printk("Build time (UTC): %s\n", BUILD_DATE);

    printk("Board name: %s\n", board_info.name);
    printk("Version: %s\n", board_info.version);
    printk("Serial Number %s\n", board_info.serial_number);
    // printk("Device version: %p\n", version);
    config_vdd_op_voltage();

    oppMaxIdx = boot_max_opp_get();
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
    printk("Done on BBB\n");


    // dram_driver.init();
    // printk("DRAM Active\n");
    // mmc_driver.init();

    // // try and write to memory 0x80000000
    // *test = 0xdeadbeef;


    // spi_setup();

    // mmc_fat32_diskio.read_sector(0, buffer);

    // UARTBootCopy();
    // volatile uint32_t* test = (uint32_t*)0x80000000;
    // printk("First 4 instructions %p %p %p %p\n", test[0], test[1], test[2], test[3]);

    // printk("Done!\n");
    // mmu_driver.init();
    // map the rest of the memory into kernel space for the jump to kernel.
    // for (uintptr_t i = DRAM_BASE; i < DRAM_BASE + DRAM_SIZE; i += PAGE_SIZE) {
    //     mmu_driver.map_page(NULL, (void*)(KERNEL_ENTRY + (i - DRAM_BASE)), (void*)i, L2_KERNEL_DATA_PAGE);
    // }

    // identity map DRAM, so we can access the bootloader (unneeded on BBB, we are on other memory)
    // for (uintptr_t i = 0; i < DRAM_SIZE; i += PAGE_SIZE) {
    //     mmu_driver.map_page(NULL, (void*)(i + DRAM_BASE), (void*)(i + DRAM_BASE), L2_KERNEL_DATA_PAGE);
    // }

    // printk("About to enable MMU\n");
    // mmu_driver.enable();
    // printk("MMU enabled\n");
    // JUMP_KERNEL(kernel);
    while(1);

    // CHECK_FAIL(fat32_mount(&boot_fs, &mmc_fat32_diskio), "Failed to mount FAT32 filesystem");
#ifndef PLATFORM_BBB
    // CHECK_FAIL(fat32_open(&boot_fs, KERNEL_PATH, &kernel), "Failed to open kernel image");
    // CHECK_FAIL(kernel.file_size == 0, "Kernel image is empty");



    // enable mmu, enable ram

    // identity map all of dram, so we can access the kernel

    /* Read kernel into DRAM */
    // if ((res = fat32_read(&kernel, (void*)KERNEL_ENTRY, kernel.file_size)) < 0
    //     || res != (int)kernel.file_size) {
    //     printk("Bootloader failed: Failed to read entire kernel into memory! (Read %d bytes, expected %d)\n", res, kernel.file_size);
    //     goto bootloader_fail;
    // }

    /* jump to kernel memory space */
#endif
    // If we get here, the kernel failed to load, or bailed out
    printk("Failed to load kernel with error code %d! Halting!\n", res);
    while(1);

bootloader_fail:
    printk("Failed to init bootloader! Halting!\n");
    while(1);
}
