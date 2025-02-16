#include <stdint.h>


#include "eeprom.h"
#include "mmc.h"
#include "i2c.h"

// should be cm.h
#define CONTROL_CONF_I2C0_SDA   (0x988)
#define CONTROL_CONF_I2C0_SCL   (0x98c)
/* CONF_I2C0_SDA */
#define CONTROL_CONF_I2C0_SDA_CONF_I2C0_SDA_MMODE   (0x00000007u)
#define CONTROL_CONF_I2C0_SDA_CONF_I2C0_SDA_MMODE_SHIFT   (0x00000000u)

#define CONTROL_CONF_I2C0_SDA_CONF_I2C0_SDA_PUDEN   (0x00000008u)
#define CONTROL_CONF_I2C0_SDA_CONF_I2C0_SDA_PUDEN_SHIFT   (0x00000003u)

#define CONTROL_CONF_I2C0_SDA_CONF_I2C0_SDA_PUTYPESEL   (0x00000010u)
#define CONTROL_CONF_I2C0_SDA_CONF_I2C0_SDA_PUTYPESEL_SHIFT   (0x00000004u)

#define CONTROL_CONF_I2C0_SDA_CONF_I2C0_SDA_RSVD   (0x000FFF80u)
#define CONTROL_CONF_I2C0_SDA_CONF_I2C0_SDA_RSVD_SHIFT   (0x00000007u)

#define CONTROL_CONF_I2C0_SDA_CONF_I2C0_SDA_RXACTIVE   (0x00000020u)
#define CONTROL_CONF_I2C0_SDA_CONF_I2C0_SDA_RXACTIVE_SHIFT   (0x00000005u)

#define CONTROL_CONF_I2C0_SDA_CONF_I2C0_SDA_SLEWCTRL   (0x00000040u)
#define CONTROL_CONF_I2C0_SDA_CONF_I2C0_SDA_SLEWCTRL_SHIFT   (0x00000006u

/* CONF_I2C0_SCL */
#define CONTROL_CONF_I2C0_SCL_CONF_I2C0_SCL_MMODE   (0x00000007u)
#define CONTROL_CONF_I2C0_SCL_CONF_I2C0_SCL_MMODE_SHIFT   (0x00000000u)

#define CONTROL_CONF_I2C0_SCL_CONF_I2C0_SCL_PUDEN   (0x00000008u)
#define CONTROL_CONF_I2C0_SCL_CONF_I2C0_SCL_PUDEN_SHIFT   (0x00000003u)

#define CONTROL_CONF_I2C0_SCL_CONF_I2C0_SCL_PUTYPESEL   (0x00000010u)
#define CONTROL_CONF_I2C0_SCL_CONF_I2C0_SCL_PUTYPESEL_SHIFT   (0x00000004u)

#define CONTROL_CONF_I2C0_SCL_CONF_I2C0_SCL_RSVD   (0x000FFF80u)
#define CONTROL_CONF_I2C0_SCL_CONF_I2C0_SCL_RSVD_SHIFT   (0x00000007u)

#define CONTROL_CONF_I2C0_SCL_CONF_I2C0_SCL_RXACTIVE   (0x00000020u)
#define CONTROL_CONF_I2C0_SCL_CONF_I2C0_SCL_RXACTIVE_SHIFT   (0x00000005u)

#define CONTROL_CONF_I2C0_SCL_CONF_I2C0_SCL_SLEWCTRL   (0x00000040u)
#define CONTROL_CONF_I2C0_SCL_CONF_I2C0_SCL_SLEWCTRL_SHIFT   (0x00000006u)

static void i2c_pins_mux(void)
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

void eeprom_read(char* data, uint32_t len, uint32_t offset) {
    unsigned int idx = 0;

    i2c_set_data_count(I2C_BASE_ADDR, 2);
    i2c_master_int_clear_ex(I2C_BASE_ADDR, 0x7FF);

    i2c_master_control(I2C_BASE_ADDR, I2C_CFG_MST_TX);
    i2c_master_start(I2C_BASE_ADDR);

    while (0 == i2c_master_bus_busy(I2C_BASE_ADDR));

    i2c_master_data_put(I2C_BASE_ADDR, (uint8_t)((offset >> 8) & 0xFF));

    while (0 == i2c_master_int_raw_status_ex(I2C_BASE_ADDR,
                                        I2C_INT_TRANSMIT_READY));

    i2c_master_data_put(I2C_BASE_ADDR, (unsigned char)(offset & 0xFF));
    i2c_master_int_clear_ex(I2C_BASE_ADDR, I2C_INT_TRANSMIT_READY);

    while(0 == (i2c_master_int_raw_status(I2C_BASE_ADDR) & I2C_INT_ADRR_READY_ACESS));
    i2c_master_int_clear_ex(I2C_BASE_ADDR, 0x7FF);

    i2c_set_data_count(I2C_BASE_ADDR, len);
    i2c_master_control(I2C_BASE_ADDR, I2C_CFG_MST_RX);
    i2c_master_start(I2C_BASE_ADDR);

    while (len--)
    {
        while (0 == i2c_master_int_raw_status_ex(I2C_BASE_ADDR,
                                            I2C_INT_RECV_READY));
        data[idx++] = (uint32_t)i2c_master_data_get(I2C_BASE_ADDR);
        i2c_master_int_clear_ex(I2C_BASE_ADDR, I2C_INT_RECV_READY);
    }

    i2c_master_stop(I2C_BASE_ADDR);

    while(0 == (i2c_master_int_raw_status(I2C_BASE_ADDR) & I2C_INT_STOP_CONDITION));

    i2c_master_int_clear_ex(I2C_BASE_ADDR, I2C_INT_STOP_CONDITION);
}

void eeprom_init(uint32_t slave_addr) {
    i2c_init_clocks();

    i2c_pins_mux();

    i2c_master_disable(I2C_BASE_ADDR);
    i2c_soft_reset(I2C_BASE_ADDR);

    i2c_master_init_clock(I2C_BASE_ADDR, 48000000, 24000000, 100000);
    i2c_master_slave_addr_set(I2C_BASE_ADDR, slave_addr);
    i2c_master_int_disable_ex(I2C_BASE_ADDR, 0xFFFFFFFF);
    i2c_master_enable(I2C_BASE_ADDR);
}
