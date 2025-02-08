#ifndef CCM_H
#define CCM_H

#include <stdint.h>

#define CCM_BASE 0x01c20000

/* reference */
/* https://linux-sunxi.org/Clock_Control_Module */
typedef struct {
    volatile uint32_t PLL1_CFG_REG;      // 0x0000
    volatile uint32_t PLL1_TUN_REG;      // 0x0004
    volatile uint32_t PLL2_CFG_REG;      // 0x0008
    volatile uint32_t PLL2_TUN_REG;      // 0x000C
    volatile uint32_t PLL3_CFG_REG;      // 0x0010
    uint32_t          __reserved0;       // 0x0014
    volatile uint32_t PLL4_CFG_REG;      // 0x0018
    uint32_t          __reserved1;       // 0x001C
    volatile uint32_t PLL5_CFG_REG;      // 0x0020
    volatile uint32_t PLL5_TUN_REG;      // 0x0024
    volatile uint32_t PLL6_CFG_REG;      // 0x0028
    volatile uint32_t PLL6_TUN_REG;      // 0x002C
    volatile uint32_t PLL7_CFG_REG;      // 0x0030
    uint32_t          __reserved2;       // 0x0034
    volatile uint32_t PLL1_TUN2_REG;     // 0x0038
    volatile uint32_t PLL5_TUN2_REG;     // 0x003C

    uint32_t          __reserved3[3];    // 0x0040-0x0048

    volatile uint32_t CCM_PLL_LOCK_DBG;  //
    volatile uint32_t OSC24M_CFG_REG;    // 0x0050
    volatile uint32_t CPU_AHB_APB0_CFG_REG; // 0x0054
    volatile uint32_t APB1_CLK_DIV_REG;  // 0x0058
    volatile uint32_t AXI_GATING_REG;    // 0x005C
    volatile uint32_t AHB_GATING_REG0;   // 0x0060
    volatile uint32_t AHB_GATING_REG1;   // 0x0064
    volatile uint32_t APB0_GATING_REG;   // 0x0068
    volatile uint32_t APB1_GATING_REG;   // 0x006C

    uint32_t          __reserved4[4];    // 0x0070-0x007C

    volatile uint32_t NAND_SCLK_CFG_REG; // 0x0080
    uint32_t          reserved5;         // 0x0084
    volatile uint32_t SD0_CLK_REG;       // 0x0088
    volatile uint32_t SD1_CLK_REG;       // 0x008C
    volatile uint32_t SD2_CLK_REG;       // 0x0090
    volatile uint32_t SD3_CLK_REG;       // 0x0094
    volatile uint32_t TS_CLK_REG;        // 0x0098
    volatile uint32_t SS_CLK_REG;        // 0x009C
    volatile uint32_t SPI0_CLK_REG;      // 0x00A0
    volatile uint32_t SPI1_CLK_REG;      // 0x00A4
    volatile uint32_t SPI2_CLK_REG;      // 0x00A8
    uint32_t          __reserved6;       // 0x00AC
    volatile uint32_t IR0_CLK_REG;       // 0x00B0
    volatile uint32_t IR1_CLK_REG;       // 0x00B4
    volatile uint32_t IIS_CLK_REG;       // 0x00B8
    volatile uint32_t AC97_CLK_REG;      // 0x00BC
    volatile uint32_t KEYPAD_CLK_REG;    // 0x00C4
    uint32_t          __reserved7;       // 0x00C8
    volatile uint32_t USB_CLK_REG;       // 0x00CC
    uint32_t          __reserved8;       // 0x00D0
    volatile uint32_t SPI3_CLK_REG;      // 0x00D4

    uint32_t          __reserved9[11];    // 0x00D8-0x00FC
    volatile uint32_t DRAM_CLK_REG;       // 0x0100
    volatile uint32_t BE0_SCLK_CFG_REG;  // 0x0104
    volatile uint32_t BE1_SCLK_CFG_REG;  // 0x0108
    volatile uint32_t FE0_CLK_REG;       // 0x010C
    volatile uint32_t FE1_CLK_REG;       // 0x0110
    volatile uint32_t MP_CLK_REG;        // 0x0114
    volatile uint32_t LCD0_CH0_CLK_REG;  // 0x0118
    volatile uint32_t LCD1_CH0_CLK_REG;  // 0x011C
    volatile uint32_t CSI_ISP_CLK_REG;   // 0x0120
    volatile uint32_t TVD_CLK_REG;       // 0x0128
    volatile uint32_t LCD0_CH1_CLK_REG;  // 0x012C
    volatile uint32_t LCD1_CH1_CLK_REG;  // 0x0130
    volatile uint32_t CSI0_CLK_REG;      // 0x0134
    volatile uint32_t CSI1_CLK_REG;      // 0x0138
    volatile uint32_t VE_CLK_REG;        // 0x013C
    volatile uint32_t AUDIO_CODEC_CLK_REG; // 0x0140
    volatile uint32_t AVS_CLK_REG;       // 0x0144
    volatile uint32_t ACE_CLK_REG;       // 0x0148
    volatile uint32_t LVDS_CLK_REG;      // 0x014C
    volatile uint32_t HDMI_CLK_REG;      // 0x0150
} CCM_Type;

#define CCM ((CCM_Type *) CCM_BASE)

#endif
