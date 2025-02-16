#ifndef KERNEL_BOARD_H
#define KERNEL_BOARD_H

#define EEPROM_SIZE_HEADER         (4)
#define EEPROM_SIZE_BOARD_NAME     (8)
#define EEPROM_SIZE_VERSION        (5)
#define EEPROM_SIZE_SERIAL_NUMBER  (12)


typedef struct {
    void (*init)(void);

    char name[EEPROM_SIZE_BOARD_NAME + 1];
    char version[EEPROM_SIZE_VERSION + 1];
    char serial_number[EEPROM_SIZE_SERIAL_NUMBER + 1];
} board_info_t;

extern board_info_t board_info;

#endif // KERNEL_BOARD_H
