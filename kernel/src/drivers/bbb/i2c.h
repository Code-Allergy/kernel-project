#include <stdint.h>
#define I2C_BASE_ADDR 0x44E0B000

#define I2C_REVNB_LO   (0x0)
#define I2C_REVNB_HI   (0x4)
#define I2C_SYSC   (0x10)
#define I2C_EOI   (0x20)
#define I2C_IRQSTATUS_RAW   (0x24)
#define I2C_IRQSTATUS   (0x28)
#define I2C_IRQENABLE_SET   (0x2C)
#define I2C_IRQENABLE_CLR   (0x30)
#define I2C_WE   (0x34)
#define I2C_DMARXENABLE_SET   (0x38)
#define I2C_DMATXENABLE_SET   (0x3C)
#define I2C_DMARXENABLE_CLR   (0x40)
#define I2C_DMATXENABLE_CLR   (0x44)
#define I2C_DMARXWAKE_EN   (0x48)
#define I2C_DMATXWAKE_EN   (0x4C)
#define I2C_SYSS   (0x90)
#define I2C_BUF   (0x94)
#define I2C_CNT   (0x98)
#define I2C_DATA   (0x9C)
#define I2C_CON   (0xA4)
#define I2C_OA   (0xA8)
#define I2C_SA   (0xAC)
#define I2C_PSC   (0xB0)
#define I2C_SCLL   (0xB4)
#define I2C_SCLH   (0xB8)
#define I2C_SYSTEST   (0xBC)
#define I2C_BUFSTAT   (0xC0)
#define I2C_OAn(n)   (0xC4 + ((n) * 4))
#define I2C_ACTOA   (0xD0)
#define I2C_SBLOCK   (0xD4)

/* IRQSTATUS_RAW */
#define I2C_IRQSTATUS_RAW_AAS   (0x00000200u)
#define I2C_IRQSTATUS_RAW_AAS_SHIFT   (0x00000009u)
#define I2C_IRQSTATUS_RAW_AAS_CLEAR   (0x1u)
#define I2C_IRQSTATUS_RAW_AAS_NOACTION   (0x0u)
#define I2C_IRQSTATUS_RAW_AAS_RECOGNIZED   (0x1u)

#define I2C_IRQSTATUS_RAW_AERR   (0x00000080u)
#define I2C_IRQSTATUS_RAW_AERR_SHIFT   (0x00000007u)
#define I2C_IRQSTATUS_RAW_AERR_CLEAR   (0x1u)
#define I2C_IRQSTATUS_RAW_AERR_ERROR   (0x1u)
#define I2C_IRQSTATUS_RAW_AERR_NOACTION   (0x0u)

#define I2C_IRQSTATUS_RAW_AL   (0x00000001u)
#define I2C_IRQSTATUS_RAW_AL_SHIFT   (0x00000000u)
#define I2C_IRQSTATUS_RAW_AL_CLEAR   (0x1u)
#define I2C_IRQSTATUS_RAW_AL_LOST   (0x1u)
#define I2C_IRQSTATUS_RAW_AL_NORMAL   (0x0u)

#define I2C_IRQSTATUS_RAW_ARDY   (0x00000004u)
#define I2C_IRQSTATUS_RAW_ARDY_SHIFT   (0x00000002u)
#define I2C_IRQSTATUS_RAW_ARDY_BUSY   (0x0u)
#define I2C_IRQSTATUS_RAW_ARDY_CLEAR   (0x1u)
#define I2C_IRQSTATUS_RAW_ARDY_READY   (0x1u)

#define I2C_IRQSTATUS_RAW_BB   (0x00001000u)
#define I2C_IRQSTATUS_RAW_BB_SHIFT   (0x0000000Cu)
#define I2C_IRQSTATUS_RAW_BB_FREE   (0x0u)
#define I2C_IRQSTATUS_RAW_BB_OCCUPIED   (0x1u)

