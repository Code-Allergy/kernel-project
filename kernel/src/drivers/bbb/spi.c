#include <kernel/printk.h>

#include "i2c.h"


static void I2C0PinMux(void)
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

/* Clear the status of all interrupts */
static void cleanupInterrupts(void)
{
    i2c_master_int_clear_ex(I2C_BASE_ADDR,  0x7FF);
}


/**
 * \brief   Reads data from the CPLD register offset
 *
 * \return  none
 *
 * \note    interrupts are not used for simplicity. In case interrupts are
 *          desired, it should be handled by the application
 */
void CPLDI2CRead(uint8_t *data, unsigned int length, unsigned char offset)
{
    unsigned int i = 0;

    /* First send the register offset - TX operation */
    i2c_set_data_count(I2C_BASE_ADDR, 1);

    cleanupInterrupts();

    i2c_master_control(I2C_BASE_ADDR, I2C_CFG_MST_TX);

    i2c_master_start(I2C_BASE_ADDR);
    printk("I2C: Waiting for bus to be free\n");
    /* Wait for the START to actually occur on the bus */
    while (0 == i2c_master_bus_busy(I2C_BASE_ADDR));

    printk("I2C: Bus is free, waiting for tx reg to be empty\n");
    /* Wait for the Tx register to be empty */
    while (0 == i2c_master_int_raw_status_ex(I2C_BASE_ADDR, I2C_INT_TRANSMIT_READY));
    printk("I2C: tx reg is empty\n");

    /* Push offset out and tell CPLD from where we intend to read the data */
    i2c_master_data_put(I2C_BASE_ADDR, offset);

    i2c_master_int_clear_ex(I2C_BASE_ADDR, I2C_INT_TRANSMIT_READY);

    printk("I2C: Waiting for address ready\n");
    while(0 == (i2c_master_int_raw_status(I2C_BASE_ADDR) & I2C_INT_ADRR_READY_ACESS));

    i2c_set_data_count(I2C_BASE_ADDR, length);

    cleanupInterrupts();

    /* Now that we have sent the register offset, start a RX operation*/
    i2c_master_control(I2C_BASE_ADDR, I2C_CFG_MST_RX);

    /* Repeated start condition */
    i2c_master_start(I2C_BASE_ADDR);
    printk("I2C: Waiting for bus to be free\n");
    while(i2c_master_bus_busy(I2C_BASE_ADDR) == 0);

    while (length--)
    {
        while (0 == i2c_master_int_raw_status_ex(I2C_BASE_ADDR, I2C_INT_RECV_READY));
        data[i] = (uint32_t)i2c_master_data_get(I2C_BASE_ADDR);
        i2c_master_int_clear_ex(I2C_BASE_ADDR, I2C_INT_RECV_READY);
    }

    i2c_master_stop(I2C_BASE_ADDR);

    while(0 == (i2c_master_int_raw_status(I2C_BASE_ADDR) & I2C_INT_STOP_CONDITION));

    i2c_master_int_clear_ex(I2C_BASE_ADDR, I2C_INT_STOP_CONDITION);

}

/**
 * \brief   Configures the I2C Instance
 *
 * \return  none
 */
void CPLDI2CSetup(void)
{
    /* Configuring system clocks for I2C0 instance. */
    i2c_init_clocks();

    /* Performing Pin Multiplexing for I2C0. */
    I2C0PinMux();

    /* Put i2c in reset/disabled state */
    i2c_master_disable(I2C_BASE_ADDR);

    /* Auto Idle functionality is Disabled */
    i2c_auto_idle_disable(I2C_BASE_ADDR);

    /* Configure i2c bus speed to 100khz */
    i2c_master_init_clock(I2C_BASE_ADDR, 48000000, 12000000, 100000);

    /* Set i2c slave address */
    i2c_master_slave_addr_set(I2C_BASE_ADDR, 0x35);

    /* Disable all I2C interrupts */
    i2c_master_int_disable_ex(I2C_BASE_ADDR, 0xFFFFFFFF);

    /* Bring I2C module out of reset */
    i2c_master_enable(I2C_BASE_ADDR);

    while(!i2c_system_status_get(I2C_BASE_ADDR));
}

