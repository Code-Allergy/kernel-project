#include <kernel/printk.h>
#include <kernel/ccm.h>
#include "mmc.h"
#include <stdint.h>
#include "i2c.h"
#include "cm_wakeup.h"
#include "cm_per.h"
#include "hw_control.h"
#include "tps65217.h"
#include "board.h"

#include <kernel/ccm.h>
#include <kernel/printk.h>
#include <kernel/mmc.h>
#include <kernel/i2c.h>


// can make this api much better later
volatile uint8_t dataFromSlave[2];
volatile uint8_t dataToSlave[3];
volatile uint32_t tCount = 0;
volatile uint32_t rCount = 0;
uint32_t device_version;
uint32_t freqMultDDR;
uint32_t oppMaxIdx;

#define LED_PINS (0xF << 21)
#define CM_PER_BASE         0x44E00000
#define CM_WKUP_BASE         0x44E00400
#define CM_DPLL_BASE         0x44E00500
#define CTRL_MODULE_BASE    0x44E10000

// Power Management Registers

#define PRM_RSTCTRL         0x44E00F00


// Control Module Clock Registers
#define CM_PER_L3_CLKSTCTRL    (CM_PER_BASE + 0x0C)
#define CM_PER_L3_INSTR_CLKCTRL (CM_PER_BASE + 0xDC)
#define CM_PER_L3_CLKCTRL      (CM_PER_BASE + 0xE0)
#define CM_PER_OCPWP_L3_CLKSTCTRL (CM_PER_BASE + 0x12C)

#define CM_CLKSEL_DPLL_MPU      (CM_WKUP_BASE + 0x2C)
#define CM_CLKMODE_DPLL_CORE    (CM_WKUP_BASE + 0x90)
#define CM_IDLEST_DPLL_CORE     (CM_WKUP_BASE + 0x5C)
#define CM_CLKMODE_DPLL_MPU     (CM_WKUP_BASE + 0x88)
#define CM_IDLEST_DPLL_MPU      (CM_WKUP_BASE + 0x20)
#define CM_CLKSEL_DPLL_CORE    (CM_WKUP_BASE + 0x68)
#define CM_CLKMODE_DPLL_CORE   (CM_WKUP_BASE + 0x90)
#define CM_DIV_M4_DPLL_CORE    (CM_WKUP_BASE + 0x80)
#define CM_DIV_M5_DPLL_CORE    (CM_WKUP_BASE + 0x84)
#define CM_DIV_M6_DPLL_CORE    (CM_WKUP_BASE + 0xD8)
#define CM_WKUP_CLKSTCTRL   (CM_WKUP_BASE + 0x00)
#define CM_WKUP_CONTROL_CLKCTRL (CM_WKUP_BASE + 0x4)
#define CM_IDLEST_DPLL_PER     (CM_WKUP_BASE + 0x70)
#define CM_CLKMODE_DPLL_PER     (CM_WKUP_BASE + 0x8C)
#define CM_CLKSEL_DPLL_PERIPH     (CM_WKUP_BASE + 0x9C)
#define CM_DIV_M2_DPLL_PER   (CM_WKUP_BASE + 0xAC)

// Control Module Registers
#define CONTROL_STATUS      (CTRL_MODULE_BASE + 0x40)
#define CONTROL_INIT        (CTRL_MODULE_BASE + 0x44)


#define DDRPLL_M_DDR3                     (303)

/* MACROS to Configure MPU divider */
#define MPUPLL_N                             (23u)
#define MPUPLL_M2                            (1u)

#define COREPLL_M                          1000
#define COREPLL_N                          23
#define COREPLL_HSD_M4                     10

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


void MPUPLLInit(unsigned int freqMult)
{
    volatile unsigned int regVal = 0;

    /* Put the PLL in bypass mode */
    regVal = REG32(CM_CLKMODE_DPLL_MPU) &
                ~CM_WKUP_CM_CLKMODE_DPLL_MPU_DPLL_EN;

    regVal |= CM_WKUP_CM_CLKMODE_DPLL_MPU_DPLL_EN_DPLL_MN_BYP_MODE;

    REG32(CM_CLKMODE_DPLL_MPU) = regVal;

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
    REG32(CM_PER_L3_CLKCTRL) |=
                                   CM_PER_L3_CLKCTRL_MODULEMODE_ENABLE;

    while((REG32(CM_PER_L3_CLKCTRL) &
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

    REG32(CM_PER_L3_INSTR_CLKCTRL) |=
                                      CM_PER_L3_INSTR_CLKCTRL_MODULEMODE_ENABLE;

    while((REG32(CM_PER_L3_INSTR_CLKCTRL) &
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
    REG32(CM_PER_L3_CLKSTCTRL) |=
                             CM_PER_L3_CLKSTCTRL_CLKTRCTRL_SW_WKUP;

     REG32(CM_PER_BASE + CM_PER_L4LS_CLKSTCTRL) |=
                             CM_PER_L4LS_CLKSTCTRL_CLKTRCTRL_SW_WKUP;

    REG32(CM_WKUP_CLKSTCTRL) |=
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

void init_all_plls(void)
{
    device_version = device_version_get();
    config_vdd_op_voltage();

    oppMaxIdx = boot_max_opp_get();
    SetVdd1OpVoltage(oppTable[oppMaxIdx].pmicVolt);
    // ccm_driver.init();

    REG32(SOC_WDT_1_REGS + WDT_WSPR) = 0xAAAAu;
    while(REG32(SOC_WDT_1_REGS + WDT_WWPS) != 0x00);

    REG32(SOC_WDT_1_REGS + WDT_WSPR) = 0x5555u;
    while(REG32(SOC_WDT_1_REGS + WDT_WWPS) != 0x00);

    freqMultDDR = DDRPLL_M_DDR3;


    MPUPLLInit(oppTable[oppMaxIdx].pllMult);
    CorePLLInit();
    PerPLLInit();
    DDRPLLInit(freqMultDDR);
    InterfaceClkInit();
    PowerDomainTransitionInit();
    DisplayPLLInit();

    // enable CM
    REG32(CM_WKUP_CONTROL_CLKCTRL) =
            CM_WKUP_CONTROL_CLKCTRL_MODULEMODE_ENABLE;
}

ccm_t ccm_driver = {
    .init = init_all_plls,
};
