#include <kernel/mmc.h>
#include <kernel/printk.h>
#include <stdint.h>
#include "mmc.h"
#include "kernel/sd.h"

static void mmc_init(void) {
    printk("Skipping setting clocks on BBB for now!\n");
}

void set_init_clock(void) {
    uint32_t target_freq_khz = 40;  // 80 kHz ensures 80 clocks = 1ms

    uint32_t sysctl = MMC0_BASE + MMC_SYSCTL;
    uint32_t val = REG32(sysctl);

    // Disable clock
    val &= ~(1 << 2);
    REG32(sysctl) = val;

    // Set divider (96MHz / 80kHz / 2) - 1 = 599
    uint32_t div = (48000 / (2 * target_freq_khz)) - 1;

    // Set divider and enable internal clock
    val &= ~(0x3FF << 6);  // Clear existing divider bits
    val |= (div << 6);     // Set new divider
    val |= (1 << 0);       // Enable internal clock (ICE)
    REG32(sysctl) = val;

    // Wait for clock to stabilize
    uint32_t timeout = 100000;
    while (!(REG32(sysctl) & (1 << 1))) {
        if (--timeout == 0) {
            printk("Timeout waiting for clock stabilization\n");
            return;
        }
    }

    // Enable clock to card
    val |= (1 << 2);
    REG32(sysctl) = val;

    printk("Init clock set to %d kHz (div=%d)\n", target_freq_khz, div);
}