#define CPLD_I2C_ADDR        0x35
#define CPLD_I2C_BASE        SOC_I2C_0_REGS
#define CPLD_CTRLREG_OFFSET    0x10
/* EVM Profile setting for McSPI */
#define MCSPI_EVM_PROFILE              (2)
#define CONTROL_CONF_SPI0_SCLK   (0x950)
#define CONTROL_CONF_SPI0_D0   (0x954)

/* CONF_SPI0_SCLK */
#define CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_MMODE   (0x00000007u)
#define CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_MMODE_SHIFT   (0x00000000u)

#define CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_PUDEN   (0x00000008u)
#define CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_PUDEN_SHIFT   (0x00000003u)

#define CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_PUTYPESEL   (0x00000010u)
#define CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_PUTYPESEL_SHIFT   (0x00000004u)

#define CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_RSVD   (0x000FFF80u)
#define CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_RSVD_SHIFT   (0x00000007u)

#define CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_RXACTIVE   (0x00000020u)
#define CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_RXACTIVE_SHIFT   (0x00000005u)

#define CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_SLEWCTRL   (0x00000040u)
#define CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_SLEWCTRL_SHIFT   (0x00000006u)


/* CONF_SPI0_D0 */
#define CONTROL_CONF_SPI0_D0_CONF_SPI0_D0_MMODE   (0x00000007u)
#define CONTROL_CONF_SPI0_D0_CONF_SPI0_D0_MMODE_SHIFT   (0x00000000u)

#define CONTROL_CONF_SPI0_D0_CONF_SPI0_D0_PUDEN   (0x00000008u)
#define CONTROL_CONF_SPI0_D0_CONF_SPI0_D0_PUDEN_SHIFT   (0x00000003u)

#define CONTROL_CONF_SPI0_D0_CONF_SPI0_D0_PUTYPESEL   (0x00000010u)
#define CONTROL_CONF_SPI0_D0_CONF_SPI0_D0_PUTYPESEL_SHIFT   (0x00000004u)

#define CONTROL_CONF_SPI0_D0_CONF_SPI0_D0_RSVD   (0x000FFF80u)
#define CONTROL_CONF_SPI0_D0_CONF_SPI0_D0_RSVD_SHIFT   (0x00000007u)

#define CONTROL_CONF_SPI0_D0_CONF_SPI0_D0_RXACTIVE   (0x00000020u)
#define CONTROL_CONF_SPI0_D0_CONF_SPI0_D0_RXACTIVE_SHIFT   (0x00000005u)

#define CONTROL_CONF_SPI0_D0_CONF_SPI0_D0_SLEWCTRL   (0x00000040u)
#define CONTROL_CONF_SPI0_D0_CONF_SPI0_D0_SLEWCTRL_SHIFT   (0x00000006u)
#define SPI_BASE                       (0x48030000)
/**
 * \brief   Reads the profile configured for the EVM from CPLD
 *
 * \return  profile number
 */
unsigned int EVMProfileGet(void)
{
    unsigned int data = 0;

    CPLDI2CSetup();
    printk("I2C setup done\n");
    CPLDI2CRead((uint8_t*)&data, 1, CPLD_CTRLREG_OFFSET);
    printk("Done reading\n");
    return (data & 0x07);
}
/**
 * \brief   This function selects the McSPI pins for use. The McSPI pins
 *          are multiplexed with pins of other peripherals in the SoC
 *
 * \param   instanceNum       The instance number of the McSPI instance to be
 *                            used.
 * \return  Returns the value S_PASS if the desired functionality is met else
 *          returns the appropriate error value. Error values can be
 *          1) E_INST_NOT_SUPP - McSPI instance not supported
 *          2) E_INVALID_PROFILE - Invalid profile setting of EVM for McSPI
 *
 * \note    This muxing depends on the profile in which the EVM is configured.
 */