#define I2C_IRQSTATUS_RAW_BF   (0x00000100u)
#define I2C_IRQSTATUS_RAW_BF_SHIFT   (0x00000008u)
#define I2C_IRQSTATUS_RAW_BF_CLEAR   (0x1u)
#define I2C_IRQSTATUS_RAW_BF_FREE   (0x1u)
#define I2C_IRQSTATUS_RAW_BF_NO   (0x0u)

#define I2C_IRQSTATUS_RAW_GC   (0x00000020u)
#define I2C_IRQSTATUS_RAW_GC_SHIFT   (0x00000005u)
#define I2C_IRQSTATUS_RAW_GC_CLEAR   (0x1u)
#define I2C_IRQSTATUS_RAW_GC_GENERALCALL   (0x1u)
#define I2C_IRQSTATUS_RAW_GC_NO   (0x0u)

#define I2C_IRQSTATUS_RAW_NACK   (0x00000002u)
#define I2C_IRQSTATUS_RAW_NACK_SHIFT   (0x00000001u)
#define I2C_IRQSTATUS_RAW_NACK_CLEAR   (0x1u)
#define I2C_IRQSTATUS_RAW_NACK_DETECTED   (0x1u)
#define I2C_IRQSTATUS_RAW_NACK_NOTDETECTED   (0x0u)

#define I2C_IRQSTATUS_RAW_RDR   (0x00002000u)
#define I2C_IRQSTATUS_RAW_RDR_SHIFT   (0x0000000Du)
#define I2C_IRQSTATUS_RAW_RDR_CLEAR   (0x1u)
#define I2C_IRQSTATUS_RAW_RDR_ENABLED   (0x1u)
#define I2C_IRQSTATUS_RAW_RDR_INACTIVE   (0x0u)

#define I2C_IRQSTATUS_RAW_ROVR   (0x00000800u)
#define I2C_IRQSTATUS_RAW_ROVR_SHIFT   (0x0000000Bu)
#define I2C_IRQSTATUS_RAW_ROVR_NORMAL   (0x0u)
#define I2C_IRQSTATUS_RAW_ROVR_OVERRUN   (0x1u)

#define I2C_IRQSTATUS_RAW_RRDY   (0x00000008u)
#define I2C_IRQSTATUS_RAW_RRDY_SHIFT   (0x00000003u)
#define I2C_IRQSTATUS_RAW_RRDY_CLEAR   (0x1u)
#define I2C_IRQSTATUS_RAW_RRDY_DATAREADY   (0x1u)
#define I2C_IRQSTATUS_RAW_RRDY_NODATA   (0x0u)

#define I2C_IRQSTATUS_RAW_STC   (0x00000040u)
#define I2C_IRQSTATUS_RAW_STC_SHIFT   (0x00000006u)
#define I2C_IRQSTATUS_RAW_STC_CLEAR   (0x1u)
#define I2C_IRQSTATUS_RAW_STC_NO   (0x0u)
#define I2C_IRQSTATUS_RAW_STC_STARTCONDITION   (0x1u)

#define I2C_IRQSTATUS_RAW_XDR   (0x00004000u)
#define I2C_IRQSTATUS_RAW_XDR_SHIFT   (0x0000000Eu)
#define I2C_IRQSTATUS_RAW_XDR_CLEAR   (0x1u)
#define I2C_IRQSTATUS_RAW_XDR_ENABLED   (0x1u)
#define I2C_IRQSTATUS_RAW_XDR_INACTIVE   (0x0u)

#define I2C_IRQSTATUS_RAW_XRDY   (0x00000010u)
#define I2C_IRQSTATUS_RAW_XRDY_SHIFT   (0x00000004u)
#define I2C_IRQSTATUS_RAW_XRDY_CLEAR   (0x1u)
#define I2C_IRQSTATUS_RAW_XRDY_DATAREADY   (0x1u)
#define I2C_IRQSTATUS_RAW_XRDY_ONGOING   (0x0u)