// Function to configure pin muxing
void configure_mmc_pins(void) {
    // Configure pins for MMC0 - Mode 0 with pullup
    const uint32_t pin_config = (0 | (1 << 4) | (1 << 5));  // Mode 0, pullup enabled, receiver enabled

    REG32(CONF_MMC0_DAT3) = pin_config;
    REG32(CONF_MMC0_DAT2) = pin_config;
    REG32(CONF_MMC0_DAT1) = pin_config;
    REG32(CONF_MMC0_DAT0) = pin_config;
    REG32(CONF_MMC0_CLK) = (0 << 0) | (1 << 4) | (1 << 5) | (1 << 6);
    REG32(CONF_MMC0_CMD)  = pin_config;
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

// Initialize SD card
void sd_card_init(void) {
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


    // 3. Reset MMC controller
    REG32(MMC0_BASE + MMC_SYSCONFIG) = 0x2;
    timeout = 100000;
    while ((REG32(MMC0_BASE + MMC_SYSSTATUS) & 0x1) == 0) {
        if (--timeout == 0) {
            printk("Timeout waiting for MMC controller reset\n");
            return;
        }
    }

    // reset all
    REG32(MMC0_BASE + MMC_SYSCTL) |= (24 << 1);
    timeout = 100000;
    while (((REG32(MMC0_BASE + MMC_SYSCTL) & (24 << 1)) != 0)) {
        if (--timeout == 0) {
            printk("Timeout waiting for MMC controller reset all\n");
            return;
        }
    }


    printk("MMC controller reset complete\n");

    // disable mmc interrupts
    // REG32(MMC0_BASE + MMC_IE) = 0;
    // REG32(MMC0_BASE + MMC_ISE) = 0;

    // REG32(MMC0_BASE + MMC_STAT) = 0xFFFFFFFF;

    // set capabilities
    // REG32(MMC0_BASE + MMC_CAPA) |= VS30_3V0SUP;

    REG32(MMC0_BASE + MMC_HCTL) = DTW_1_BITMODE | SDBP_PWROFF | SDVS_3V0;
    REG32(MMC0_BASE + MMC_CAPA) |= VS30_3V0SUP | VS18_1V8SUP;


    REG32(MMC0_BASE + MMC_CON) |= CTPL_MMC_SD | WPP_ACTIVEHIGH | CDP_ACTIVEHIGH |
		MIT_CTO | DW8_1_4BITMODE | MODE_FUNC | STR_BLOCK |
		HR_NOHOSTRESP | INIT_NOINIT | NOOPENDRAIN;

    uint32_t dsor = 240;
   	mmc_reg_out(MMC0_BASE + MMC_SYSCTL, (ICE_MASK | DTO_MASK | CEN_MASK),
		(ICE_STOP | DTO_15THDTO));
	mmc_reg_out(MMC0_BASE + MMC_SYSCTL, ICE_MASK | CLKD_MASK,
		(dsor << CLKD_OFFSET) | ICE_OSCILLATE);

    timeout = 100000;
    while (((REG32(MMC0_BASE + MMC_SYSCTL) & ICS_MASK) == ICS_NOTREADY)) {
        if (--timeout == 0) {
            printk("Timeout waiting for MMC controller reset all\n");
            return;
        }
    }

    REG32(MMC0_BASE + MMC_SYSCTL) |= CEN_ENABLE;
    REG32(MMC0_BASE + MMC_HCTL) |= SDBP_PWRON;

    // set init freq


    // 4. Set initial clock to 400KHz (SD card identification mode)

    // 5. Power on the controller
    // printk("SD capabilities %p\n", REG32(MMC0_BASE + MMC_CAPA));

    // set_init_clock();
    // printk("Clock set for initialization\n");

    // uint32_t hctl = REG32(MMC0_BASE + MMC_HCTL);
    // hctl &= ~(0x7 << 9);      // Clear existing voltage bits
    // hctl |= (0x6 << 9);       // Set to 3.0V (0x6)
    // REG32(MMC0_BASE + MMC_HCTL) = hctl;
    // REG32(MMC0_BASE + MMC_HCTL) |= (1 << 8);  // Set SD bus power on

    // Verify power settings took effect
    // if ((REG32(MMC0_BASE + MMC_HCTL) & (1 << 8)) == 0) {
    //     printk("Failed to power on MMC/SD controller\n");
    //     return;
    // }
    // printk("MMC controller powered on\n");

    REG32(MMC0_BASE + MMC_CON) |= INIT_INITSTREAM;

    // Wait for command complete
    // timeout = 1000000;
    // while (!(REG32(MMC0_BASE + MMC_STAT) & 0x1)) {
    //     if (--timeout == 0) {
    //         printk("Timeout waiting for initialization sequence\n");
    //         return;
    //     }
    // }
    printk("Initialization sequence complete\n");

    // // Clear the CC bit in STAT
    // REG32(MMC0_BASE + MMC_STAT) = 0x1;

    // // Clear INIT bit after initialization sequence
    // REG32(MMC0_BASE + MMC_CON) &= ~(1 << 1);  // Clear INIT bit

    // Now switch to 400 kHz for card identification
    set_mmc_clock(400);
    printk("Clock set to 400 kHz for card identification\n");

    // 6. SD Card Initialization sequence
    // Send CMD0 (GO_IDLE_STATE)
    // mmc_send_cmd(CMD0, 0);
    REG32(MMC0_BASE + MMC_CMD) = CMD0;
    timeout = 100000;
    while (!(REG32(MMC0_BASE + MMC_STAT) & CC_MASK)) {
        if (--timeout == 0) {
            printk("Timeout waiting for MMC CMD 0\n");
            return;
        }
    }
    printk("Done!\n");
    // Send CMD8 (SEND_IF_COND) - Required for SD 2.0
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

    // 7. Set final clock speed (25MHz for standard cards)
    set_mmc_clock(25000);
}

// Example block read function
int sd_read_block(uint32_t block_addr, uint8_t* buffer) {
    // Set block length
    REG32(MMC0_BASE + MMC_BLK) = 512;  // 512 bytes

    // Send read command
    mmc_send_cmd(0x00000011, block_addr);  // READ_SINGLE_BLOCK

    // Read data
    volatile uint32_t* data_reg = (volatile uint32_t*)(MMC0_BASE + MMC_DATA);
    for (int i = 0; i < 512/4; i++) {
        while (!(REG32(MMC0_BASE + MMC_PSTATE) & (1 << 11)));  // Wait for buffer ready
        ((uint32_t*)buffer)[i] = *data_reg;
    }

    return 0;
}


mmc_driver_t mmc_driver = {
    .init = sd_card_init,
};