int McSPIPinMuxSetup(unsigned int instanceNum)
{
    unsigned int profile = 0;
    int status = -1;

    if(0 == instanceNum)
    {
        profile = EVMProfileGet();
        printk("Back from profile\n");
        switch (profile)
        {
            case MCSPI_EVM_PROFILE:
                REG32(CONTROL_MODULE_BASE + CONTROL_CONF_SPI0_SCLK) =
                     (CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_PUTYPESEL |
                      CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_RXACTIVE);
                REG32(CONTROL_MODULE_BASE + CONTROL_CONF_SPI0_D0) =
                     (CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_PUTYPESEL |
                      CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_RXACTIVE);
                REG32(CONTROL_MODULE_BASE + CONTROL_CONF_SPI0_D1) =
                     (CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_PUTYPESEL |
                      CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_RXACTIVE);
                status = 0;
            break;
            default:
                status = -2;
            break;
        }
    }
    return status;
}

int McSPI0CSPinMuxSetup(unsigned int csPinNum)
{
    unsigned int profile = 0;
    int status = -1;

    if(0 == csPinNum)
    {
        profile = EVMProfileGet();

        switch(profile)
        {
            case MCSPI_EVM_PROFILE:
                REG32(CONTROL_MODULE_BASE + CONTROL_CONF_SPI0_CS0) =
                     (CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_PUTYPESEL |
                      CONTROL_CONF_SPI0_SCLK_CONF_SPI0_SCLK_RXACTIVE);
                status = 0;
            break;
            default:
                status = -2;
            break;
        }
    }
    return status;
}

/**
* \brief  This API will reset the McSPI peripheral.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
*
* \return none.
**/
void McSPIReset(unsigned int baseAdd)
{
    /* Set the SOFTRESET field of MCSPI_SYSCONFIG register. */
    REG32(baseAdd + MCSPI_SYSCONFIG) |= MCSPI_SYSCONFIG_SOFTRESET;

    /* Stay in the loop until reset is done. */
    while(!(MCSPI_SYSSTATUS_RESETDONE_COMPLETED &
            REG32(baseAdd + MCSPI_SYSSTATUS)));
}

/**
* \brief  This API will enable the chip select pin.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
*
* \return none.
*
* \note:  Modification of CS polarity, SPI clock phase and polarity
          is not allowed when CS is enabled.
**/
void McSPICSEnable(unsigned int baseAdd)
{
    /* CLear PIN34 of MCSPI_MODULCTRL register. */
    REG32(baseAdd + MCSPI_MODULCTRL) &= ~MCSPI_MODULCTRL_PIN34;
}

/**
* \brief  This API will enable the McSPI controller in master mode.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
*
* \return none.
**/
void McSPIMasterModeEnable(unsigned int baseAdd)
{
    /* Clear the MS field of MCSPI_MODULCTRL register. */
    REG32(baseAdd + MCSPI_MODULCTRL) &= ~MCSPI_MODULCTRL_MS;
}

#define MCSPI_TX_RX_MODE                    (MCSPI_CH0CONF_TRM_TXRX)
#define MCSPI_RX_ONLY_MODE                  (MCSPI_CH0CONF_TRM_RXONLY << \
                                             MCSPI_CH0CONF_TRM_SHIFT)
#define MCSPI_TX_ONLY_MODE                  (MCSPI_CH0CONF_TRM_TXONLY << \
                                             MCSPI_CH0CONF_TRM_SHIFT)

/*
** Values used to configure communication on data line pins.
*/
#define MCSPI_DATA_LINE_COMM_MODE_0        ((MCSPI_CH0CONF_IS_LINE0) | \
                                            (MCSPI_CH0CONF_DPE1_ENABLED) | \
                                            (MCSPI_CH0CONF_DPE0_ENABLED))
#define MCSPI_DATA_LINE_COMM_MODE_1        ((MCSPI_CH0CONF_IS_LINE0)| \
                                            (MCSPI_CH0CONF_DPE1_ENABLED)| \
                                            (MCSPI_CH0CONF_DPE0_DISABLED << \
                                             MCSPI_CH0CONF_DPE0_SHIFT))
#define MCSPI_DATA_LINE_COMM_MODE_2        ((MCSPI_CH0CONF_IS_LINE0) | \
                                            (MCSPI_CH0CONF_DPE1_DISABLED << \
                                             MCSPI_CH0CONF_DPE1_SHIFT) | \
                                            (MCSPI_CH0CONF_DPE0_ENABLED))