#define I2C_IRQSTATUS_RAW_XUDF   (0x00000400u)
#define I2C_IRQSTATUS_RAW_XUDF_SHIFT   (0x0000000Au)
#define I2C_IRQSTATUS_RAW_XUDF_NORMAL   (0x0u)
#define I2C_IRQSTATUS_RAW_XUDF_UNDERFLOW   (0x1u)

/* IRQSTATUS */
#define I2C_IRQSTATUS_AAS   (0x00000200u)
#define I2C_IRQSTATUS_AAS_SHIFT   (0x00000009u)
#define I2C_IRQSTATUS_AAS_NO   (0x0u)
#define I2C_IRQSTATUS_AAS_RECOGNIZED   (0x1u)

#define I2C_IRQSTATUS_AERR   (0x00000080u)
#define I2C_IRQSTATUS_AERR_SHIFT   (0x00000007u)
#define I2C_IRQSTATUS_AERR_ERROR   (0x1u)
#define I2C_IRQSTATUS_AERR_NO   (0x0u)

#define I2C_IRQSTATUS_AL   (0x00000001u)
#define I2C_IRQSTATUS_AL_SHIFT   (0x00000000u)
#define I2C_IRQSTATUS_AL_LOST   (0x1u)
#define I2C_IRQSTATUS_AL_NORMAL   (0x0u)

#define I2C_IRQSTATUS_ARDY   (0x00000004u)
#define I2C_IRQSTATUS_ARDY_SHIFT   (0x00000002u)
#define I2C_IRQSTATUS_ARDY_BUSY   (0x0u)
#define I2C_IRQSTATUS_ARDY_READY   (0x1u)

#define I2C_IRQSTATUS_BB   (0x00001000u)
#define I2C_IRQSTATUS_BB_SHIFT   (0x0000000Cu)
#define I2C_IRQSTATUS_BB_FREE   (0x0u)
#define I2C_IRQSTATUS_BB_OCCUPIED   (0x1u)

#define I2C_IRQSTATUS_BF   (0x00000100u)
#define I2C_IRQSTATUS_BF_SHIFT   (0x00000008u)
#define I2C_IRQSTATUS_BF_FREE   (0x1u)
#define I2C_IRQSTATUS_BF_NO   (0x0u)

#define I2C_IRQSTATUS_GC   (0x00000020u)
#define I2C_IRQSTATUS_GC_SHIFT   (0x00000005u)
#define I2C_IRQSTATUS_GC_GENERALCALL   (0x1u)
#define I2C_IRQSTATUS_GC_NO   (0x0u)

#define I2C_IRQSTATUS_NACK   (0x00000002u)
#define I2C_IRQSTATUS_NACK_SHIFT   (0x00000001u)
#define I2C_IRQSTATUS_NACK_DETECTED   (0x1u)
#define I2C_IRQSTATUS_NACK_NOTDETECTED   (0x0u)

#define I2C_IRQSTATUS_RDR   (0x00002000u)
#define I2C_IRQSTATUS_RDR_SHIFT   (0x0000000Du)
#define I2C_IRQSTATUS_RDR_ENABLED   (0x1u)
#define I2C_IRQSTATUS_RDR_INACTIVE   (0x0u)

#define I2C_IRQSTATUS_ROVR   (0x00000800u)
#define I2C_IRQSTATUS_ROVR_SHIFT   (0x0000000Bu)
#define I2C_IRQSTATUS_ROVR_NORMAL   (0x0u)
#define I2C_IRQSTATUS_ROVR_OVERRUN   (0x1u)

#define I2C_IRQSTATUS_RRDY   (0x00000008u)
#define I2C_IRQSTATUS_RRDY_SHIFT   (0x00000003u)
#define I2C_IRQSTATUS_RRDY_DATAREADY   (0x1u)
#define I2C_IRQSTATUS_RRDY_NODATA   (0x0u)

