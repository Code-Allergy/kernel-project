#include <kernel/printk.h>
#include <kernel/dram.h>
#include <stdint.h>

#include "ccm.h"
#include "mmc.h"
#include "dram.h"
#include "cm_per.h"
#include "board.h"

extern uint32_t device_version;

#define REG32(addr)    (*(volatile unsigned int *)(addr))

void EMIFInit(void) {
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

static void BBBlack_DDR3PhyInit(void) {
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

void BBBlack_DDR3Init(void) {
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

void dram_init(void) {
    EMIFInit();
    BBBlack_DDR3Init();
}


dram_driver_t dram_driver = {
    .start = 0x80000000,
    .size = 0x20000000,
    .init = dram_init,
    // void (*cleanup)(void);
    // void (*suspend)(void);
    // void (*resume)(void);
    // int (*get_size)(unsigned long *size);
    // int (*get_info)(void *info, size_t size);
};