#define MCSPI_DATA_LINE_COMM_MODE_3        ((MCSPI_CH0CONF_IS_LINE0) | \
                                            (MCSPI_CH0CONF_DPE1_DISABLED << \
                                             MCSPI_CH0CONF_DPE1_SHIFT) | \
                                            (MCSPI_CH0CONF_DPE0_DISABLED << \
                                             MCSPI_CH0CONF_DPE0_SHIFT))
#define MCSPI_DATA_LINE_COMM_MODE_4         ((MCSPI_CH0CONF_IS_LINE1 << \
                                              MCSPI_CH0CONF_IS_SHIFT) | \
                                             (MCSPI_CH0CONF_DPE1_ENABLED) | \
                                             (MCSPI_CH0CONF_DPE0_ENABLED))
#define MCSPI_DATA_LINE_COMM_MODE_5         ((MCSPI_CH0CONF_IS_LINE1 << \
                                              MCSPI_CH0CONF_IS_SHIFT) | \
                                             (MCSPI_CH0CONF_DPE1_ENABLED) | \
                                             (MCSPI_CH0CONF_DPE0_DISABLED << \
                                              MCSPI_CH0CONF_DPE0_SHIFT))
#define MCSPI_DATA_LINE_COMM_MODE_6         ((MCSPI_CH0CONF_IS_LINE1 << \
                                              MCSPI_CH0CONF_IS_SHIFT) | \
                                             (MCSPI_CH0CONF_DPE1_DISABLED << \
                                              MCSPI_CH0CONF_DPE1_SHIFT) | \
                                             (MCSPI_CH0CONF_DPE0_ENABLED))
#define MCSPI_DATA_LINE_COMM_MODE_7         ((MCSPI_CH0CONF_IS_LINE1 << \
                                              MCSPI_CH0CONF_IS_SHIFT) | \
                                             (MCSPI_CH0CONF_DPE1_DISABLED << \
                                              MCSPI_CH0CONF_DPE1_SHIFT) | \
                                             (MCSPI_CH0CONF_DPE0_DISABLED << \
                                              MCSPI_CH0CONF_DPE0_SHIFT))

#define MCSPI_SINGLE_CH                (MCSPI_MODULCTRL_SINGLE_SINGLE)
#define MCSPI_MULTI_CH                 (MCSPI_MODULCTRL_SINGLE_MULTI)