#define I2C_IRQSTATUS_STC   (0x00000040u)
#define I2C_IRQSTATUS_STC_SHIFT   (0x00000006u)
#define I2C_IRQSTATUS_STC_NO   (0x0u)
#define I2C_IRQSTATUS_STC_STARTCONDITION   (0x1u)

#define I2C_IRQSTATUS_XDR   (0x00004000u)
#define I2C_IRQSTATUS_XDR_SHIFT   (0x0000000Eu)
#define I2C_IRQSTATUS_XDR_ENABLED   (0x1u)
#define I2C_IRQSTATUS_XDR_INACTIVE   (0x0u)

#define I2C_IRQSTATUS_XRDY   (0x00000010u)
#define I2C_IRQSTATUS_XRDY_SHIFT   (0x00000004u)
#define I2C_IRQSTATUS_XRDY_DATAREADY   (0x1u)
#define I2C_IRQSTATUS_XRDY_ONGOING   (0x0u)

#define I2C_IRQSTATUS_XUDF   (0x00000400u)
#define I2C_IRQSTATUS_XUDF_SHIFT   (0x0000000Au)
#define I2C_IRQSTATUS_XUDF_NORMAL   (0x0u)
#define I2C_IRQSTATUS_XUDF_UNDERFLOW   (0x1u)

/* SYSC */
#define I2C_SYSC_AUTOIDLE   (0x00000001u)
#define I2C_SYSC_AUTOIDLE_SHIFT   (0x00000000u)
#define I2C_SYSC_AUTOIDLE_DISABLE   (0x0u)
#define I2C_SYSC_AUTOIDLE_ENABLE   (0x1u)

#define I2C_SYSC_CLKACTIVITY   (0x00000300u)
#define I2C_SYSC_CLKACTIVITY_SHIFT   (0x00000008u)
#define I2C_SYSC_CLKACTIVITY_BOTH   (0x3u)
#define I2C_SYSC_CLKACTIVITY_FUNC   (0x2u)
#define I2C_SYSC_CLKACTIVITY_NONE   (0x0u)
#define I2C_SYSC_CLKACTIVITY_OCP   (0x1u)

#define I2C_SYSC_ENAWAKEUP   (0x00000004u)
#define I2C_SYSC_ENAWAKEUP_SHIFT   (0x00000002u)
#define I2C_SYSC_ENAWAKEUP_DISABLE   (0x0u)
#define I2C_SYSC_ENAWAKEUP_ENABLE   (0x1u)

#define I2C_SYSC_IDLEMODE   (0x00000018u)
#define I2C_SYSC_IDLEMODE_SHIFT   (0x00000003u)
#define I2C_SYSC_IDLEMODE_FORCEIDLE   (0x0u)
#define I2C_SYSC_IDLEMODE_NOIDLE   (0x1u)
#define I2C_SYSC_IDLEMODE_SMARTIDLE   (0x2u)
#define I2C_SYSC_IDLEMODE_SMARTIDLE_WAKEUP   (0x3u)

#define I2C_SYSC_SRST   (0x00000002u)
#define I2C_SYSC_SRST_SHIFT   (0x00000001u)
#define I2C_SYSC_SRST_NORMAL   (0x0u)
#define I2C_SYSC_SRST_RESET   (0x1u)

/* CON */
#define I2C_CON_I2C_EN   (0x00008000u)
#define I2C_CON_I2C_EN_SHIFT   (0x0000000Fu)
#define I2C_CON_I2C_EN_DISABLE   (0x0u)
#define I2C_CON_I2C_EN_ENABLE   (0x1u)

#define I2C_CON_MST   (0x00000400u)
#define I2C_CON_MST_SHIFT   (0x0000000Au)
#define I2C_CON_MST_MST   (0x1u)
#define I2C_CON_MST_SLV   (0x0u)

