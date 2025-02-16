#include <stdint.h>

#include "i2c.h"
#include "ccm.h"
#include "mmc.h"


#define REG32(x) (*((volatile uint32_t *)(x)))

void i2c_master_disable(uint32_t base_addr) {
    REG32(base_addr + I2C_CON) &= ~I2C_CON_I2C_EN;
}
void i2c_soft_reset(uint32_t base_addr) {
    REG32(base_addr + I2C_SYSC) |= I2C_SYSC_SRST;
}

void i2c_master_init_clock(uint32_t base_addr, uint32_t sys_clock,
    uint32_t internal_clock, uint32_t output_clock) {
        unsigned int prescalar;
        unsigned int divider;

        /* Calculate prescalar value */
        prescalar = (sys_clock / internal_clock) - 1;

        REG32(base_addr + I2C_PSC) = prescalar;

        divider = internal_clock/output_clock;

        divider = divider / 2;

        REG32(base_addr + I2C_SCLL) = divider - 7;

        REG32(base_addr + I2C_SCLH) = divider - 5;
}

void i2c_master_slave_addr_set(uint32_t base_addr, uint32_t slave_addr) {
    REG32(base_addr + I2C_SA) = slave_addr;
}

void i2c_master_int_disable_ex(uint32_t base_addr, uint32_t int_flag) {
    REG32(base_addr + I2C_IRQENABLE_CLR) = int_flag;
}

void i2c_master_enable(uint32_t base_addr) {
    REG32(base_addr + I2C_CON) |= I2C_CON_I2C_EN;
}

void i2c_set_data_count(uint32_t base_addr, uint32_t count) {
    REG32(base_addr + I2C_CNT) = count;
}

void i2c_master_int_clear_ex(uint32_t base_addr, uint32_t int_flag) {
    REG32(base_addr + I2C_IRQSTATUS) = int_flag;
}

void i2c_master_control(uint32_t base_addr, uint32_t cmd) {
    REG32(base_addr + I2C_CON) = cmd | I2C_CON_I2C_EN;
}

void i2c_master_start(uint32_t base_addr) {
    REG32(base_addr + I2C_CON) |= I2C_CON_STT;
}

uint32_t i2c_master_bus_busy(uint32_t base_addr) {
    uint32_t status;

    if (REG32(base_addr + I2C_IRQSTATUS_RAW) & I2C_IRQSTATUS_RAW_BB) {
        status = 1;
    } else {
        status = 0;
    }

    return status;
}

void i2c_master_data_put(uint32_t base_addr, uint8_t data) {
    REG32(base_addr + I2C_DATA) = data;
}

uint8_t i2c_master_data_get(uint32_t base_addr) {
    return (uint8_t) REG32(base_addr + I2C_DATA);
}

uint32_t i2c_master_int_raw_status_ex(uint32_t base_addr, uint32_t int_flag) {
        return ((REG32(base_addr + I2C_IRQSTATUS_RAW)) & (int_flag));
}

uint32_t i2c_master_int_raw_status(uint32_t base_addr) {
    return ((REG32(base_addr + I2C_IRQSTATUS_RAW)));
}

void i2c_master_stop(uint32_t base_addr) {
    REG32(base_addr + I2C_CON) |= I2C_CON_STP;
}

void i2c_auto_idle_disable(uint32_t base_addr) {
    REG32(base_addr + I2C_SYSC) &= ~I2C_SYSC_AUTOIDLE;
}

uint32_t i2c_system_status_get(uint32_t base_addr) {
    return (REG32(base_addr + I2C_SYSS) & I2C_SYSS_RDONE);
}

void i2c_pin_mux_setup(uint32_t instance)
{
    if(instance == 0)
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
    else if(instance == 1)
    {
                               /* I2C_SCLK */
         REG32(CONTROL_MODULE_BASE + CONTROL_CONF_SPI0_D1)  =
              (CONTROL_CONF_SPI0_D1_CONF_SPI0_D1_PUTYPESEL |
               CONTROL_CONF_SPI0_D1_CONF_SPI0_D1_RXACTIVE  |
               CONTROL_CONF_SPI0_D1_CONF_SPI0_D1_SLEWCTRL  |
               CONTROL_CONF_MUXMODE(2));
                              /* I2C_SDA */
         REG32(CONTROL_MODULE_BASE + CONTROL_CONF_SPI0_CS0) =
              (CONTROL_CONF_SPI0_CS0_CONF_SPI0_CS0_PUTYPESEL |
               CONTROL_CONF_SPI0_CS0_CONF_SPI0_CS0_RXACTIVE  |
               CONTROL_CONF_SPI0_D1_CONF_SPI0_D1_SLEWCTRL    |
               CONTROL_CONF_MUXMODE(2));
    }

}

