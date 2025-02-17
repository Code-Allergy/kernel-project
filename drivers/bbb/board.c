#include <kernel/board.h>
#include <kernel/string.h>
#include <kernel/printk.h>
#include "eeprom.h"
#include "i2c.h"

#define I2C_SLAVE_ADDR                      (0x50)


void init_bbb_board_info(void) {
    BOARDINFO raw_board_info;

    eeprom_init(I2C_SLAVE_ADDR);
    eeprom_read((void*)&raw_board_info, MAX_DATA, 0);
    printk("Done reading\n");
    for(int i = 0; i < EEPROM_SIZE_BOARD_NAME; i++) {
        board_info.name[i] = raw_board_info.boardName[i];
    }
    board_info.name[EEPROM_SIZE_BOARD_NAME] = '\0';

    for(int i = 0; i < EEPROM_SIZE_SERIAL_NUMBER; i++) {
        board_info.serial_number[i] = raw_board_info.serialNumber[i];
    }
    board_info.serial_number[EEPROM_SIZE_SERIAL_NUMBER] = '\0';

    for(int i = 0; i < EEPROM_SIZE_VERSION; i++) {
        board_info.version[i] = raw_board_info.version[i];
    }
    board_info.version[EEPROM_SIZE_VERSION] = '\0';
}



board_info_t board_info = {
    .init = init_bbb_board_info,
};