#define I2C_CON_OPMODE   (0x00003000u)
#define I2C_CON_OPMODE_SHIFT   (0x0000000Cu)
#define I2C_CON_OPMODE_FSI2C   (0x0u)
#define I2C_CON_OPMODE_HSI2C   (0x1u)
#define I2C_CON_OPMODE_RESERVED   (0x3u)
#define I2C_CON_OPMODE_SCCB   (0x2u)

#define I2C_CON_STB   (0x00000800u)
#define I2C_CON_STB_SHIFT   (0x0000000Bu)
#define I2C_CON_STB_NORMAL   (0x0u)
#define I2C_CON_STB_STB   (0x1u)

#define I2C_CON_STP   (0x00000002u)
#define I2C_CON_STP_SHIFT   (0x00000001u)
#define I2C_CON_STP_NSTP   (0x0u)
#define I2C_CON_STP_STP   (0x1u)

#define I2C_CON_STT   (0x00000001u)
#define I2C_CON_STT_SHIFT   (0x00000000u)
#define I2C_CON_STT_NSTT   (0x0u)
#define I2C_CON_STT_STT   (0x1u)

#define I2C_CON_TRX   (0x00000200u)
#define I2C_CON_TRX_SHIFT   (0x00000009u)
#define I2C_CON_TRX_RCV   (0x0u)
#define I2C_CON_TRX_TRX   (0x1u)

#define I2C_CON_XOA0   (0x00000080u)
#define I2C_CON_XOA0_SHIFT   (0x00000007u)
#define I2C_CON_XOA0_B07   (0x0u)
#define I2C_CON_XOA0_B10   (0x1u)

#define I2C_CON_XOA1   (0x00000040u)
#define I2C_CON_XOA1_SHIFT   (0x00000006u)
#define I2C_CON_XOA1_B07   (0x0u)
#define I2C_CON_XOA1_B10   (0x1u)

#define I2C_CON_XOA2   (0x00000020u)
#define I2C_CON_XOA2_SHIFT   (0x00000005u)
#define I2C_CON_XOA2_B07   (0x0u)
#define I2C_CON_XOA2_B10   (0x1u)

#define I2C_CON_XOA3   (0x00000010u)
#define I2C_CON_XOA3_SHIFT   (0x00000004u)
#define I2C_CON_XOA3_B07   (0x0u)
#define I2C_CON_XOA3_B10   (0x1u)

#define I2C_CON_XSA   (0x00000100u)
#define I2C_CON_XSA_SHIFT   (0x00000008u)
#define I2C_CON_XSA_B07   (0x0u)
#define I2C_CON_XSA_B10   (0x1u)

// i2c 0
/*	System clock fed to I2C module - 48Mhz	*/
#define  I2C_SYSTEM_CLOCK		   (48000000u)
/*	Internal clock used by I2C module - 12Mhz	*/
#define  I2C_INTERNAL_CLOCK		   (12000000u)
/*	I2C bus speed or frequency - 100Khz	*/
#define	 I2C_OUTPUT_CLOCK		   (100000u)
#define  I2C_INTERRUPT_FLAG_TO_CLR         (0x7FF)

/*
** Values that can be passed to  I2CMasterControl API as cmd to configure mode
** of operation of I2C
*/

#define     I2C_CFG_MST_TX              I2C_CON_TRX | I2C_CON_MST
#define     I2C_CFG_MST_RX              I2C_CON_MST
#define     I2C_CFG_STOP                I2C_CON_STP
#define     I2C_CFG_N0RMAL_MODE         (0 << I2C_CON_STB_SHIFT)
#define     I2C_CFG_SRT_BYTE_MODE       I2C_CON_STB
#define     I2C_CFG_7BIT_SLAVE_ADDR     (0 << I2C_CON_XSA_SHIFT)
#define     I2C_CFG_10BIT_SLAVE_ADDR    I2C_CON_XSA
#define     I2C_CFG_10BIT_OWN_ADDR_0    I2C_CON_XOA0
#define     I2C_CFG_10BIT_OWN_ADDR_1    I2C_CON_XOA1
#define     I2C_CFG_10BIT_OWN_ADDR_2    I2C_CON_XOA2
#define     I2C_CFG_10BIT_OWN_ADDR_3    I2C_CON_XOA3
#define     I2C_CFG_7BIT_OWN_ADDR_0     (0 << I2C_CON_XOA0_SHIFT)
#define     I2C_CFG_7BIT_OWN_ADDR_1     (0 << I2C_CON_XOA1_SHIFT)
#define     I2C_CFG_7BIT_OWN_ADDR_2     (0 << I2C_CON_XOA2_SHIFT
#define     I2C_CFG_7BIT_OWN_ADDR_3     (0 << I2C_CON_XOA3_SHIFT)

