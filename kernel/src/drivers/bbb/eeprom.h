#include <stdint.h>

/****************************************************************************
**                    MACRO DEFINITIONS
****************************************************************************/

#define EEPROM_SIZE_HEADER         (4)
#define EEPROM_SIZE_BOARD_NAME     (8)
#define EEPROM_SIZE_VERSION        (5)
#define EEPROM_SIZE_SERIAL_NUMBER  (12)
#define MAX_DATA                   (EEPROM_SIZE_HEADER + \
		                    EEPROM_SIZE_BOARD_NAME + \
		                    EEPROM_SIZE_VERSION + \
		                    EEPROM_SIZE_SERIAL_NUMBER)

#define BOARD_UNKNOWN              (0xFF)
#define BOARD_VER_UNKNOWN          (0xFE)

/****************************************************************************
**                  STRUCTURE DECLARATION
****************************************************************************/

struct EEPROMData {
                 unsigned char header[EEPROM_SIZE_HEADER];
                 unsigned char boardName[EEPROM_SIZE_BOARD_NAME];
                 unsigned char version[EEPROM_SIZE_VERSION];
                 unsigned char serialNumber[EEPROM_SIZE_SERIAL_NUMBER];
            };

typedef struct EEPROMData BOARDINFO;
// extern unsigned char boardVersion[];
// extern unsigned char boardName[];
// extern unsigned int boardId;

void eeprom_init(uint32_t slave_addr);
void eeprom_read(char* data, uint32_t len, uint32_t offset);