/**
* \brief  This API will enable the McSPI controller in master mode and
*         configure other parameters required for master mode.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  channelMode    Single/Multi channel.
* \param  trMode         Transmit/Receive mode used in master
*                        configuration.
* \param  pinMode        Interface mode and pin assignment.
* \param  chNum          Channel number of the McSPI instance used.\n
*
*         'channelMode' can take the following values.\n
*         MCSPI_SINGLE_CH - Single channel mode is used.\n
*         MCSPI_MULTI_CH  - Multi channel mode is used.\n
*
*         'trMode' can take the following values.\n
*         MCSPI_TX_RX_MODE   - Enable McSPI in TX and RX modes.\n
*         MCSPI_RX_ONLY_MODE - Enable McSPI only in RX mode.\n
*         MCSPI_TX_ONLY_MODE - Enable McSPI only in TX mode.\n
*
*         'pinMode' can take the following values.\n
*         MCSPI_DATA_LINE_COMM_MODE_n - Mode n configuration for SPIDAT[1:0].\n
*
*         For pinMode 0 <= n <= 7.\n
*
*         'chNum' can take the following values.\n
*         MCSPI_CHANNEL_n - Channel n is used for communication.\n
*
*         For chNum 0 <= n <= 3.\n
*
* \return 'retVal' which states if the combination of trMode and pinMode chosen
*          by the user is supported for communication on SPIDAT[1:0] pins.\n
*          TRUE  - Communication supported by SPIDAT[1:0].\n
*          FALSE - Communication not supported by SPIDAT[1:0].\n
*
* \note   Please refer the description about IS,DPE1,DPE0 and TRM bits for
*         proper configuration of SPIDAT[1:0].\n
**/
unsigned int McSPIMasterModeConfig(unsigned int baseAdd,
                                   unsigned int channelMode,
                                   unsigned int trMode,
                                   unsigned int pinMode,
                                   unsigned int chNum)
{
    unsigned int retVal = 0;

    /* Clear the MS field of MCSPI_MODULCTRL register. */
    REG32(baseAdd + MCSPI_MODULCTRL) &= ~MCSPI_MODULCTRL_SINGLE;

    /* Set the MS field with the user sent value. */
    REG32(baseAdd + MCSPI_MODULCTRL) |= (channelMode & MCSPI_MODULCTRL_SINGLE);

    /* Clear the TRM field of MCSPI_CHCONF register. */
    REG32(baseAdd + MCSPI_CHCONF(chNum)) &= ~MCSPI_CH0CONF_TRM;

    /* Set the TRM field with the user sent value. */
    REG32(baseAdd + MCSPI_CHCONF(chNum)) |= (trMode & MCSPI_CH0CONF_TRM);

    if(((MCSPI_TX_RX_MODE == trMode) &&
       (MCSPI_DATA_LINE_COMM_MODE_3 == pinMode)) ||
       ((MCSPI_TX_ONLY_MODE == trMode) &&
       (MCSPI_DATA_LINE_COMM_MODE_3 == pinMode)) ||
       ((MCSPI_TX_RX_MODE == trMode) &&
       (MCSPI_DATA_LINE_COMM_MODE_7 == pinMode)) ||
       ((MCSPI_TX_ONLY_MODE == trMode) &&
       (MCSPI_DATA_LINE_COMM_MODE_7 == pinMode)))
    {
        retVal = 0;
    }
    else
    {
        /* Clear the IS, DPE0, DPE1 fields of MCSPI_CHCONF register. */
        REG32(baseAdd + MCSPI_CHCONF(chNum)) &= ~(MCSPI_CH0CONF_IS |
                                                   MCSPI_CH0CONF_DPE1 |
                                                   MCSPI_CH0CONF_DPE0);

        /* Set the IS, DPE0, DPE1 fields with the user sent values. */
        REG32(baseAdd + MCSPI_CHCONF(chNum)) |= (pinMode & (MCSPI_CH0CONF_IS |
                                                            MCSPI_CH0CONF_DPE1 |
                                                            MCSPI_CH0CONF_DPE0));

        retVal = 1;
    }

    return retVal;
}

#define MCSPI_CLKD_MASK       (0xF)