void i2c_init_clocks(void) {
    REG32(CM_PER_BASE + CM_PER_L3_CLKCTRL) |= CM_PER_L3_CLKCTRL_MODULEMODE_ENABLE;
    while ((CM_PER_L3_CLKCTRL_MODULEMODE_ENABLE != REG32(CM_PER_BASE + CM_PER_L3_CLKCTRL)) & CM_PER_L3_CLKCTRL_MODULEMODE);

    REG32(CM_PER_BASE + CM_PER_L3_INSTR_CLKCTRL) |= CM_PER_L3_INSTR_CLKCTRL_MODULEMODE_ENABLE;
    while(CM_PER_L3_INSTR_CLKCTRL_MODULEMODE_ENABLE !=
          (REG32(CM_PER_BASE + CM_PER_L3_INSTR_CLKCTRL) &
           CM_PER_L3_INSTR_CLKCTRL_MODULEMODE));

    REG32(CM_PER_BASE + CM_PER_L3_CLKSTCTRL) |= CM_PER_L3_CLKSTCTRL_CLKTRCTRL_SW_WKUP;
    while(CM_PER_L3_CLKSTCTRL_CLKTRCTRL_SW_WKUP !=
          (REG32(CM_PER_BASE + CM_PER_L3_CLKSTCTRL) &
           CM_PER_L3_CLKSTCTRL_CLKTRCTRL));

    REG32(CM_PER_BASE + CM_PER_OCPWP_L3_CLKSTCTRL) |= CM_PER_OCPWP_L3_CLKSTCTRL_CLKTRCTRL_SW_WKUP;
    while(CM_PER_OCPWP_L3_CLKSTCTRL_CLKTRCTRL_SW_WKUP !=
          (REG32(CM_PER_BASE + CM_PER_OCPWP_L3_CLKSTCTRL) &
           CM_PER_OCPWP_L3_CLKSTCTRL_CLKTRCTRL));

    REG32(CM_PER_BASE + CM_PER_L3S_CLKSTCTRL) |= CM_PER_L3S_CLKSTCTRL_CLKTRCTRL_SW_WKUP;
    while(CM_PER_L3S_CLKSTCTRL_CLKTRCTRL_SW_WKUP !=
          (REG32(CM_PER_BASE + CM_PER_L3S_CLKSTCTRL) &
           CM_PER_L3S_CLKSTCTRL_CLKTRCTRL));

    while((CM_PER_L3_CLKCTRL_IDLEST_FUNC << CM_PER_L3_CLKCTRL_IDLEST_SHIFT)!=
          (REG32(CM_PER_BASE + CM_PER_L3_CLKCTRL) &
           CM_PER_L3_CLKCTRL_IDLEST));

    while((CM_PER_L3_INSTR_CLKCTRL_IDLEST_FUNC <<
           CM_PER_L3_INSTR_CLKCTRL_IDLEST_SHIFT)!=
          (REG32(CM_PER_BASE + CM_PER_L3_INSTR_CLKCTRL) &
           CM_PER_L3_INSTR_CLKCTRL_IDLEST));

    while(CM_PER_L3_CLKSTCTRL_CLKACTIVITY_L3_GCLK !=
          (REG32(CM_PER_BASE + CM_PER_L3_CLKSTCTRL) &
           CM_PER_L3_CLKSTCTRL_CLKACTIVITY_L3_GCLK));

    while(CM_PER_OCPWP_L3_CLKSTCTRL_CLKACTIVITY_OCPWP_L3_GCLK !=
          (REG32(CM_PER_BASE + CM_PER_OCPWP_L3_CLKSTCTRL) &
           CM_PER_OCPWP_L3_CLKSTCTRL_CLKACTIVITY_OCPWP_L3_GCLK));

    while(CM_PER_L3S_CLKSTCTRL_CLKACTIVITY_L3S_GCLK !=
          (REG32(CM_PER_BASE + CM_PER_L3S_CLKSTCTRL) &
          CM_PER_L3S_CLKSTCTRL_CLKACTIVITY_L3S_GCLK));

    /* Configuring registers related to Wake-Up region. */

    /* Writing to MODULEMODE field of CM_WKUP_CONTROL_CLKCTRL register. */
    REG32(CM_WKUP_BASE + CM_WKUP_CONTROL_CLKCTRL) |=
          CM_WKUP_CONTROL_CLKCTRL_MODULEMODE_ENABLE;

    /* Waiting for MODULEMODE field to reflect the written value. */
    while(CM_WKUP_CONTROL_CLKCTRL_MODULEMODE_ENABLE !=
          (REG32(CM_WKUP_BASE + CM_WKUP_CONTROL_CLKCTRL) &
           CM_WKUP_CONTROL_CLKCTRL_MODULEMODE));

    /* Writing to CLKTRCTRL field of CM_PER_L3S_CLKSTCTRL register. */
    REG32(CM_WKUP_BASE + CM_WKUP_CLKSTCTRL) |=
          CM_WKUP_CLKSTCTRL_CLKTRCTRL_SW_WKUP;

    /*Waiting for CLKTRCTRL field to reflect the written value. */
    while(CM_WKUP_CLKSTCTRL_CLKTRCTRL_SW_WKUP !=
          (REG32(CM_WKUP_BASE + CM_WKUP_CLKSTCTRL) &
           CM_WKUP_CLKSTCTRL_CLKTRCTRL));

    /* Writing to CLKTRCTRL field of CM_L3_AON_CLKSTCTRL register. */
    REG32(CM_WKUP_BASE + CM_WKUP_CM_L3_AON_CLKSTCTRL) |=
          CM_WKUP_CM_L3_AON_CLKSTCTRL_CLKTRCTRL_SW_WKUP;

    /*Waiting for CLKTRCTRL field to reflect the written value. */
    while(CM_WKUP_CM_L3_AON_CLKSTCTRL_CLKTRCTRL_SW_WKUP !=
          (REG32(CM_WKUP_BASE + CM_WKUP_CM_L3_AON_CLKSTCTRL) &
           CM_WKUP_CM_L3_AON_CLKSTCTRL_CLKTRCTRL));

    /* Writing to MODULEMODE field of CM_WKUP_I2C0_CLKCTRL register. */
    REG32(CM_WKUP_BASE + CM_WKUP_I2C0_CLKCTRL) |=
          CM_WKUP_I2C0_CLKCTRL_MODULEMODE_ENABLE;

    /* Waiting for MODULEMODE field to reflect the written value. */
    while(CM_WKUP_I2C0_CLKCTRL_MODULEMODE_ENABLE !=
          (REG32(CM_WKUP_BASE + CM_WKUP_I2C0_CLKCTRL) &
           CM_WKUP_I2C0_CLKCTRL_MODULEMODE));

    /* Verifying if the other bits are set to required settings. */

    /*
    ** Waiting for IDLEST field in CM_WKUP_CONTROL_CLKCTRL register to attain
    ** desired value.
    */
    while((CM_WKUP_CONTROL_CLKCTRL_IDLEST_FUNC <<
           CM_WKUP_CONTROL_CLKCTRL_IDLEST_SHIFT) !=
          (REG32(CM_WKUP_BASE + CM_WKUP_CONTROL_CLKCTRL) &
           CM_WKUP_CONTROL_CLKCTRL_IDLEST));

    /*
    ** Waiting for CLKACTIVITY_L3_AON_GCLK field in CM_L3_AON_CLKSTCTRL
    ** register to attain desired value.
    */
    while(CM_WKUP_CM_L3_AON_CLKSTCTRL_CLKACTIVITY_L3_AON_GCLK !=
          (REG32(CM_WKUP_BASE + CM_WKUP_CM_L3_AON_CLKSTCTRL) &
           CM_WKUP_CM_L3_AON_CLKSTCTRL_CLKACTIVITY_L3_AON_GCLK));

    /*
    ** Waiting for IDLEST field in CM_WKUP_L4WKUP_CLKCTRL register to attain
    ** desired value.
    */
    while((CM_WKUP_L4WKUP_CLKCTRL_IDLEST_FUNC <<
           CM_WKUP_L4WKUP_CLKCTRL_IDLEST_SHIFT) !=
          (REG32(CM_WKUP_BASE + CM_WKUP_L4WKUP_CLKCTRL) &
           CM_WKUP_L4WKUP_CLKCTRL_IDLEST));

    /*
    ** Waiting for CLKACTIVITY_L4_WKUP_GCLK field in CM_WKUP_CLKSTCTRL register
    ** to attain desired value.
    */
    while(CM_WKUP_CLKSTCTRL_CLKACTIVITY_L4_WKUP_GCLK !=
          (REG32(CM_WKUP_BASE + CM_WKUP_CLKSTCTRL) &
           CM_WKUP_CLKSTCTRL_CLKACTIVITY_L4_WKUP_GCLK));

    /*
    ** Waiting for CLKACTIVITY_L4_WKUP_AON_GCLK field in CM_L4_WKUP_AON_CLKSTCTRL
    ** register to attain desired value.
    */
    while(CM_WKUP_CM_L4_WKUP_AON_CLKSTCTRL_CLKACTIVITY_L4_WKUP_AON_GCLK !=
          (REG32(CM_WKUP_BASE + CM_WKUP_CM_L4_WKUP_AON_CLKSTCTRL) &
           CM_WKUP_CM_L4_WKUP_AON_CLKSTCTRL_CLKACTIVITY_L4_WKUP_AON_GCLK));

    /*
    ** Waiting for CLKACTIVITY_I2C0_GFCLK field in CM_WKUP_CLKSTCTRL
    ** register to attain desired value.
    */
    while(CM_WKUP_CLKSTCTRL_CLKACTIVITY_I2C0_GFCLK !=
          (REG32(CM_WKUP_BASE + CM_WKUP_CLKSTCTRL) &
           CM_WKUP_CLKSTCTRL_CLKACTIVITY_I2C0_GFCLK));

    /*
    ** Waiting for IDLEST field in CM_WKUP_I2C0_CLKCTRL register to attain
    ** desired value.
    */
    while((CM_WKUP_I2C0_CLKCTRL_IDLEST_FUNC <<
           CM_WKUP_I2C0_CLKCTRL_IDLEST_SHIFT) !=
          (REG32(CM_WKUP_BASE + CM_WKUP_I2C0_CLKCTRL) &
           CM_WKUP_I2C0_CLKCTRL_IDLEST));
}
