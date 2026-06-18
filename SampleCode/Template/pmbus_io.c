#include "pmbus_types.h"
#include "board_config.h"
#include "pmbus_io.h"

/*
    Active NuMicro Cortex-M port implementation.

    Keep this file aligned with pmbus_io_cortexm_generic.c.
    The generic file is a reusable reference for M031 / M032 / M480 style ports.
*/

static uint8_t g_pmbus_i2c_next_ctrl = I2C_CTL_SI_AA;
static uint8_t g_pmbus_i2c_primary_address_7bit;

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
    g_pmbus_i2c_primary_address_7bit = (uint8_t)address_7bit;

    PMBUS_PORT_INIT_I2C_PINS();
    CLK_EnableModuleClock(PMBUS_PORT_I2C_MODULE);
    I2C_Open(PMBUS_PORT_I2C_INSTANCE, PMBUS_PORT_I2C_BUS_CLOCK);
    I2C_SetSlaveAddr(PMBUS_PORT_I2C_INSTANCE, 0U, address_7bit, 0U);
    I2C_SetSlaveAddrMask(PMBUS_PORT_I2C_INSTANCE, 0U, 0U);
    I2C_SET_CONTROL_REG(PMBUS_PORT_I2C_INSTANCE, I2C_CTL_SI_AA);
    g_pmbus_i2c_next_ctrl = I2C_CTL_SI_AA;
}

void pmbus_io_i2c_slave_set_alias(unsigned char slot, unsigned char address_7bit, unsigned char enable_state)
{
    uint8_t alias_address;

    if ((slot == PMBUS_I2C_ALIAS_SLOT_DISABLED) || (slot > 3U))
    {
        return;
    }

    if (enable_state == Enable)
    {
        alias_address = (uint8_t)(address_7bit & 0x7FU);
        I2C_SetSlaveAddr(PMBUS_PORT_I2C_INSTANCE, slot, alias_address, 0U);
        I2C_SetSlaveAddrMask(PMBUS_PORT_I2C_INSTANCE, slot, 0U);
    }
    else
    {
        I2C_SetSlaveAddr(PMBUS_PORT_I2C_INSTANCE, slot, 0x7FU, 0U);
        I2C_SetSlaveAddrMask(PMBUS_PORT_I2C_INSTANCE, slot, 0U);
    }
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

unsigned char pmbus_io_i2c_get_received_address(void)
{
    uint8_t bus_value;

    /*
        NuMicro keeps the SLA byte in DAT during address-match states.
        If a port cannot expose this value, return the primary PMBus address
        in its platform implementation.
    */
    bus_value = (uint8_t)I2C_GET_DATA(PMBUS_PORT_I2C_INSTANCE);
    if (bus_value == 0U)
    {
        return g_pmbus_i2c_primary_address_7bit;
    }

    return (uint8_t)((bus_value >> 1) & 0x7FU);
}

void pmbus_io_i2c_enable_timeout_counter(void)
{
    I2C_EnableTimeout(PMBUS_PORT_I2C_INSTANCE, 0U);
}

void pmbus_io_i2c_disable_timeout_counter(void)
{
    I2C_DisableTimeout(PMBUS_PORT_I2C_INSTANCE);
}

void pmbus_io_i2c_bus_error_reset(void)
{
    I2C_SET_CONTROL_REG(PMBUS_PORT_I2C_INSTANCE, I2C_CTL_STO_SI_AA);
    I2C_SET_CONTROL_REG(PMBUS_PORT_I2C_INSTANCE, I2C_CTL_SI_AA);
    g_pmbus_i2c_next_ctrl = I2C_CTL_SI_AA;
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