/**
* \brief  This API will configure the clkD and extClk fields to generate
*         required spi clock depending on the type of granularity. It will
*         also set the phase and polarity of spiClk by the clkMode field.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  spiInClk       Clock frequency given to the McSPI module.
* \param  spiOutClk      Clock frequency on the McSPI bus.
* \param  chNum          Channel number of the McSPI instance used.
* \param  clkMode        Clock mode used.\n
*
*         'chNum' can take the following values.\n
*         MCSPI_CHANNEL_n - Channel n is used for communication.\n
*
*         For chNum 0 <= n <= 3.\n
*
*         'clkMode' can take the following values.\n
*         MCSPI_CLK_MODE_n - McSPI clock mode n.\n
*
*         For clkMode 0 <= n <= 3.\n
*
* \return none.
*
* \note:  1) clkMode depends on phase and polarity of McSPI clock.\n
*         2) To pass the desired value for clkMode please refer the
*            McSPI_CH(i)CONF register.\n
*         3) Please understand the polarity and phase of the slave device
*            connected and accordingly set the clkMode.\n
*         4) McSPIClkConfig does not have any significance in slave mode
*            because the clock signal required for communication is generated
*            by the master device.
**/
void McSPIClkConfig(unsigned int baseAdd, unsigned int spiInClk,
                    unsigned int spiOutClk, unsigned int chNum,
                    unsigned int clkMode)
{
    unsigned int fRatio = 0;
    unsigned int extClk = 0;
    unsigned int clkD = 0;

    /* Calculate the value of fRatio. */
    fRatio = (spiInClk / spiOutClk);

    /* If fRatio is not a power of 2, set granularity of 1 clock cycle */
    if(0 != (fRatio & (fRatio - 1)))
    {
        /* Set the clock granularity to 1 clock cycle.*/
        REG32(baseAdd + MCSPI_CHCONF(chNum)) |= MCSPI_CH0CONF_CLKG;

        /* Calculate the ratios clkD and extClk based on fClk */
        extClk = (fRatio - 1) >> 4;
        clkD = (fRatio - 1) & MCSPI_CLKD_MASK;

        /*Clear the extClk field of MCSPI_CHCTRL register.*/
        REG32(baseAdd + MCSPI_CHCTRL(chNum)) &= ~MCSPI_CH0CTRL_EXTCLK;

        /* Set the extClk field of MCSPI_CHCTRL register.*/
        REG32(baseAdd + MCSPI_CHCTRL(chNum)) |= (extClk <<
                                                 MCSPI_CH0CTRL_EXTCLK_SHIFT);
    }
    else
    {
        /* Clock granularity of power of 2.*/
        REG32(baseAdd + MCSPI_CHCONF(chNum)) &= ~MCSPI_CH0CONF_CLKG;

        while(1 != fRatio)
        {
            fRatio /= 2;
            clkD++;
        }
    }

    /*Clearing the clkD field of MCSPI_CHCONF register.*/
    REG32(baseAdd + MCSPI_CHCONF(chNum)) &= ~MCSPI_CH0CONF_CLKD;

    /* Configure the clkD field of MCSPI_CHCONF register.*/
    REG32(baseAdd + MCSPI_CHCONF(chNum)) |= (clkD << MCSPI_CH0CONF_CLKD_SHIFT);

    /*Clearing the clkMode field of MCSPI_CHCONF register.*/
    REG32(baseAdd + MCSPI_CHCONF(chNum)) &= ~(MCSPI_CH0CONF_PHA |
                                               MCSPI_CH0CONF_POL);

    /* Configure the clkMode of MCSPI_CHCONF register.*/
    REG32(baseAdd + MCSPI_CHCONF(chNum)) |= (clkMode & (MCSPI_CH0CONF_PHA |
                                                         MCSPI_CH0CONF_POL));
}

#define CHAR_LENGTH             0x8
#define MCSPI_IN_CLK            48000000
#define MCSPI_OUT_CLK           24000000

/* flash data read command */
#define SPI_FLASH_READ          0x03
#define SPI_CHAN                       0x0

#define MCSPI_CLK_MODE_0              ((MCSPI_CH0CONF_POL_ACTIVEHIGH << \
                                        MCSPI_CH0CONF_POL_SHIFT) | \
                                        MCSPI_CH0CONF_PHA_ODD)
#define MCSPI_CLK_MODE_1               ((MCSPI_CH0CONF_POL_ACTIVEHIGH << \
                                         MCSPI_CH0CONF_POL_SHIFT) | \
                                         MCSPI_CH0CONF_PHA_EVEN)
#define MCSPI_CLK_MODE_2               ((MCSPI_CH0CONF_POL_ACTIVELOW << \
                                         MCSPI_CH0CONF_POL_SHIFT) | \
                                         MCSPI_CH0CONF_PHA_ODD)
#define MCSPI_CLK_MODE_3               ((MCSPI_CH0CONF_POL_ACTIVELOW << \
                                         MCSPI_CH0CONF_POL_SHIFT) | \
                                         MCSPI_CH0CONF_PHA_EVEN)
#define MCSPI_WORD_LENGTH(n)           ((n - 1) << MCSPI_CH0CONF_WL_SHIFT)

/*
** Values used to set CS polarity for the McSPI peripheral.
*/
#define MCSPI_CS_POL_HIGH            (MCSPI_CH0CONF_EPOL_ACTIVEHIGH)
#define MCSPI_CS_POL_LOW             (MCSPI_CH0CONF_EPOL)

/*
** Values used to enable/disable the read/write DMA events of McSPI peripheral.
*/
#define MCSPI_DMA_RX_EVENT           (MCSPI_CH0CONF_DMAR)
#define MCSPI_DMA_TX_EVENT           (MCSPI_CH0CONF_DMAW)

