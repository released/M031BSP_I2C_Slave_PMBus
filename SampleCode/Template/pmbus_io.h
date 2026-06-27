#ifndef PMBUS_IO_H
#define PMBUS_IO_H

/*
    This file is the platform porting contract for the PMBus core.

    Porting rule:
    - Keep pmbus_app.c/.h, pmbus_dispatch.c/.h, pmbus_drv.c/.h,
      and pmbus_pec.c/.h unchanged across MCU families when possible.
    - Re-implement this API for each platform such as MS51, M032, or M480.
    - The project-selected implementation stays in pmbus_io.c.

    Contract expectations:
    - All functions must be non-blocking.
    - pmbus_io_i2c_get_status() must return the current slave status code.
    - pmbus_io_i2c_get_received_address() should return the matched 7-bit
      slave address for the current SLA+W/SLA+R event when the MCU exposes it.
      If the platform cannot report it, return the primary PMBus address.
    - pmbus_io_i2c_slave_set_alias() must configure optional secondary slave
      address slots for ARA, ARP, and Zone aliases. Pass
      PMBUS_I2C_ALIAS_SLOT_DISABLED when a platform has no free slot.
    - pmbus_io_i2c_set_ack() and pmbus_io_i2c_clear_ack() must only control
      the next ACK/NACK response state.
    - pmbus_io_i2c_timeout_flag() and pmbus_io_i2c_clear_timeout_flag() should
      map to the platform timeout indication used by the slave recovery path
      when such a timeout source exists. Targets without a usable timeout
      source may implement them as no-op / return 0 and rely on bus-error,
      watchdog, explicit bus-clear, and external validation.
    - Do not assume hardware SMBus Bus Management / PEC registers exist.
      Software PMBus/SMBus ports must not depend on BUSCTL/BUSTCTL/BUSSTS,
      PKTSIZE/PKTCRC, BUSTOUT, or CLKTOUT.
    - pmbus_io_isr_enter() and pmbus_io_isr_exit() must preserve any required
      register-bank or special-function-page state for the target MCU.
    - Bus-clear GPIO helpers must temporarily release or drive the PMBus lines
      in open-drain compatible fashion.
    - board_config.h must provide the PMBUS_PORT_* macro contract used by the
      platform layer. Prefer selecting a PMBUS_PORT_PROFILE in board_config.h
      instead of editing PMBus protocol or dispatch files when moving to
      another I2C instance or pin assignment.
    - board_config.h should provide readable names for trace/debug:
      PMBUS_PORT_I2C_NAME
      PMBUS_PORT_SCL_PIN_NAME / PMBUS_PORT_SDA_PIN_NAME
      PMBUS_PORT_ALERT_PIN_NAME
    - Required platform actions include:
      PMBUS_PORT_I2C_ISR_PROTOTYPE
      PMBUS_PORT_INIT_I2C_PINS / PMBUS_PORT_INIT_ALERT_PIN /
      PMBUS_PORT_INIT_ADDRESS_PINS
      PMBUS_PORT_READ_ADDRESS_A0 / PMBUS_PORT_READ_ADDRESS_A1
      PMBUS_PORT_READ_SCL / PMBUS_PORT_READ_SDA
      PMBUS_PORT_PREPARE_BUS_GPIO
      PMBUS_PORT_DRIVE_SCL_LOW / PMBUS_PORT_DRIVE_SCL_HIGH /
      PMBUS_PORT_DRIVE_SDA_HIGH
      PMBUS_PORT_ALERT_ASSERT / PMBUS_PORT_ALERT_RELEASE
    - ARM Cortex-M ports typically also define PMBUS_PORT_I2C_INSTANCE,
      PMBUS_PORT_I2C_MODULE, PMBUS_PORT_I2C_IRQn, and
      PMBUS_PORT_I2C_BUS_CLOCK for pmbus_io.c.
*/

void pmbus_io_init_i2c_pins(void);
void pmbus_io_init_alert_pin(void);
void pmbus_io_init_address_pins(void);

unsigned char pmbus_io_read_address_a0(void);
unsigned char pmbus_io_read_address_a1(void);
unsigned char pmbus_io_read_scl(void);
unsigned char pmbus_io_read_sda(void);

void pmbus_io_drive_scl_low(void);
void pmbus_io_drive_scl_high(void);
void pmbus_io_drive_sda_high(void);

void pmbus_io_alert_assert(void);
void pmbus_io_alert_release(void);

void pmbus_io_i2c_slave_open(unsigned char slave_address);
void pmbus_io_i2c_slave_set_alias(unsigned char slot, unsigned char address_7bit, unsigned char enable_state);
void pmbus_io_i2c_interrupt(unsigned char enable_state);
void pmbus_io_i2c_irq_guard(unsigned char enable_state);
void pmbus_io_i2c_timeout(unsigned char enable_state);
void pmbus_io_i2c_clear_timeout_flag(void);
void pmbus_io_i2c_si_check(void);
void pmbus_io_i2c_disable(void);
unsigned char pmbus_io_i2c_get_status(void);
unsigned char pmbus_io_i2c_timeout_flag(void);
unsigned char pmbus_io_i2c_get_received_address(void);
void pmbus_io_i2c_enable_timeout_counter(void);
void pmbus_io_i2c_disable_timeout_counter(void);
void pmbus_io_i2c_bus_error_reset(void);
void pmbus_io_i2c_set_ack(void);
void pmbus_io_i2c_clear_ack(void);
unsigned char pmbus_io_i2c_read_data(void);
void pmbus_io_i2c_write_data(unsigned char value);

unsigned char pmbus_io_irq_save_disable(void);
void pmbus_io_irq_restore(unsigned char state);
void pmbus_io_enable_global_interrupt(void);
unsigned char pmbus_io_isr_enter(void);
void pmbus_io_isr_exit(unsigned char saved_state);

#endif
