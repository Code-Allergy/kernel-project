#include <kernel/board.h>
#include <stdint.h>

// read actual virtual eeprom later
#define QEMU_BOARD_NAME "QEMU-CB"
#define QEMU_BOARD_VERSION "0.1"
#define QEMU_BOARD_SERIAL "QEMU-0001"

void init_qemu_board_info(void) {
    for(uint32_t i = 0; i < sizeof(QEMU_BOARD_NAME) - 1; i++) {
        board_info.name[i] = QEMU_BOARD_NAME[i];
    }
    board_info.name[sizeof(QEMU_BOARD_NAME) - 1] = '\0';

    for(uint32_t i = 0; i < sizeof(QEMU_BOARD_VERSION) - 1; i++) {
        board_info.version[i] = QEMU_BOARD_VERSION[i];
    }
    board_info.version[sizeof(QEMU_BOARD_VERSION) - 1] = '\0';

    for(uint32_t i = 0; i < sizeof(QEMU_BOARD_SERIAL) - 1; i++) {
        board_info.serial_number[i] = QEMU_BOARD_SERIAL[i];
    }
    board_info.serial_number[sizeof(QEMU_BOARD_SERIAL) - 1] = '\0';
}


board_info_t board_info = {
    .init = init_qemu_board_info,
};