/****************************************************************************/
/*
** Values that can be passed to  I2CMasterIntEnableEx API as intFlag to Enable
** interrupts.
*/

#define     I2C_INT_ARBITRATION_LOST     I2C_IRQSTATUS_AL
#define     I2C_INT_NO_ACK               I2C_IRQSTATUS_NACK
#define     I2C_INT_ADRR_READY_ACESS     I2C_IRQSTATUS_ARDY
#define     I2C_INT_RECV_READY           I2C_IRQSTATUS_RRDY
#define     I2C_INT_TRANSMIT_READY       I2C_IRQSTATUS_XRDY
#define     I2C_INT_GENERAL_CALL         I2C_IRQSTATUS_GC
#define     I2C_INT_START                I2C_IRQSTATUS_STC
#define     I2C_INT_ACCESS_ERROR         I2C_IRQSTATUS_AERR
#define     I2C_INT_STOP_CONDITION       I2C_IRQSTATUS_BF
#define     I2C_INT_ADRR_SLAVE           I2C_IRQSTATUS_AAS
#define     I2C_INT_TRANSMIT_UNDER_FLOW  I2C_IRQSTATUS_XUDF
#define     I2C_INT_RECV_OVER_RUN        I2C_IRQSTATUS_ROVR
#define     I2C_INT_RECV_DRAIN           I2C_IRQSTATUS_RDR
#define     I2C_INT_TRANSMIT_DRAIN       I2C_IRQSTATUS_XDR

/* SYSS */
#define I2C_SYSS_RDONE   (0x00000001u)
#define I2C_SYSS_RDONE_SHIFT   (0x00000000u)
#define I2C_SYSS_RDONE_RSTCOMP   (0x1u)
#define I2C_SYSS_RDONE_RSTONGOING   (0x0u)

void i2c_init_clocks(void);
void i2c_soft_reset(uint32_t base_addr);
void i2c_master_disable(uint32_t base_addr);
void i2c_master_init_clock(uint32_t base_addr, uint32_t sys_clock,
    uint32_t internal_clock, uint32_t output_clock);
void i2c_master_slave_addr_set(uint32_t base_addr, uint32_t slave_addr);
void i2c_master_int_disable_ex(uint32_t base_addr, uint32_t int_flag);
void i2c_master_enable(uint32_t base_addr);
void i2c_set_data_count(uint32_t base_addr, uint32_t count);
void i2c_master_int_clear_ex(uint32_t base_addr, uint32_t int_flag);
void i2c_master_control(uint32_t base_addr, uint32_t cmd);
void i2c_master_start(uint32_t base_addr);
uint32_t i2c_master_bus_busy(uint32_t base_addr);
void i2c_master_data_put(uint32_t base_addr, uint8_t data);
uint8_t i2c_master_data_get(uint32_t base_addr);
uint32_t i2c_master_int_raw_status_ex(uint32_t base_addr, uint32_t int_flag);
uint32_t i2c_master_int_raw_status(uint32_t base_addr);
void i2c_master_stop(uint32_t base_addr);
void i2c_pin_mux_setup(uint32_t instance);
void i2c_auto_idle_disable(uint32_t base_addr);
uint32_t i2c_system_status_get(uint32_t base_addr);
