#include <kernel/printk.h>
#include <stdint.h>
#include "mmc.h"
#include "ccm.h"
#include "dma.h"
#include "kernel/mmc.h"
#include <kernel/fat32.h>
#include <kernel/sd.h>

// TODO error checking

void mmc_clock_init(void) {
    // 2. Configure SD clock to 400 kHz (24 MHz / 60 = 400 kHz)
    mmc0->clkcr = (60 << 0) |  // CLK_DIV = N-1 (59 for divisor 60)
                 (1 << 16);    // CLK_ENABLE
}

void mmc_init(void) {
    // Reset controller
    mmc0->gctrl |= SD_GCTL_SOFT_RST;
    while (mmc0->gctrl & SD_GCTL_SOFT_RST);

    // Set initialization clock (400 kHz)
    mmc0->clkcr = (59 << 0) | (1 << 16); // 24MHz/(59+1) = 400kHz

    // 1. CMD0: Reset to idle state
    mmc_send_cmd(0, 0);

    // 2. CMD8: Check voltage
    mmc_send_cmd(8, 0x1AA);

    // 3. ACMD41: Initialize card
    // Must send CMD55 before each ACMD41
    uint32_t ocr = 0x40FF8000;  // OCR with HCS (High Capacity Support)
    do {
        mmc_send_cmd(55, 0);    // CMD55 (prefix for ACMD)
        mmc_send_cmd(41, ocr);  // ACMD41
        // return;
    } while (!(mmc0->resp[0] & (1 << 31))); // Wait until ready

    // 4. Send CMD2 (get CID)
    mmc_send_cmd(2, 0);

    // 5. Send CMD3 (get RCA)
    mmc_send_cmd(3, 0);
    uint32_t rca = (mmc0->resp[0] >> 16) & 0xFFFF; // Extract RCA

    // 6. Send CMD7 (select card) to enter transfer state
    mmc_send_cmd(7, rca << 16); // Argument: [31:16] = RCA

    // 4. Switch to high speed (25 MHz)
    mmc0->clkcr = (0 << 0) | (1 << 16); // 24MHz/2 = 12MHz
    mmc0->idie = (1 << 4) | (1 << 3);
    mmc0->blksz = 512;
}

int mmc_read_sector(uint32_t sector, uint8_t *buffer) {
    // Send read command (CMD17)
    // SD card uses byte addressing in QEMU
    mmc_send_cmd(17, sector * 512);

    // Read data from FIFO
    uint32_t *ptr = (uint32_t *)buffer;
    for (int i = 0; i < 128; i++) { // 512 bytes / 4 bytes per word
        *ptr++ = mmc0->fifo;
    }

    return 0;
}

int mmc_send_cmd(uint32_t cmd, uint32_t arg) {
    uint32_t cmdr = (cmd & 0x3F);
    switch (cmd) {
        case CMD0:   // Reset
        case CMD8:   // Voltage check
            cmdr |= (1 << 6);  // Short response
            break;
        case CMD2:   // CID
        case CMD9:   // CSD
            cmdr |= (1 << 7);  // Long response
            break;
        default:
            cmdr |= (1 << 6);  // Default to short response
            break;
    }

    mmc0->arg = arg;        // Set argument

    // if (cmd == 0 || cmd == 8) {
    //     // CMD0: No response
    //     cmdr &= ~((1 << 7) | (1 << 6));
    // } else if (cmd == 42) {
    //     cmdr |= (1 << 7); // RESP_EXPIRE + RESPONSE_LONG
    // } else if (cmd == 55 || cmd == 41 || cmd == 16 || cmd == 17 | cmd == 58) {
    //     cmdr |= (1 << 6); // RESPONSE only
    // }

    // Trigger command execution
    mmc0->cmd = cmdr | (1 << 31); // CMDR_LOAD

    // Log the command and response
    // printk("Command: %d, Argument: 0x%x\n", cmd, arg);
    // // Wait a bit
    // for (int i = 0; i < 4; i++) printk("Response: %p\n", mmc0->resp[i]);

    // Wait for completion (with timeout)
    while ((!(mmc0->rint & (SD_RISR_CMD_COMPLETE | SD_RISR_NO_RESPONSE))));

    return (mmc0->rint & SD_RISR_CMD_COMPLETE) ? 0 : -1;
}


// TODO broken stull
// int mmc_read_sectors(uint32_t sector, uint8_t *buffer, uint32_t count) {
//     dma_desc_t desc = {
//         .status = DESC_STATUS_HOLD | DESC_STATUS_LAST,  // Active descriptor
//         .size = count * 512,                            // Total bytes to read
//         .addr = (uint32_t)buffer,                       // Physical buffer address
//         .next = 0                                        // No chaining
//     };
//     printk("Sector=%d, Buffer=%p, Count=%d\n", sector, buffer, count);

//     dma_memory_write(DESC_PHYS_ADDR, &desc, sizeof(desc));

//     // configure DMA
//     mmc0->dlba = DESC_PHYS_ADDR;       // Set descriptor base address
//     mmc0->dmac = SD_GCTL_DMA_ENB;      // Enable DMA

//     // 4. Send CMD18 with proper flags
//     mmc0->arg = sector;                // SDSC uses byte addressing, SDHC uses block
//     mmc0->cmd = (18 & 0x3F) |          // CMD18 in command index
//                SD_CMDR_LOAD |         // Execute command
//                SD_CMDR_DATA |         // Data transfer expected
//                SD_CMDR_AUTOSTOP |     // Auto-send CMD12 after transfer
//                SD_CMDR_RESPONSE;      // Expect response


//     // 4. Wait for DMA completion
//     while (!(mmc0->idst & (SD_IDST_INT_SUMMARY | SD_IDST_RECEIVE_IRQ))) {
//         // Add timeout handling in real code
//     }

//     // reset
//     mmc0->idst = SD_IDST_INT_SUMMARY | SD_IDST_RECEIVE_IRQ;
//     return 0; // Success
// }



const fat32_diskio_t mmc_fat32_diskio = {
    .read_sector = mmc_read_sector,
    // .read_sectors = mmc_read_sectors
};

mmc_driver_t mmc_driver = {
    .init = mmc_init,
};