/**
* \brief  This API will configure the length of McSPI word used for
*         communication.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  wordLength     Length of a data word used for McSPI communication.
* \param  chNum          Channel number of the McSPI instance used.\n
*
*         'wordLength' can take the following values.\n
*          MCSPI_WORD_LENGTH(n)  - McSPI word length is n bits long.\n
*
*          For wordLength 4 <= n <= 32.\n
*
*          'chNum' can take the following values.\n
*          MCSPI_CHANNEL_n - Channel n is used for communication.\n
*
*          For chNum n can vary from 0-3.\n
*
* \return none.
*
* \note:  wordLength can vary from 4-32 bits length. To program the
*         required value of wordLength please refer the MCSPI_CH(i)CONF
*         register.\n
**/
void McSPIWordLengthSet(unsigned int baseAdd, unsigned int wordLength,
                        unsigned int chNum)
{
    /*Clearing the wordLength field of MCSPI_CHCONF register.*/
    REG32(baseAdd + MCSPI_CHCONF(chNum)) &= ~MCSPI_CH0CONF_WL;

    /* Setting the wordlength field. */
    REG32(baseAdd + MCSPI_CHCONF(chNum)) |= (wordLength & MCSPI_CH0CONF_WL);
}

/*
** Values used to enable/disable the Tx/Rx FIFOs of McSPI peripheral.
*/
#define MCSPI_RX_FIFO_ENABLE         (MCSPI_CH0CONF_FFER)
#define MCSPI_RX_FIFO_DISABLE        (MCSPI_CH0CONF_FFER_FFDISABLED)
#define MCSPI_TX_FIFO_ENABLE         (MCSPI_CH0CONF_FFEW)
#define MCSPI_TX_FIFO_DISABLE        (MCSPI_CH0CONF_FFEW_FFDISABLED)

/**
* \brief  This API will configure the chip select polarity.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  spiEnPol       Polarity of CS line.
* \param  chNum          Channel number of the McSPI instance used.\n
*
*        'spiEnPol' can take the following values.\n
*         MCSPI_CS_POL_HIGH - SPIEN pin is held high during the active state.\n
*         MCSPI_CS_POL_LOW - SPIEN pin is held low during the active state.\n
*
*         'chNum' can take the following values.\n
*         MCSPI_CHANNEL_n - Channel n is used for communication.\n
*
*         For chNum 0 <= n <= 3.\n
*
* \return none.
**/
void McSPICSPolarityConfig(unsigned int baseAdd, unsigned int spiEnPol,
                           unsigned int chNum)
{
    /* Clear the EPOL field of MCSPI_CHCONF register. */
    REG32(baseAdd + MCSPI_CHCONF(chNum)) &= ~MCSPI_CH0CONF_EPOL;

    /* Set the EPOL field with the user sent value. */
    REG32(baseAdd + MCSPI_CHCONF(chNum)) |= (spiEnPol & MCSPI_CH0CONF_EPOL);
}


/**
* \brief  This API will enable/disable the Tx FIFOs of McSPI peripheral.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  txFifo         FIFO used for transmit mode.
* \param  chNum          Channel number of the McSPI instance used.\n
*
*         'txFifo' can take the following values.\n
*         MCSPI_TX_FIFO_ENABLE  - Enables the transmitter FIFO of McSPI.\n
*         MCSPI_TX_FIFO_DISABLE - Disables the transmitter FIFO of McSPI.\n
*
*         'chNum' can take the following values.\n
*         MCSPI_CHANNEL_n - Channel n is used for communication.\n
*
*         For chNum n can range from 0-3.\n
*
* \return none.
*
* \note:  Enabling FIFO is restricted to only 1 channel.
**/
void McSPITxFIFOConfig(unsigned int baseAdd, unsigned int txFifo,
                       unsigned int chNum)
{
    /* Clear the FFEW field of MCSPI_CHCONF register. */
    REG32(baseAdd + MCSPI_CHCONF(chNum)) &= ~MCSPI_CH0CONF_FFEW;

    /* Set the FFEW field with user sent value. */
    REG32(baseAdd + MCSPI_CHCONF(chNum)) |= (txFifo & MCSPI_CH0CONF_FFEW);
}

