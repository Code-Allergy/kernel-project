#include <kernel/printk.h>
#include <kernel/ccm.h>
#include "ccm.h"

/**
 * @brief Configure PLL1 (CPU PLL) with specified parameters
 *
 * @param N     PLL1 multiplier factor (integer division ratio, 1-127)
 * @param M     Pre-divider (1-3)
 * @param P     Post-divider (0=1x, 1=2x, 2=4x, 3=8x)
 *
 * PLL1 Frequency = (24MHz * N * K) / (M * P)
 * (Note: K factor is in PLL1_TUN_REG if needed)
 */
void pll1_configure(uint8_t N, uint8_t M, uint8_t P) {
    // Disable PLL1 temporarily
    CCM->PLL1_CFG_REG &= ~(1 << 31);

    // Configure PLL1 parameters with proper bit shifting
    CCM->PLL1_CFG_REG = (1 << 31) |   // Enable PLL1
                    (0 << 30) |   // Disable bypass
                    (P << 16) |   // P=2 (01)
                    (N << 8) |   // N=31
                    (3 << 4) |    // K=4 (11)
                    (M << 0);     // M=1 (00)

    // Set PLL1 as CPU clock source, wait 8 cycles for stabilization
    CCM->CPU_AHB_APB0_CFG_REG = (CCM->CPU_AHB_APB0_CFG_REG & ~(3 << 16)) | (0x02 << 16);
    asm volatile("nop; nop; nop; nop; nop; nop; nop; nop;");


    CCM->CPU_AHB_APB0_CFG_REG &= ~(0x3F); // Clear dividers (AXI=1, AHB=1, APB0=2)
}

void clock_init(void) {
    /* check if the ccm is emulated in the qemu version
     * CCM->PLL1_CFG_REG == 0x21005000 default value
     * check if ccm is emulated by running `info mtree`
     * and find allwinner-a10-ccm
     */
    if (CCM->PLL1_CFG_REG != 0x21005000) {
        printk("CCM is not emulated\n");
        return;
    }

    // as far as I can tell this does nothing on qemu anyways lol
    CCM->OSC24M_CFG_REG |= (1 << 0);
    pll1_configure(31, 0, 1);
}


ccm_t ccm_driver = {
    .init = clock_init,
};
