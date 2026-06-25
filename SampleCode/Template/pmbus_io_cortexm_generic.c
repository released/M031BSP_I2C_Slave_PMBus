#include "pmbus_types.h"
#include "board_config.h"
#include "pmbus_io.h"

/*
    Reusable NuMicro Cortex-M PMBus I/O reference.

    Target style:
    - Nuvoton Cortex-M family such as M031, M032, M480
    - StdDriver I2C slave implementation
    - PMBUS_PORT_* macro contract provided by board_config.h

    Keep this file aligned with the active Cortex-M project pmbus_io.c.
*/

static uint8_t g_pmbus_i2c_next_ctrl = I2C_CTL_SI_AA;

void pmbus_io_init_i2c_pins(void)
{
    PMBUS_PORT_INIT_I2C_PINS();
}

void pmbus_io_init_alert_pin(void)
{
    PMBUS_PORT_INIT_ALERT_PIN();
}

void pmbus_io_init_address_pins(void)
{
    PMBUS_PORT_INIT_ADDRESS_PINS();
}

unsigned char pmbus_io_read_address_a0(void)
{
    return (unsigned char)PMBUS_PORT_READ_ADDRESS_A0();
}

unsigned char pmbus_io_read_address_a1(void)
{
    return (unsigned char)PMBUS_PORT_READ_ADDRESS_A1();
}

unsigned char pmbus_io_read_scl(void)
{
    return (unsigned char)PMBUS_PORT_READ_SCL();
}

unsigned char pmbus_io_read_sda(void)
{
    return (unsigned char)PMBUS_PORT_READ_SDA();
}

void pmbus_io_drive_scl_low(void)
{
    PMBUS_PORT_PREPARE_BUS_GPIO();
    PMBUS_PORT_DRIVE_SCL_LOW();
}

void pmbus_io_drive_scl_high(void)
{
    PMBUS_PORT_PREPARE_BUS_GPIO();
    PMBUS_PORT_DRIVE_SCL_HIGH();
}

void pmbus_io_drive_sda_high(void)
{
    PMBUS_PORT_PREPARE_BUS_GPIO();
    PMBUS_PORT_DRIVE_SDA_HIGH();
}

void pmbus_io_alert_assert(void)
{
    PMBUS_PORT_ALERT_ASSERT();
}

void pmbus_io_alert_release(void)
{
    PMBUS_PORT_ALERT_RELEASE();
}

void pmbus_io_i2c_slave_open(unsigned char slave_address)
{
    uint32_t address_7bit;

    address_7bit = (uint32_t)(slave_address >> 1);

    PMBUS_PORT_INIT_I2C_PINS();
    CLK_EnableModuleClock(PMBUS_PORT_I2C_MODULE);
    I2C_Open(PMBUS_PORT_I2C_INSTANCE, PMBUS_PORT_I2C_BUS_CLOCK);
    I2C_SetSlaveAddr(PMBUS_PORT_I2C_INSTANCE, 0U, address_7bit, 0U);
    I2C_SET_CONTROL_REG(PMBUS_PORT_I2C_INSTANCE, I2C_CTL_SI_AA);
    g_pmbus_i2c_next_ctrl = I2C_CTL_SI_AA;
}

void pmbus_io_i2c_interrupt(unsigned char enable_state)
{
    if (enable_state == Enable)
    {
        I2C_EnableInt(PMBUS_PORT_I2C_INSTANCE);
        NVIC_EnableIRQ(PMBUS_PORT_I2C_IRQn);
    }
    else
    {
        I2C_DisableInt(PMBUS_PORT_I2C_INSTANCE);
        NVIC_DisableIRQ(PMBUS_PORT_I2C_IRQn);
    }
}

void pmbus_io_i2c_irq_guard(unsigned char enable_state)
{
    if (enable_state == Enable)
    {
        NVIC_EnableIRQ(PMBUS_PORT_I2C_IRQn);
    }
    else
    {
        NVIC_DisableIRQ(PMBUS_PORT_I2C_IRQn);
    }
}

void pmbus_io_i2c_timeout(unsigned char enable_state)
{
    if (enable_state == Enable)
    {
        I2C_EnableTimeout(PMBUS_PORT_I2C_INSTANCE, 0U);
    }
    else
    {
        I2C_DisableTimeout(PMBUS_PORT_I2C_INSTANCE);
    }
}

void pmbus_io_i2c_clear_timeout_flag(void)
{
    I2C_ClearTimeoutFlag(PMBUS_PORT_I2C_INSTANCE);
}

void pmbus_io_i2c_si_check(void)
{
    I2C_SET_CONTROL_REG(PMBUS_PORT_I2C_INSTANCE, g_pmbus_i2c_next_ctrl);
}

void pmbus_io_i2c_disable(void)
{
    I2C_Close(PMBUS_PORT_I2C_INSTANCE);
}

unsigned char pmbus_io_i2c_get_status(void)
{
    return (unsigned char)I2C_GET_STATUS(PMBUS_PORT_I2C_INSTANCE);
}

unsigned char pmbus_io_i2c_timeout_flag(void)
{
    return (unsigned char)I2C_GET_TIMEOUT_FLAG(PMBUS_PORT_I2C_INSTANCE);
}

void pmbus_io_i2c_enable_timeout_counter(void)
{
    I2C_EnableTimeout(PMBUS_PORT_I2C_INSTANCE, 0U);
}

void pmbus_io_i2c_disable_timeout_counter(void)
{
    I2C_DisableTimeout(PMBUS_PORT_I2C_INSTANCE);
}

void pmbus_io_i2c_set_ack(void)
{
    g_pmbus_i2c_next_ctrl = I2C_CTL_SI_AA;
}

void pmbus_io_i2c_clear_ack(void)
{
    g_pmbus_i2c_next_ctrl = I2C_CTL_SI;
}

unsigned char pmbus_io_i2c_read_data(void)
{
    return (unsigned char)I2C_GET_DATA(PMBUS_PORT_I2C_INSTANCE);
}

void pmbus_io_i2c_write_data(unsigned char value)
{
    I2C_SET_DATA(PMBUS_PORT_I2C_INSTANCE, value);
}

unsigned char pmbus_io_irq_save_disable(void)
{
    unsigned char state;

    state = (unsigned char)(__get_PRIMASK() & 0x01U);
    __disable_irq();
    return state;
}

void pmbus_io_irq_restore(unsigned char state)
{
    __set_PRIMASK((uint32_t)state);
}

void pmbus_io_enable_global_interrupt(void)
{
    __enable_irq();
}

unsigned char pmbus_io_isr_enter(void)
{
    return 0U;
}

void pmbus_io_isr_exit(unsigned char saved_state)
{
    saved_state = saved_state;
}