/**
* \brief  This API will enable/disable the Rx FIFOs of McSPI peripheral.
*
* \param  baseAdd        Memory Address of the McSPI instance used.
* \param  rxFifo         FIFO used for receive mode.
* \param  chNum          Channel number of the McSPI instance used.\n
*
*         'rxFifo' can take the following values.\n
*         MCSPI_RX_FIFO_ENABLE - Enables the receiver FIFO of McSPI.\n
*         MCSPI_RX_FIFO_DISABLE - Disables the receiver FIFO of McSPI.\n
*
*         'chNum' can take the following values.\n
*         MCSPI_CHANNEL_n - Channel n is used for communication.\n
*
*         For chNum n can range from 0-3.\n
*
* \return none.
*
* \note:  Enabling FIFO is restricted to only 1 channel.
**/
void McSPIRxFIFOConfig(unsigned int baseAdd, unsigned int rxFifo,
                       unsigned int chNum)
{
    /* Clear the FFER field of MCSPI_CHCONF register. */
    REG32(baseAdd + MCSPI_CHCONF(chNum)) &= ~MCSPI_CH0CONF_FFER;

    /* Set the FFER field with the user sent value. */
    REG32(baseAdd + MCSPI_CHCONF(chNum)) |= (rxFifo & MCSPI_CH0CONF_FFER);
}

/*
* \brief - SPI Configures.
* \param - none.
*
* \return  none.
*/

void SPIConfigure(void)
{
    unsigned int retVal = 0;

    /* Reset the McSPI instance.*/
    McSPIReset(SPI_BASE);

    /* Enable chip select pin.*/
    McSPICSEnable(SPI_BASE);

    /* Enable master mode of operation.*/
    McSPIMasterModeEnable(SPI_BASE);

    /* Perform the necessary configuration for master mode.*/
    retVal = McSPIMasterModeConfig(SPI_BASE, MCSPI_SINGLE_CH, MCSPI_TX_RX_MODE,\
                                   MCSPI_DATA_LINE_COMM_MODE_1, SPI_CHAN);

    /*
    ** If combination of trm and IS,DPE0 and DPE1 is not valid then retVal is
    ** false.
    */
    if(!retVal)
    {
        printk("Invalid McSPI config \n");
        printk("Aborting Boot\n");
        while(1);
    }

    /*
    ** Default granularity is used. Also as per my understanding clock mode
    ** 0 is proper.
    */
    McSPIClkConfig(SPI_BASE, MCSPI_IN_CLK, MCSPI_OUT_CLK,
                   SPI_CHAN, MCSPI_CLK_MODE_0);

    /* Configure the word length.*/
    McSPIWordLengthSet(SPI_BASE, MCSPI_WORD_LENGTH(8), SPI_CHAN);

    /* Set polarity of SPIEN to low.*/
    McSPICSPolarityConfig(SPI_BASE, MCSPI_CS_POL_LOW, SPI_CHAN);

    /* Enable the transmitter FIFO of McSPI peripheral.*/
    McSPITxFIFOConfig(SPI_BASE, MCSPI_TX_FIFO_ENABLE, SPI_CHAN);

    /* Enable the receiver FIFO of McSPI peripheral.*/
    McSPIRxFIFOConfig(SPI_BASE, MCSPI_RX_FIFO_ENABLE, SPI_CHAN);
}


void spi_setup(void) {
    unsigned int regVal;

    /* Enable clock for SPI */
    regVal = (REG32(CM_PER_BASE + CM_PER_SPI0_CLKCTRL) &
                    ~(CM_PER_SPI0_CLKCTRL_MODULEMODE));

    regVal |= CM_PER_SPI0_CLKCTRL_MODULEMODE_ENABLE;

    REG32(CM_PER_BASE + CM_PER_SPI0_CLKCTRL) = regVal;

    /* Setup SPI PINMUX */
    McSPIPinMuxSetup(0);
    McSPI0CSPinMuxSetup(0);
    SPIConfigure();
}
