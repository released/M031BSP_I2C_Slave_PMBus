#include "pmbus_types.h"
#include "board_config.h"
#include "pmbus_app.h"
#include "pmbus_io.h"

typedef struct
{
    uint8_t slave_address_7bit;
    uint8_t address_valid;
    uint8_t page;
    uint8_t operation;
    uint8_t on_off_config;
    uint8_t phase;
    uint8_t page_plus_last_page;
    uint8_t page_plus_last_command;
    uint8_t page_plus_last_length;
    uint8_t page_plus_last_payload[PMBUS_MAX_BLOCK_SIZE];
    uint8_t store_restore_last_command;
    uint8_t store_restore_last_code;
    uint16_t store_restore_count;
    uint8_t write_protect;
    uint8_t fan_config_1_2;
    uint16_t fan_command_1;
    uint16_t fan_command_2;
    uint8_t fan_config_3_4;
    uint16_t fan_command_3;
    uint16_t fan_command_4;
    uint8_t vout_mode;
    uint16_t vout_ov_fault_limit;
    uint16_t vout_ov_warn_limit;
    uint16_t vout_uv_warn_limit;
    uint16_t vout_uv_fault_limit;
    uint16_t iout_oc_fault_limit;
    uint16_t iout_oc_lv_fault_limit;
    uint16_t iout_oc_warn_limit;
    uint16_t iout_uc_fault_limit;
    uint16_t ot_fault_limit;
    uint16_t ot_warn_limit;
    uint16_t ut_warn_limit;
    uint16_t ut_fault_limit;
    uint16_t vin_ov_fault_limit;
    uint16_t vin_ov_warn_limit;
    uint16_t vin_uv_warn_limit;
    uint16_t vin_uv_fault_limit;
    uint16_t iin_oc_fault_limit;
    uint16_t iin_oc_warn_limit;
    uint8_t vout_ov_fault_response;
    uint8_t vout_uv_fault_response;
    uint8_t iout_oc_fault_response;
    uint8_t iout_oc_lv_fault_response;
    uint8_t iout_uc_fault_response;
    uint8_t ot_fault_response;
    uint8_t ut_fault_response;
    uint8_t vin_ov_fault_response;
    uint8_t vin_uv_fault_response;
    uint8_t iin_oc_fault_response;
    uint8_t ton_max_fault_response;
    uint8_t pout_op_fault_response;
    uint16_t zone_config;
    uint16_t zone_active;
    uint16_t vout_command;
    uint16_t vout_trim;
    uint16_t vout_cal_offset;
    uint16_t vout_max;
    uint16_t vout_margin_high;
    uint16_t vout_margin_low;
    uint16_t vout_transition_rate;
    uint16_t vout_droop;
    uint16_t vout_scale_loop;
    uint16_t vout_scale_monitor;
    uint16_t vout_min;
    uint8_t coefficients[5];
    uint16_t pout_max;
    uint16_t max_duty;
    uint16_t frequency_switch;
    uint8_t power_mode;
    uint16_t vin_on;
    uint16_t vin_off;
    uint16_t interleave;
    uint16_t iout_cal_gain;
    uint16_t iout_cal_offset;
    uint16_t power_good_on;
    uint16_t power_good_off;
    uint16_t ton_delay;
    uint16_t ton_rise;
    uint16_t ton_max_fault_limit;
    uint16_t toff_delay;
    uint16_t toff_fall;
    uint16_t toff_max_warn_limit;
    uint16_t pout_op_fault_limit;
    uint16_t pout_op_warn_limit;
    uint16_t pin_op_warn_limit;
    uint16_t smbalert_mask;
    uint8_t capability;
    uint8_t status_byte;
    uint16_t status_word;
    uint8_t status_vout;
    uint8_t status_iout;
    uint8_t status_input;
    uint8_t status_temperature;
    uint8_t status_cml;
    uint8_t status_other;
    uint8_t status_mfr_specific;
    uint8_t status_fans_1_2;
    uint8_t status_vout_source;
    uint8_t status_iout_source;
    uint8_t status_input_source;
    uint8_t status_temperature_source;
    uint8_t status_cml_source;
    uint8_t status_other_source;
    uint8_t status_mfr_specific_source;
    uint8_t status_fans_1_2_source;
    uint8_t alert_asserted;
    uint8_t busy_state;
    uint8_t power_good_asserted;
    uint8_t mfr_fault_active;
    uint8_t address_a0_raw;
    uint8_t address_a1_raw;
    uint8_t comm_last_recover_reason;
    uint8_t comm_recover_count;
    uint8_t comm_recover_fail_count;
    uint8_t comm_bus_released;
    uint8_t blackbox_latched;
    uint8_t mfr_cold_redundancy_config;
    uint8_t mfr_fwupload_mode;
    uint16_t mfr_fwupload_status;
    uint8_t mfr_fwupload_last_block[PMBUS_MAX_BLOCK_SIZE];
    uint8_t mfr_fwupload_last_block_len;
    uint16_t mfr_fwupload_block_count;
    uint16_t mfr_fwupload_expected_sequence;
    uint16_t mfr_fwupload_last_sequence;
    uint8_t mfr_fwupload_final_seen;
    uint32_t source_vin_mv;
    uint32_t source_iin_ma;
    uint32_t source_vout_mv;
    uint32_t source_iout_ma;
    uint32_t source_temp1_mc;
    uint32_t source_temp2_mc;
    uint32_t source_temp3_mc;
    uint32_t source_fan1_rpm;
    uint32_t source_fan2_rpm;
    uint32_t source_pout_mw;
    uint32_t source_pin_mw;
    uint16_t read_vin;
    uint16_t read_iin;
    uint16_t read_vout;
    uint16_t read_iout;
    uint16_t read_temperature_1;
    uint16_t read_temperature_2;
    uint16_t read_temperature_3;
    uint16_t read_fan_speed_1;
    uint16_t read_fan_speed_2;
    uint16_t read_pout;
    uint16_t read_pin;
    uint8_t read_ein[6];
    uint8_t read_eout[6];
    uint8_t mfr_blackbox_live[PMBUS_BLACKBOX_BLOCK_SIZE];
    uint8_t mfr_blackbox_latched[PMBUS_BLACKBOX_BLOCK_SIZE];
    uint16_t ein_accumulator;
    uint16_t eout_accumulator;
    uint8_t ein_rollover_count;
    uint8_t eout_rollover_count;
    uint32_t ein_sample_count;
    uint32_t eout_sample_count;
} pmbus_app_store_t;

static pmbus_app_store_t g_pmbus_app;
static pmbus_platform_status_reader_t g_pmbus_platform_status_reader;

/* Fixed inventory placeholders for MFR_ID/MFR_MODEL/MFR_REVISION/MFR_SERIAL.
   TODO: Replace with SKU, production, or EEPROM-backed inventory strings. */
static uint8_t g_mfr_id[] = "MFR_ID_001";
static uint8_t g_mfr_model[] = "MFR_MODEL_001";
static uint8_t g_mfr_revision[] = "MFR_REV_001";
static uint8_t g_mfr_serial[] = "MFR_SERIAL_001";

static void pmbus_app_copy_bytes(uint8_t *dst, uint8_t *src, uint8_t length);
static uint16_t pmbus_app_compute_energy_step(uint32_t power_mw);
static void pmbus_app_update_fault_register(uint8_t *reg_value, uint8_t set_mask, uint8_t clear_mask);
static uint8_t pmbus_app_apply_fault_source_policy(uint8_t current_status, uint8_t source_status, uint8_t fault_mask);
static void pmbus_app_sync_alert_policy(void);
static void pmbus_app_refresh_blackbox_snapshot(void);
static void pmbus_app_refresh_status_and_optional_latch(uint8_t latch_on_fault);
static uint8_t pmbus_app_map_address_strap_to_7bit(uint8_t a0_level, uint8_t a1_level);

static void pmbus_app_copy_bytes(uint8_t *dst, uint8_t *src, uint8_t length)
{
    uint8_t index;

    for (index = 0U; index < length; index++)
    {
        dst[index] = src[index];
    }
}

static uint16_t pmbus_app_compute_energy_step(uint32_t power_mw)
{
    uint32_t sample_step;

    sample_step = power_mw / 1000UL;
    if ((sample_step == 0UL) && (power_mw != 0UL))
    {
        sample_step = 1UL;
    }

    if (sample_step > 65535UL)
    {
        sample_step = 65535UL;
    }

    return (uint16_t)sample_step;
}

static void pmbus_app_update_fault_register(uint8_t *reg_value, uint8_t set_mask, uint8_t clear_mask)
{
    *reg_value = (uint8_t)((*reg_value | set_mask) & (uint8_t)(~clear_mask));
}

static uint8_t pmbus_app_apply_fault_source_policy(uint8_t current_status, uint8_t source_status, uint8_t fault_mask)
{
    uint8_t latched_fault_bits;
    uint8_t active_fault_bits;
    uint8_t transparent_bits;

    latched_fault_bits = (uint8_t)(current_status & fault_mask);
    active_fault_bits = (uint8_t)(source_status & fault_mask);
    transparent_bits = (uint8_t)(source_status & (uint8_t)(~fault_mask));

    return (uint8_t)(latched_fault_bits | active_fault_bits | transparent_bits);
}

static void pmbus_app_sync_alert_policy(void)
{
    uint8_t has_fault;

    has_fault = 0U;

    if ((g_pmbus_app.status_vout & PMBUS_STATUS_VOUT_FAULT_MASK) != 0U)
    {
        has_fault = 1U;
    }
    else if ((g_pmbus_app.status_iout & PMBUS_STATUS_IOUT_FAULT_MASK) != 0U)
    {
        has_fault = 1U;
    }
    else if ((g_pmbus_app.status_input & PMBUS_STATUS_INPUT_FAULT_MASK) != 0U)
    {
        has_fault = 1U;
    }
    else if ((g_pmbus_app.status_temperature & PMBUS_STATUS_TEMPERATURE_FAULT_MASK) != 0U)
    {
        has_fault = 1U;
    }
    else if ((g_pmbus_app.status_fans_1_2 & PMBUS_STATUS_FANS_1_2_FAULT_MASK) != 0U)
    {
        has_fault = 1U;
    }
    else if ((g_pmbus_app.status_cml & PMBUS_STATUS_CML_FAULT_MASK) != 0U)
    {
        has_fault = 1U;
    }
    else if ((g_pmbus_app.status_other & PMBUS_STATUS_OTHER_FAULT_MASK) != 0U)
    {
        has_fault = 1U;
    }
    else if ((g_pmbus_app.status_mfr_specific & PMBUS_STATUS_MFR_SPECIFIC_FAULT_MASK) != 0U)
    {
        has_fault = 1U;
    }
    else if (g_pmbus_app.mfr_fault_active != 0U)
    {
        has_fault = 1U;
    }

    if (has_fault != 0U)
    {
        pmbus_app_assert_alert();
    }
    else
    {
        pmbus_app_release_alert();
    }
}

static uint8_t pmbus_app_map_address_strap_to_7bit(uint8_t a0_level, uint8_t a1_level)
{
    uint8_t strap_code;

    strap_code = (uint8_t)(a0_level | (uint8_t)(a1_level << 1));

    switch (strap_code)
    {
        case 0x00U:
            return PMBUS_ADDRESS_STRAP_00_7BIT;

        case 0x01U:
            return PMBUS_ADDRESS_STRAP_01_7BIT;

        case 0x02U:
            return PMBUS_ADDRESS_STRAP_10_7BIT;

        case 0x03U:
            return PMBUS_ADDRESS_STRAP_11_7BIT;

        default:
            return PMBUS_ADDRESS_INVALID_FALLBACK_7BIT;
    }
}

static long pmbus_app_div_round_signed(long numerator, long denominator)
{
    if (numerator >= 0L)
    {
        return (numerator + (denominator / 2L)) / denominator;
    }

    return -(((-numerator) + (denominator / 2L)) / denominator);
}

static int pmbus_app_clamp_linear11_mantissa(long value)
{
    if (value > 1023L)
    {
        return 1023;
    }

    if (value < -1024L)
    {
        return -1024;
    }

    return (int)value;
}

static signed char pmbus_app_clamp_linear11_exponent(int value)
{
    if (value > 15)
    {
        return 15;
    }

    if (value < -16)
    {
        return -16;
    }

    return (signed char)value;
}

static uint16_t pmbus_app_encode_linear11_fields(int mantissa, signed char exponent)
{
    uint16_t raw_mantissa;
    uint16_t raw_exponent;

    mantissa = pmbus_app_clamp_linear11_mantissa((long)mantissa);
    exponent = pmbus_app_clamp_linear11_exponent((int)exponent);

    raw_mantissa = (uint16_t)mantissa & 0x07FFU;
    raw_exponent = (uint16_t)exponent & 0x001FU;

    return (uint16_t)((raw_exponent << 11) | raw_mantissa);
}

static uint16_t pmbus_app_encode_linear11_scaled(long scaled_value, uint16_t scale_divisor)
{
    signed char exponent;
    long mantissa;
    long denominator;
    long multiplier;

    for (exponent = -16; exponent <= 15; exponent++)
    {
        if (exponent >= 0)
        {
            denominator = (long)scale_divisor * (1L << exponent);
            mantissa = pmbus_app_div_round_signed(scaled_value, denominator);
        }
        else
        {
            multiplier = 1L << (-exponent);
            mantissa = pmbus_app_div_round_signed(scaled_value * multiplier, (long)scale_divisor);
        }

        if ((mantissa >= -1024L) && (mantissa <= 1023L))
        {
            return pmbus_app_encode_linear11_fields((int)mantissa, exponent);
        }
    }

    if (scaled_value < 0L)
    {
        return pmbus_app_encode_linear11_fields(-1024, -16);
    }

    return pmbus_app_encode_linear11_fields(1023, 15);
}

static long pmbus_app_sign_extend_linear11_mantissa(uint16_t raw_value)
{
    long mantissa;

    mantissa = (long)(raw_value & 0x07FFU);
    if ((mantissa & 0x0400L) != 0L)
    {
        mantissa -= 0x0800L;
    }

    return mantissa;
}

static signed char pmbus_app_sign_extend_linear11_exponent(uint16_t raw_value)
{
    uint8_t exponent_bits;

    exponent_bits = (uint8_t)((raw_value >> 11) & 0x1FU);
    if ((exponent_bits & 0x10U) != 0U)
    {
        return (signed char)(exponent_bits | 0xE0U);
    }

    return (signed char)exponent_bits;
}

static uint32_t pmbus_app_decode_ulinear16_to_mv(uint16_t raw_value, signed char exponent)
{
    uint32_t value_mv;

    if (exponent >= 0)
    {
        value_mv = (uint32_t)raw_value * 1000UL * (1UL << exponent);
    }
    else
    {
        value_mv = ((uint32_t)raw_value * 1000UL) / (1UL << (-exponent));
    }

    return value_mv;
}

static long pmbus_app_decode_linear11_scaled(uint16_t raw_value, uint16_t scale_multiplier)
{
    long mantissa;
    signed char exponent;
    long scaled_value;

    mantissa = pmbus_app_sign_extend_linear11_mantissa(raw_value);
    exponent = pmbus_app_sign_extend_linear11_exponent(raw_value);
    scaled_value = mantissa * (long)scale_multiplier;

    if (exponent >= 0)
    {
        scaled_value *= (1L << exponent);
    }
    else
    {
        scaled_value = pmbus_app_div_round_signed(scaled_value, (1L << (-exponent)));
    }

    return scaled_value;
}

static signed char pmbus_app_decode_vout_exponent(uint8_t vout_mode)
{
    uint8_t raw_exponent;

    raw_exponent = (uint8_t)(vout_mode & 0x1FU);
    if ((raw_exponent & 0x10U) != 0U)
    {
        return (signed char)(raw_exponent | 0xE0U);
    }

    return (signed char)raw_exponent;
}

static uint16_t pmbus_app_encode_ulinear16_from_mv(uint16_t millivolts, signed char exponent)
{
    uint32_t raw_value;
    uint32_t denominator;

    if (exponent >= 0)
    {
        denominator = 1000UL * (1UL << exponent);
        raw_value = ((uint32_t)millivolts + (denominator / 2UL)) / denominator;
    }
    else
    {
        raw_value = (uint32_t)millivolts * (1UL << (-exponent));
        raw_value = (raw_value + 500UL) / 1000UL;
    }

    if (raw_value > 65535UL)
    {
        raw_value = 65535UL;
    }

    return (uint16_t)raw_value;
}

static void pmbus_app_refresh_status_word(void)
{
    uint8_t status_byte;
    uint16_t status_word;
    uint16_t mfr_status_bits;
    uint8_t has_other_summary;
    uint8_t has_mfr_summary;

    status_byte = 0U;
    status_word = 0U;
    mfr_status_bits = (uint16_t)(PMBUS_FWUPLOAD_STATUS_BAD_IMAGE | PMBUS_FWUPLOAD_STATUS_UNSUPPORTED);
    has_other_summary = 0U;
    has_mfr_summary = 0U;

    if ((g_pmbus_app.busy_state != 0U) ||
        ((g_pmbus_app.mfr_fwupload_mode & 0x01U) != 0U) ||
        ((g_pmbus_app.mfr_fwupload_status & PMBUS_FWUPLOAD_STATUS_IN_PROGRESS) != 0U))
    {
        status_byte = (uint8_t)(status_byte | PMBUS_STATUS_BYTE_BUSY);
    }

    if ((g_pmbus_app.operation & 0x80U) == 0U)
    {
        status_byte = (uint8_t)(status_byte | PMBUS_STATUS_BYTE_OFF);
    }

    if ((g_pmbus_app.status_vout & PMBUS_STATUS_VOUT_VOUT_OV_FAULT) != 0U)
    {
        status_byte = (uint8_t)(status_byte | PMBUS_STATUS_BYTE_VOUT_OV_FAULT);
    }

    if (g_pmbus_app.status_vout != 0U)
    {
        status_word = (uint16_t)(status_word | PMBUS_STATUS_WORD_VOUT);
    }

    if ((g_pmbus_app.status_iout & (PMBUS_STATUS_IOUT_IOUT_OC_FAULT | PMBUS_STATUS_IOUT_IOUT_OC_LV_FAULT)) != 0U)
    {
        status_byte = (uint8_t)(status_byte | PMBUS_STATUS_BYTE_IOUT_OC_FAULT);
    }

    if (g_pmbus_app.status_iout != 0U)
    {
        status_word = (uint16_t)(status_word | PMBUS_STATUS_WORD_IOUT_POUT);
    }

    if ((g_pmbus_app.status_input & (PMBUS_STATUS_INPUT_VIN_UV_FAULT | PMBUS_STATUS_INPUT_UNIT_OFF_FOR_INSUFFICIENT_INPUT_VOLTAGE)) != 0U)
    {
        status_byte = (uint8_t)(status_byte | PMBUS_STATUS_BYTE_VIN_UV_FAULT);
    }

    if (g_pmbus_app.status_input != 0U)
    {
        status_word = (uint16_t)(status_word | PMBUS_STATUS_WORD_INPUT);
    }

    if (g_pmbus_app.status_temperature != 0U)
    {
        status_byte = (uint8_t)(status_byte | PMBUS_STATUS_BYTE_TEMPERATURE);
    }

    if (g_pmbus_app.status_cml != 0U)
    {
        status_byte = (uint8_t)(status_byte | PMBUS_STATUS_BYTE_CML);
        status_word = (uint16_t)(status_word | PMBUS_STATUS_WORD_LOW_BYTE_CML);
    }

    if (g_pmbus_app.status_fans_1_2 != 0U)
    {
        status_word = (uint16_t)(status_word | PMBUS_STATUS_WORD_FANS);
        status_byte = (uint8_t)(status_byte | PMBUS_STATUS_BYTE_NONE_OF_THE_ABOVE);
    }

    if (g_pmbus_app.power_good_asserted == 0U)
    {
        status_word = (uint16_t)(status_word | PMBUS_STATUS_WORD_PG_STATUS);
        status_byte = (uint8_t)(status_byte | PMBUS_STATUS_BYTE_NONE_OF_THE_ABOVE);
    }

    if (g_pmbus_app.status_other != 0U)
    {
        has_other_summary = 1U;
    }

    if (g_pmbus_app.status_mfr_specific != 0U)
    {
        has_mfr_summary = 1U;
    }

    if ((g_pmbus_app.mfr_fwupload_status & mfr_status_bits) != 0U)
    {
        has_mfr_summary = 1U;
    }

    if (g_pmbus_app.mfr_fault_active != 0U)
    {
        has_mfr_summary = 1U;
    }

    if (has_mfr_summary != 0U)
    {
        status_word = (uint16_t)(status_word | PMBUS_STATUS_WORD_MFR_SPECIFIC);
        status_byte = (uint8_t)(status_byte | PMBUS_STATUS_BYTE_NONE_OF_THE_ABOVE);
    }

    if ((g_pmbus_app.comm_bus_released == 0U) || (g_pmbus_app.comm_recover_fail_count != 0U))
    {
        has_other_summary = 1U;
    }

    if (has_other_summary != 0U)
    {
        status_word = (uint16_t)(status_word | PMBUS_STATUS_WORD_OTHER);
        status_byte = (uint8_t)(status_byte | PMBUS_STATUS_BYTE_NONE_OF_THE_ABOVE);
    }

    if (g_pmbus_app.address_valid == 0U)
    {
        status_word = (uint16_t)(status_word | PMBUS_STATUS_WORD_UNKNOWN);
        status_byte = (uint8_t)(status_byte | PMBUS_STATUS_BYTE_NONE_OF_THE_ABOVE);
    }

    status_word = (uint16_t)(status_word | status_byte);

    g_pmbus_app.status_byte = status_byte;
    g_pmbus_app.status_word = status_word;
}

static void pmbus_app_update_energy_shadow(uint8_t *buffer,
    uint16_t *accumulator,
    uint8_t *rollover_count,
    uint32_t *sample_count,
    uint16_t sample_step)
{
    uint32_t next_accumulator;

    next_accumulator = (uint32_t)(*accumulator) + (uint32_t)sample_step;
    if (next_accumulator > 0xFFFFUL)
    {
        next_accumulator &= 0xFFFFUL;
        *rollover_count = (uint8_t)(*rollover_count + 1U);
    }

    *accumulator = (uint16_t)next_accumulator;
    *sample_count = (*sample_count + 1UL) & 0x00FFFFFFUL;

    buffer[0] = (uint8_t)(*accumulator & 0x00FFU);
    buffer[1] = (uint8_t)((*accumulator >> 8) & 0x00FFU);
    buffer[2] = *rollover_count;
    buffer[3] = (uint8_t)(*sample_count & 0x0000FFUL);
    buffer[4] = (uint8_t)((*sample_count >> 8) & 0x0000FFUL);
    buffer[5] = (uint8_t)((*sample_count >> 16) & 0x0000FFUL);
}

static void pmbus_app_refresh_blackbox_snapshot(void)
{
    g_pmbus_app.mfr_blackbox_live[0] = 0x02U;
    g_pmbus_app.mfr_blackbox_live[1] = g_pmbus_app.status_byte;
    g_pmbus_app.mfr_blackbox_live[2] = (uint8_t)(g_pmbus_app.status_word & 0x00FFU);
    g_pmbus_app.mfr_blackbox_live[3] = (uint8_t)((g_pmbus_app.status_word >> 8) & 0x00FFU);
    g_pmbus_app.mfr_blackbox_live[4] = g_pmbus_app.status_vout;
    g_pmbus_app.mfr_blackbox_live[5] = g_pmbus_app.status_iout;
    g_pmbus_app.mfr_blackbox_live[6] = g_pmbus_app.status_input;
    g_pmbus_app.mfr_blackbox_live[7] = g_pmbus_app.status_temperature;
    g_pmbus_app.mfr_blackbox_live[8] = g_pmbus_app.status_cml;
    g_pmbus_app.mfr_blackbox_live[9] = g_pmbus_app.status_fans_1_2;
    g_pmbus_app.mfr_blackbox_live[10] = (uint8_t)(g_pmbus_app.read_vin & 0x00FFU);
    g_pmbus_app.mfr_blackbox_live[11] = (uint8_t)((g_pmbus_app.read_vin >> 8) & 0x00FFU);
    g_pmbus_app.mfr_blackbox_live[12] = (uint8_t)(g_pmbus_app.read_vout & 0x00FFU);
    g_pmbus_app.mfr_blackbox_live[13] = (uint8_t)((g_pmbus_app.read_vout >> 8) & 0x00FFU);
    g_pmbus_app.mfr_blackbox_live[14] = (uint8_t)(g_pmbus_app.read_iout & 0x00FFU);
    g_pmbus_app.mfr_blackbox_live[15] = (uint8_t)((g_pmbus_app.read_iout >> 8) & 0x00FFU);
    g_pmbus_app.mfr_blackbox_live[16] = (uint8_t)(g_pmbus_app.read_temperature_1 & 0x00FFU);
    g_pmbus_app.mfr_blackbox_live[17] = (uint8_t)((g_pmbus_app.read_temperature_1 >> 8) & 0x00FFU);
    g_pmbus_app.mfr_blackbox_live[18] = (uint8_t)(g_pmbus_app.read_temperature_2 & 0x00FFU);
    g_pmbus_app.mfr_blackbox_live[19] = (uint8_t)((g_pmbus_app.read_temperature_2 >> 8) & 0x00FFU);
    g_pmbus_app.mfr_blackbox_live[20] = (uint8_t)(g_pmbus_app.read_temperature_3 & 0x00FFU);
    g_pmbus_app.mfr_blackbox_live[21] = (uint8_t)((g_pmbus_app.read_temperature_3 >> 8) & 0x00FFU);
    g_pmbus_app.mfr_blackbox_live[22] = (uint8_t)(g_pmbus_app.read_fan_speed_1 & 0x00FFU);
    g_pmbus_app.mfr_blackbox_live[23] = (uint8_t)((g_pmbus_app.read_fan_speed_1 >> 8) & 0x00FFU);
    g_pmbus_app.mfr_blackbox_live[24] = (uint8_t)(g_pmbus_app.read_fan_speed_2 & 0x00FFU);
    g_pmbus_app.mfr_blackbox_live[25] = (uint8_t)((g_pmbus_app.read_fan_speed_2 >> 8) & 0x00FFU);
    g_pmbus_app.mfr_blackbox_live[26] = (uint8_t)(g_pmbus_app.read_pin & 0x00FFU);
    g_pmbus_app.mfr_blackbox_live[27] = (uint8_t)((g_pmbus_app.read_pin >> 8) & 0x00FFU);
    g_pmbus_app.mfr_blackbox_live[28] = (uint8_t)(g_pmbus_app.read_pout & 0x00FFU);
    g_pmbus_app.mfr_blackbox_live[29] = (uint8_t)((g_pmbus_app.read_pout >> 8) & 0x00FFU);
    g_pmbus_app.mfr_blackbox_live[30] = (uint8_t)(((g_pmbus_app.address_valid & 0x01U) << 7) |
        ((g_pmbus_app.comm_bus_released & 0x01U) << 6) |
        ((g_pmbus_app.comm_recover_fail_count & 0x03U) << 4) |
        ((g_pmbus_app.address_a1_raw & 0x03U) << 2) |
        (g_pmbus_app.address_a0_raw & 0x03U));
    g_pmbus_app.mfr_blackbox_live[31] = (uint8_t)(((g_pmbus_app.comm_recover_count & 0x0FU) << 4) |
        (g_pmbus_app.comm_last_recover_reason & 0x0FU));
}

static void pmbus_app_refresh_status_and_optional_latch(uint8_t latch_on_fault)
{
    pmbus_app_refresh_status_word();
    pmbus_app_sync_alert_policy();
    pmbus_app_refresh_blackbox_snapshot();

    if ((latch_on_fault != 0U) && (g_pmbus_app.blackbox_latched == 0U))
    {
        pmbus_app_copy_bytes(g_pmbus_app.mfr_blackbox_latched,
            g_pmbus_app.mfr_blackbox_live,
            PMBUS_BLACKBOX_BLOCK_SIZE);
        g_pmbus_app.blackbox_latched = 1U;
    }
}

static void pmbus_app_refresh_shadow_data(void)
{
    signed char vout_exponent;
    uint16_t ein_sample_step;
    uint16_t eout_sample_step;
    uint8_t effective_vout_source;
    uint8_t effective_iout_source;
    uint8_t effective_input_source;
    uint8_t effective_temperature_source;
    uint8_t effective_fans_source;
    uint32_t vout_ov_fault_limit_mv;
    uint32_t vout_ov_warn_limit_mv;
    uint32_t vout_uv_warn_limit_mv;
    uint32_t vout_uv_fault_limit_mv;
    long vin_ov_fault_limit_mv;
    long vin_ov_warn_limit_mv;
    long vin_uv_warn_limit_mv;
    long vin_uv_fault_limit_mv;
    long iout_oc_fault_limit_ma;
    long iout_oc_warn_limit_ma;
    long iin_oc_fault_limit_ma;
    long iin_oc_warn_limit_ma;
    long ot_fault_limit_mc;
    long ot_warn_limit_mc;
    long fan_command_1_rpm;
    long fan_command_2_rpm;
    uint32_t hottest_temp_mc;

    vout_exponent = pmbus_app_decode_vout_exponent(g_pmbus_app.vout_mode);

    g_pmbus_app.read_vin = pmbus_app_encode_linear11_scaled((long)g_pmbus_app.source_vin_mv, 1000U);
    g_pmbus_app.read_iin = pmbus_app_encode_linear11_scaled((long)g_pmbus_app.source_iin_ma, 1000U);
    g_pmbus_app.read_vout = pmbus_app_encode_ulinear16_from_mv((uint16_t)g_pmbus_app.source_vout_mv, vout_exponent);
    g_pmbus_app.read_iout = pmbus_app_encode_linear11_scaled((long)g_pmbus_app.source_iout_ma, 1000U);
    g_pmbus_app.read_temperature_1 = pmbus_app_encode_linear11_scaled((long)g_pmbus_app.source_temp1_mc, 1000U);
    g_pmbus_app.read_temperature_2 = pmbus_app_encode_linear11_scaled((long)g_pmbus_app.source_temp2_mc, 1000U);
    g_pmbus_app.read_temperature_3 = pmbus_app_encode_linear11_scaled((long)g_pmbus_app.source_temp3_mc, 1000U);
    g_pmbus_app.read_fan_speed_1 = pmbus_app_encode_linear11_scaled((long)g_pmbus_app.source_fan1_rpm, 1U);
    g_pmbus_app.read_fan_speed_2 = pmbus_app_encode_linear11_scaled((long)g_pmbus_app.source_fan2_rpm, 1U);
    g_pmbus_app.read_pout = pmbus_app_encode_linear11_scaled((long)g_pmbus_app.source_pout_mw, 1000U);
    g_pmbus_app.read_pin = pmbus_app_encode_linear11_scaled((long)g_pmbus_app.source_pin_mw, 1000U);

    effective_vout_source = g_pmbus_app.status_vout_source;
    effective_iout_source = g_pmbus_app.status_iout_source;
    effective_input_source = g_pmbus_app.status_input_source;
    effective_temperature_source = g_pmbus_app.status_temperature_source;
    effective_fans_source = g_pmbus_app.status_fans_1_2_source;

    vout_ov_fault_limit_mv = pmbus_app_decode_ulinear16_to_mv(g_pmbus_app.vout_ov_fault_limit, vout_exponent);
    vout_ov_warn_limit_mv = pmbus_app_decode_ulinear16_to_mv(g_pmbus_app.vout_ov_warn_limit, vout_exponent);
    vout_uv_warn_limit_mv = pmbus_app_decode_ulinear16_to_mv(g_pmbus_app.vout_uv_warn_limit, vout_exponent);
    vout_uv_fault_limit_mv = pmbus_app_decode_ulinear16_to_mv(g_pmbus_app.vout_uv_fault_limit, vout_exponent);
    vin_ov_fault_limit_mv = pmbus_app_decode_linear11_scaled(g_pmbus_app.vin_ov_fault_limit, 1000U);
    vin_ov_warn_limit_mv = pmbus_app_decode_linear11_scaled(g_pmbus_app.vin_ov_warn_limit, 1000U);
    vin_uv_warn_limit_mv = pmbus_app_decode_linear11_scaled(g_pmbus_app.vin_uv_warn_limit, 1000U);
    vin_uv_fault_limit_mv = pmbus_app_decode_linear11_scaled(g_pmbus_app.vin_uv_fault_limit, 1000U);
    iout_oc_fault_limit_ma = pmbus_app_decode_linear11_scaled(g_pmbus_app.iout_oc_fault_limit, 1000U);
    iout_oc_warn_limit_ma = pmbus_app_decode_linear11_scaled(g_pmbus_app.iout_oc_warn_limit, 1000U);
    iin_oc_fault_limit_ma = pmbus_app_decode_linear11_scaled(g_pmbus_app.iin_oc_fault_limit, 1000U);
    iin_oc_warn_limit_ma = pmbus_app_decode_linear11_scaled(g_pmbus_app.iin_oc_warn_limit, 1000U);
    ot_fault_limit_mc = pmbus_app_decode_linear11_scaled(g_pmbus_app.ot_fault_limit, 1000U);
    ot_warn_limit_mc = pmbus_app_decode_linear11_scaled(g_pmbus_app.ot_warn_limit, 1000U);
    fan_command_1_rpm = pmbus_app_decode_linear11_scaled(g_pmbus_app.fan_command_1, 1U);
    fan_command_2_rpm = pmbus_app_decode_linear11_scaled(g_pmbus_app.fan_command_2, 1U);

    if (g_pmbus_app.source_vout_mv >= vout_ov_fault_limit_mv)
    {
        effective_vout_source = (uint8_t)(effective_vout_source | PMBUS_STATUS_VOUT_VOUT_OV_FAULT);
    }
    else if (g_pmbus_app.source_vout_mv >= vout_ov_warn_limit_mv)
    {
        effective_vout_source = (uint8_t)(effective_vout_source | PMBUS_STATUS_VOUT_VOUT_OV_WARNING);
    }

    if (g_pmbus_app.source_vout_mv <= vout_uv_fault_limit_mv)
    {
        effective_vout_source = (uint8_t)(effective_vout_source | PMBUS_STATUS_VOUT_VOUT_UV_FAULT);
    }
    else if (g_pmbus_app.source_vout_mv <= vout_uv_warn_limit_mv)
    {
        effective_vout_source = (uint8_t)(effective_vout_source | PMBUS_STATUS_VOUT_VOUT_UV_WARNING);
    }

    if ((long)g_pmbus_app.source_iout_ma >= iout_oc_fault_limit_ma)
    {
        effective_iout_source = (uint8_t)(effective_iout_source | PMBUS_STATUS_IOUT_IOUT_OC_FAULT);
    }
    else if ((long)g_pmbus_app.source_iout_ma >= iout_oc_warn_limit_ma)
    {
        effective_iout_source = (uint8_t)(effective_iout_source | PMBUS_STATUS_IOUT_IOUT_OC_WARNING);
    }

    if ((long)g_pmbus_app.source_vin_mv >= vin_ov_fault_limit_mv)
    {
        effective_input_source = (uint8_t)(effective_input_source | PMBUS_STATUS_INPUT_VIN_OV_FAULT);
    }
    else if ((long)g_pmbus_app.source_vin_mv >= vin_ov_warn_limit_mv)
    {
        effective_input_source = (uint8_t)(effective_input_source | PMBUS_STATUS_INPUT_VIN_OV_WARNING);
    }

    if ((long)g_pmbus_app.source_vin_mv <= vin_uv_fault_limit_mv)
    {
        effective_input_source = (uint8_t)(effective_input_source | PMBUS_STATUS_INPUT_VIN_UV_FAULT);
    }
    else if ((long)g_pmbus_app.source_vin_mv <= vin_uv_warn_limit_mv)
    {
        effective_input_source = (uint8_t)(effective_input_source | PMBUS_STATUS_INPUT_VIN_UV_WARNING);
    }

    if ((long)g_pmbus_app.source_iin_ma >= iin_oc_fault_limit_ma)
    {
        effective_input_source = (uint8_t)(effective_input_source | PMBUS_STATUS_INPUT_IIN_OC_FAULT);
    }
    else if ((long)g_pmbus_app.source_iin_ma >= iin_oc_warn_limit_ma)
    {
        effective_input_source = (uint8_t)(effective_input_source | PMBUS_STATUS_INPUT_IIN_OC_WARNING);
    }

    hottest_temp_mc = g_pmbus_app.source_temp1_mc;
    if (g_pmbus_app.source_temp2_mc > hottest_temp_mc)
    {
        hottest_temp_mc = g_pmbus_app.source_temp2_mc;
    }
    if (g_pmbus_app.source_temp3_mc > hottest_temp_mc)
    {
        hottest_temp_mc = g_pmbus_app.source_temp3_mc;
    }

    if ((long)hottest_temp_mc >= ot_fault_limit_mc)
    {
        effective_temperature_source = (uint8_t)(effective_temperature_source | PMBUS_STATUS_TEMPERATURE_OT_FAULT);
    }
    else if ((long)hottest_temp_mc >= ot_warn_limit_mc)
    {
        effective_temperature_source = (uint8_t)(effective_temperature_source | PMBUS_STATUS_TEMPERATURE_OT_WARNING);
    }

    if (fan_command_1_rpm > 0L)
    {
        if ((long)g_pmbus_app.source_fan1_rpm < ((fan_command_1_rpm * 8L) / 10L))
        {
            effective_fans_source = (uint8_t)(effective_fans_source | PMBUS_STATUS_FANS_1_2_FAN_1_FAULT);
        }
        else if ((long)g_pmbus_app.source_fan1_rpm < ((fan_command_1_rpm * 9L) / 10L))
        {
            effective_fans_source = (uint8_t)(effective_fans_source | PMBUS_STATUS_FANS_1_2_FAN_1_WARNING);
        }
    }

    if (fan_command_2_rpm > 0L)
    {
        if ((long)g_pmbus_app.source_fan2_rpm < ((fan_command_2_rpm * 8L) / 10L))
        {
            effective_fans_source = (uint8_t)(effective_fans_source | PMBUS_STATUS_FANS_1_2_FAN_2_FAULT);
        }
        else if ((long)g_pmbus_app.source_fan2_rpm < ((fan_command_2_rpm * 9L) / 10L))
        {
            effective_fans_source = (uint8_t)(effective_fans_source | PMBUS_STATUS_FANS_1_2_FAN_2_WARNING);
        }
    }

    ein_sample_step = pmbus_app_compute_energy_step(g_pmbus_app.source_pin_mw);
    eout_sample_step = pmbus_app_compute_energy_step(g_pmbus_app.source_pout_mw);

    pmbus_app_update_energy_shadow(g_pmbus_app.read_ein,
        &g_pmbus_app.ein_accumulator,
        &g_pmbus_app.ein_rollover_count,
        &g_pmbus_app.ein_sample_count,
        ein_sample_step);

    pmbus_app_update_energy_shadow(g_pmbus_app.read_eout,
        &g_pmbus_app.eout_accumulator,
        &g_pmbus_app.eout_rollover_count,
        &g_pmbus_app.eout_sample_count,
        eout_sample_step);

    g_pmbus_app.status_vout = pmbus_app_apply_fault_source_policy(g_pmbus_app.status_vout,
        effective_vout_source,
        PMBUS_STATUS_VOUT_FAULT_MASK);
    g_pmbus_app.status_iout = pmbus_app_apply_fault_source_policy(g_pmbus_app.status_iout,
        effective_iout_source,
        PMBUS_STATUS_IOUT_FAULT_MASK);
    g_pmbus_app.status_input = pmbus_app_apply_fault_source_policy(g_pmbus_app.status_input,
        effective_input_source,
        PMBUS_STATUS_INPUT_FAULT_MASK);
    g_pmbus_app.status_temperature = pmbus_app_apply_fault_source_policy(g_pmbus_app.status_temperature,
        effective_temperature_source,
        PMBUS_STATUS_TEMPERATURE_FAULT_MASK);
    g_pmbus_app.status_fans_1_2 = pmbus_app_apply_fault_source_policy(g_pmbus_app.status_fans_1_2,
        effective_fans_source,
        PMBUS_STATUS_FANS_1_2_FAULT_MASK);
    g_pmbus_app.status_cml = pmbus_app_apply_fault_source_policy(g_pmbus_app.status_cml,
        g_pmbus_app.status_cml_source,
        PMBUS_STATUS_CML_FAULT_MASK);
    g_pmbus_app.status_other = pmbus_app_apply_fault_source_policy(g_pmbus_app.status_other,
        g_pmbus_app.status_other_source,
        PMBUS_STATUS_OTHER_FAULT_MASK);
    g_pmbus_app.status_mfr_specific = pmbus_app_apply_fault_source_policy(g_pmbus_app.status_mfr_specific,
        g_pmbus_app.status_mfr_specific_source,
        PMBUS_STATUS_MFR_SPECIFIC_FAULT_MASK);

    pmbus_app_refresh_status_and_optional_latch(0U);
}

void pmbus_app_init(void)
{
    g_pmbus_app.slave_address_7bit = pmbus_app_detect_slave_address_7bit();
    /* Core command defaults. These are writable PMBus shadows until a product
       policy owner maps them to real rail/page/operation control behavior. */
    g_pmbus_app.page = 0x00U;
    g_pmbus_app.operation = 0x80U;
    g_pmbus_app.on_off_config = 0x1AU;
    g_pmbus_app.phase = 0x00U;
    /* PAGE_PLUS_* and STORE/RESTORE are portable host-compatibility shadows.
       TODO: Bind PAGE_PLUS to real multi-page rails and STORE/RESTORE to
       product NVM with wear/error policy before relying on persistence. */
    g_pmbus_app.page_plus_last_page = 0x00U;
    g_pmbus_app.page_plus_last_command = 0x00U;
    g_pmbus_app.page_plus_last_length = 0x00U;
    g_pmbus_app.store_restore_last_command = 0x00U;
    g_pmbus_app.store_restore_last_code = 0x00U;
    g_pmbus_app.store_restore_count = 0x0000U;
    g_pmbus_app.write_protect = 0x00U;
    /* Fan 1/2 defaults are portable telemetry-policy test values.
       TODO: Replace with real fan controller/tachometer integration. */
    g_pmbus_app.fan_config_1_2 = 0x00U;
    g_pmbus_app.fan_command_1 = pmbus_app_encode_linear11_scaled(12000L, 1U);
    g_pmbus_app.fan_command_2 = pmbus_app_encode_linear11_scaled(11800L, 1U);
    /* FAN_CONFIG_3_4/FAN_COMMAND_3/FAN_COMMAND_4 are fixed shadows for
       host compatibility. TODO: Bind to real fan 3/4 hardware if present. */
    g_pmbus_app.fan_config_3_4 = 0x00U;
    g_pmbus_app.fan_command_3 = 0x0000U;
    g_pmbus_app.fan_command_4 = 0x0000U;
    /* VOUT_MODE and limit defaults are portable CRPS-like placeholder values.
       TODO: Replace or validate against final PSU voltage/current limits. */
    g_pmbus_app.vout_mode = 0x17U;
    g_pmbus_app.vout_ov_fault_limit = pmbus_app_encode_ulinear16_from_mv(13200U, -9);
    g_pmbus_app.vout_ov_warn_limit = pmbus_app_encode_ulinear16_from_mv(12600U, -9);
    g_pmbus_app.vout_uv_warn_limit = pmbus_app_encode_ulinear16_from_mv(11400U, -9);
    g_pmbus_app.vout_uv_fault_limit = pmbus_app_encode_ulinear16_from_mv(10800U, -9);
    g_pmbus_app.iout_oc_fault_limit = pmbus_app_encode_linear11_scaled(25000L, 1000U);
    g_pmbus_app.iout_oc_lv_fault_limit = pmbus_app_encode_linear11_scaled(1000L, 1000U);
    g_pmbus_app.iout_oc_warn_limit = pmbus_app_encode_linear11_scaled(22000L, 1000U);
    g_pmbus_app.iout_uc_fault_limit = 0x0000U;
    g_pmbus_app.ot_fault_limit = pmbus_app_encode_linear11_scaled(100000L, 1000U);
    g_pmbus_app.ot_warn_limit = pmbus_app_encode_linear11_scaled(90000L, 1000U);
    g_pmbus_app.ut_warn_limit = pmbus_app_encode_linear11_scaled(0L, 1000U);
    g_pmbus_app.ut_fault_limit = pmbus_app_encode_linear11_scaled(-10000L, 1000U);
    g_pmbus_app.vin_ov_fault_limit = pmbus_app_encode_linear11_scaled(264000L, 1000U);
    g_pmbus_app.vin_ov_warn_limit = pmbus_app_encode_linear11_scaled(250000L, 1000U);
    g_pmbus_app.vin_uv_warn_limit = pmbus_app_encode_linear11_scaled(180000L, 1000U);
    g_pmbus_app.vin_uv_fault_limit = pmbus_app_encode_linear11_scaled(170000L, 1000U);
    g_pmbus_app.iin_oc_fault_limit = pmbus_app_encode_linear11_scaled(3000L, 1000U);
    g_pmbus_app.iin_oc_warn_limit = pmbus_app_encode_linear11_scaled(2500L, 1000U);
    /* Fault response command defaults are response-policy shadows only.
       TODO: Map these bytes to real shutdown/retry/latch platform behavior. */
    g_pmbus_app.vout_ov_fault_response = 0x00U;
    g_pmbus_app.vout_uv_fault_response = 0x00U;
    g_pmbus_app.iout_oc_fault_response = 0x00U;
    g_pmbus_app.iout_oc_lv_fault_response = 0x00U;
    g_pmbus_app.iout_uc_fault_response = 0x00U;
    g_pmbus_app.ot_fault_response = 0x00U;
    g_pmbus_app.ut_fault_response = 0x00U;
    g_pmbus_app.vin_ov_fault_response = 0x00U;
    g_pmbus_app.vin_uv_fault_response = 0x00U;
    g_pmbus_app.iin_oc_fault_response = 0x00U;
    g_pmbus_app.ton_max_fault_response = 0x00U;
    g_pmbus_app.pout_op_fault_response = 0x00U;
    g_pmbus_app.zone_config = 0x0000U;
    g_pmbus_app.zone_active = 0x0000U;
    /* VOUT programming commands are stable read/write shadows for host
       validation. TODO: Connect writes to the real voltage-loop owner. */
    g_pmbus_app.vout_command = 0x1800U;
    g_pmbus_app.vout_trim = 0x0000U;
    g_pmbus_app.vout_cal_offset = 0x0000U;
    g_pmbus_app.vout_max = pmbus_app_encode_ulinear16_from_mv(13200U, -9);
    g_pmbus_app.vout_margin_high = pmbus_app_encode_ulinear16_from_mv(12600U, -9);
    g_pmbus_app.vout_margin_low = pmbus_app_encode_ulinear16_from_mv(11400U, -9);
    g_pmbus_app.vout_transition_rate = 0x0000U;
    g_pmbus_app.vout_droop = 0x0000U;
    g_pmbus_app.vout_scale_loop = 0x0000U;
    g_pmbus_app.vout_scale_monitor = 0x0000U;
    g_pmbus_app.vout_min = pmbus_app_encode_ulinear16_from_mv(10800U, -9);
    /* Fixed/shadow Table 31 expansion values:
       COEFFICIENTS, POUT_MAX, MAX_DUTY, FREQUENCY_SWITCH, POWER_MODE,
       VIN_ON, VIN_OFF, INTERLEAVE, IOUT_CAL_GAIN, IOUT_CAL_OFFSET,
       POWER_GOOD_ON, POWER_GOOD_OFF, TON_DELAY, TON_RISE,
       TON_MAX_FAULT_LIMIT, TOFF_DELAY, TOFF_FALL, TOFF_MAX_WARN_LIMIT,
       POUT_OP_FAULT_LIMIT, POUT_OP_WARN_LIMIT, PIN_OP_WARN_LIMIT.
       TODO: Replace these portable values with real CRPS control-loop,
       telemetry, calibration, sequencer, and DIRECT-format coefficient data. */
    g_pmbus_app.coefficients[0] = 0x01U;
    g_pmbus_app.coefficients[1] = 0x00U;
    g_pmbus_app.coefficients[2] = 0x00U;
    g_pmbus_app.coefficients[3] = 0x00U;
    g_pmbus_app.coefficients[4] = 0x00U;
    g_pmbus_app.pout_max = pmbus_app_encode_linear11_scaled(300000L, 1000U);
    g_pmbus_app.max_duty = pmbus_app_encode_linear11_scaled(100000L, 1000U);
    g_pmbus_app.frequency_switch = pmbus_app_encode_linear11_scaled(100000L, 1U);
    g_pmbus_app.power_mode = 0x00U;
    g_pmbus_app.vin_on = pmbus_app_encode_linear11_scaled(180000L, 1000U);
    g_pmbus_app.vin_off = pmbus_app_encode_linear11_scaled(170000L, 1000U);
    g_pmbus_app.interleave = 0x0000U;
    g_pmbus_app.iout_cal_gain = 0x0000U;
    g_pmbus_app.iout_cal_offset = 0x0000U;
    g_pmbus_app.power_good_on = pmbus_app_encode_ulinear16_from_mv(11000U, -9);
    g_pmbus_app.power_good_off = pmbus_app_encode_ulinear16_from_mv(10000U, -9);
    g_pmbus_app.ton_delay = pmbus_app_encode_linear11_scaled(100L, 1U);
    g_pmbus_app.ton_rise = pmbus_app_encode_linear11_scaled(200L, 1U);
    g_pmbus_app.ton_max_fault_limit = pmbus_app_encode_linear11_scaled(500L, 1U);
    g_pmbus_app.toff_delay = pmbus_app_encode_linear11_scaled(100L, 1U);
    g_pmbus_app.toff_fall = pmbus_app_encode_linear11_scaled(200L, 1U);
    g_pmbus_app.toff_max_warn_limit = pmbus_app_encode_linear11_scaled(500L, 1U);
    g_pmbus_app.pout_op_fault_limit = pmbus_app_encode_linear11_scaled(320000L, 1000U);
    g_pmbus_app.pout_op_warn_limit = pmbus_app_encode_linear11_scaled(300000L, 1000U);
    g_pmbus_app.pin_op_warn_limit = pmbus_app_encode_linear11_scaled(330000L, 1000U);
    g_pmbus_app.smbalert_mask = 0x0000U;
    /* Status command defaults are zero/idle until platform status or PMBus
       fault handlers update them. Commands affected: STATUS_BYTE,
       STATUS_WORD, STATUS_VOUT, STATUS_IOUT, STATUS_INPUT,
       STATUS_TEMPERATURE, STATUS_CML, STATUS_OTHER, STATUS_MFR_SPECIFIC,
       STATUS_FANS_1_2. */
    g_pmbus_app.status_byte = 0x00U;
    g_pmbus_app.status_word = 0x0000U;
    g_pmbus_app.status_vout = 0x00U;
    g_pmbus_app.status_iout = 0x00U;
    g_pmbus_app.status_input = 0x00U;
    g_pmbus_app.status_temperature = 0x00U;
    g_pmbus_app.status_cml = 0x00U;
    g_pmbus_app.status_other = 0x00U;
    g_pmbus_app.status_mfr_specific = 0x00U;
    g_pmbus_app.status_fans_1_2 = 0x00U;
    g_pmbus_app.status_vout_source = 0x00U;
    g_pmbus_app.status_iout_source = 0x00U;
    g_pmbus_app.status_input_source = 0x00U;
    g_pmbus_app.status_temperature_source = 0x00U;
    g_pmbus_app.status_cml_source = 0x00U;
    g_pmbus_app.status_other_source = 0x00U;
    g_pmbus_app.status_mfr_specific_source = 0x00U;
    g_pmbus_app.status_fans_1_2_source = 0x00U;
    g_pmbus_app.blackbox_latched = 0U;
    g_pmbus_app.busy_state = 0U;
    g_pmbus_app.power_good_asserted = 1U;
    g_pmbus_app.mfr_fault_active = 0U;
    g_pmbus_app.comm_last_recover_reason = 0U;
    g_pmbus_app.comm_recover_count = 0U;
    g_pmbus_app.comm_recover_fail_count = 0U;
    g_pmbus_app.comm_bus_released = 1U;
    /* Vendor command defaults are portable test shadows.
       TODO: Replace with production cold-redundancy and firmware-upload policy. */
    g_pmbus_app.mfr_cold_redundancy_config = 0x00U;
    g_pmbus_app.mfr_fwupload_mode = 0x00U;
    g_pmbus_app.mfr_fwupload_status = 0x0000U;
    g_pmbus_app.mfr_fwupload_last_block_len = 0U;
    g_pmbus_app.mfr_fwupload_block_count = 0U;
    g_pmbus_app.mfr_fwupload_expected_sequence = 0U;
    g_pmbus_app.mfr_fwupload_last_sequence = 0U;
    g_pmbus_app.mfr_fwupload_final_seen = 0U;
#if PMBUS_ENABLE_PEC
    /* CAPABILITY advertises PEC support based on build-time configuration.
       TODO: Keep this aligned with any future runtime PEC mode policy. */
    g_pmbus_app.capability = 0x80U;
#else
    /* CAPABILITY advertises PEC support based on build-time configuration.
       TODO: Keep this aligned with any future runtime PEC mode policy. */
    g_pmbus_app.capability = 0x00U;
#endif
    g_pmbus_app.alert_asserted = 0U;
    g_pmbus_app.ein_accumulator = 0U;
    g_pmbus_app.eout_accumulator = 0U;
    g_pmbus_app.ein_rollover_count = 0U;
    g_pmbus_app.eout_rollover_count = 0U;
    g_pmbus_app.ein_sample_count = 0UL;
    g_pmbus_app.eout_sample_count = 0UL;
    /* Default telemetry fallback values used until pmbus_platform_status_reader
       or measurement tasks update the sources. Commands affected:
       READ_VIN, READ_IIN, READ_VOUT, READ_IOUT, READ_TEMPERATURE_1/2/3,
       READ_FAN_SPEED_1/2, READ_POUT, READ_PIN, READ_EIN, READ_EOUT.
       TODO: Feed these from ADC/control-loop/fan/energy accumulation tasks. */
    g_pmbus_app.source_vin_mv = 230000UL;
    g_pmbus_app.source_iin_ma = 1250UL;
    g_pmbus_app.source_vout_mv = 12000UL;
    g_pmbus_app.source_iout_ma = 18500UL;
    g_pmbus_app.source_temp1_mc = 35000UL;
    g_pmbus_app.source_temp2_mc = 43000UL;
    g_pmbus_app.source_temp3_mc = 50000UL;
    g_pmbus_app.source_fan1_rpm = 12000UL;
    g_pmbus_app.source_fan2_rpm = 11800UL;
    g_pmbus_app.source_pout_mw = 222000UL;
    g_pmbus_app.source_pin_mw = 276000UL;

    pmbus_app_refresh_shadow_data();
    pmbus_app_set_power_good_state(1U);
}

void pmbus_app_register_platform_status_reader(pmbus_platform_status_reader_t reader)
{
    g_pmbus_platform_status_reader = reader;
}

void pmbus_app_apply_platform_status(pmbus_platform_status_t *platform_status)
{
    if (platform_status == 0)
    {
        return;
    }

    pmbus_app_set_power_good_state(platform_status->power_good_asserted);
    pmbus_app_set_mfr_fault_state(platform_status->mfr_fault_active);
    pmbus_app_update_status_vout_source(platform_status->status_vout_source);
    pmbus_app_update_status_iout_source(platform_status->status_iout_source);
    pmbus_app_update_status_input_source(platform_status->status_input_source);
    pmbus_app_update_status_temperature_source(platform_status->status_temperature_source);
    pmbus_app_update_status_cml_source(platform_status->status_cml_source);
    pmbus_app_update_status_other_source(platform_status->status_other_source);
    pmbus_app_update_status_mfr_specific_source(platform_status->status_mfr_specific_source);
    pmbus_app_update_status_fans_1_2_source(platform_status->status_fans_1_2_source);
}

void pmbus_app_background_task(void)
{
    pmbus_platform_status_t *platform_status;

    platform_status = 0;

    if (g_pmbus_platform_status_reader != 0)
    {
        platform_status = g_pmbus_platform_status_reader();
    }

    if (platform_status != 0)
    {
        pmbus_app_apply_platform_status(platform_status);
    }

    pmbus_app_refresh_shadow_data();
}

unsigned char pmbus_app_detect_slave_address_7bit(void)
{
    uint8_t a0_raw;
    uint8_t a1_raw;
    uint8_t a0_level;
    uint8_t a1_level;

    pmbus_io_init_address_pins();

    a0_raw = (uint8_t)pmbus_io_read_address_a0();
    a1_raw = (uint8_t)pmbus_io_read_address_a1();
    g_pmbus_app.address_a0_raw = a0_raw;
    g_pmbus_app.address_a1_raw = a1_raw;

    if ((a0_raw > 1U) || (a1_raw > 1U))
    {
        g_pmbus_app.address_valid = 0U;
        return PMBUS_ADDRESS_INVALID_FALLBACK_7BIT;
    }

    g_pmbus_app.address_valid = 1U;
    a0_level = (uint8_t)(a0_raw & 0x01U);
    a1_level = (uint8_t)(a1_raw & 0x01U);

    return pmbus_app_map_address_strap_to_7bit(a0_level, a1_level);
}

unsigned char pmbus_app_get_address_valid(void)
{
    return g_pmbus_app.address_valid;
}

void pmbus_app_set_slave_address_7bit(unsigned char value)
{
    g_pmbus_app.slave_address_7bit = value;
}

unsigned char pmbus_app_get_slave_address_7bit(void)
{
    return g_pmbus_app.slave_address_7bit;
}

void pmbus_app_clear_faults(void)
{
    g_pmbus_app.status_vout = (uint8_t)(g_pmbus_app.status_vout_source & (uint8_t)(~PMBUS_STATUS_VOUT_FAULT_MASK));
    g_pmbus_app.status_iout = (uint8_t)(g_pmbus_app.status_iout_source & (uint8_t)(~PMBUS_STATUS_IOUT_FAULT_MASK));
    g_pmbus_app.status_input = (uint8_t)(g_pmbus_app.status_input_source & (uint8_t)(~PMBUS_STATUS_INPUT_FAULT_MASK));
    g_pmbus_app.status_temperature = (uint8_t)(g_pmbus_app.status_temperature_source & (uint8_t)(~PMBUS_STATUS_TEMPERATURE_FAULT_MASK));
    g_pmbus_app.status_cml = 0x00U;
    g_pmbus_app.status_cml_source = 0x00U;
    g_pmbus_app.status_other = (uint8_t)(g_pmbus_app.status_other_source & (uint8_t)(~PMBUS_STATUS_OTHER_FAULT_MASK));
    g_pmbus_app.status_mfr_specific = (uint8_t)(g_pmbus_app.status_mfr_specific_source & (uint8_t)(~PMBUS_STATUS_MFR_SPECIFIC_FAULT_MASK));
    g_pmbus_app.status_fans_1_2 = (uint8_t)(g_pmbus_app.status_fans_1_2_source & (uint8_t)(~PMBUS_STATUS_FANS_1_2_FAULT_MASK));
    g_pmbus_app.mfr_fault_active = 0U;
    g_pmbus_app.mfr_fwupload_status = (uint16_t)(g_pmbus_app.mfr_fwupload_status &
        (uint16_t)(PMBUS_FWUPLOAD_STATUS_COMPLETE | PMBUS_FWUPLOAD_STATUS_IN_PROGRESS));
    pmbus_app_release_alert();
    pmbus_app_refresh_status_and_optional_latch(0U);
}

void pmbus_app_set_busy_state(unsigned char busy)
{
    g_pmbus_app.busy_state = (uint8_t)(busy & 0x01U);
    pmbus_app_refresh_status_and_optional_latch(0U);
}

void pmbus_app_set_power_good_state(unsigned char asserted)
{
    g_pmbus_app.power_good_asserted = (uint8_t)(asserted & 0x01U);
    pmbus_app_refresh_status_and_optional_latch((uint8_t)(g_pmbus_app.power_good_asserted == 0U));
}

void pmbus_app_set_mfr_fault_state(unsigned char asserted)
{
    g_pmbus_app.mfr_fault_active = (uint8_t)(asserted & 0x01U);

    if (g_pmbus_app.mfr_fault_active != 0U)
    {
        pmbus_app_assert_alert();
    }

    pmbus_app_refresh_status_and_optional_latch(g_pmbus_app.mfr_fault_active);
}

void pmbus_app_set_comm_recovery_state(unsigned char last_reason,
    unsigned char recover_count,
    unsigned char recover_fail_count,
    unsigned char bus_released)
{
    g_pmbus_app.comm_last_recover_reason = (uint8_t)(last_reason & 0x0FU);
    g_pmbus_app.comm_recover_count = recover_count;
    g_pmbus_app.comm_recover_fail_count = recover_fail_count;
    g_pmbus_app.comm_bus_released = (uint8_t)(bus_released & 0x01U);
    pmbus_app_refresh_status_and_optional_latch(0U);
}

void pmbus_app_set_status_cml(unsigned char mask)
{
    pmbus_app_update_status_cml_bits(mask, 0U);
}

void pmbus_app_update_status_cml_bits(unsigned char set_mask, unsigned char clear_mask)
{
    pmbus_app_update_fault_register(&g_pmbus_app.status_cml, set_mask, clear_mask);
    g_pmbus_app.status_cml_source = (uint8_t)(g_pmbus_app.status_cml_source | set_mask);
    g_pmbus_app.status_cml_source = (uint8_t)(g_pmbus_app.status_cml_source & (uint8_t)(~clear_mask));

    if (g_pmbus_app.status_cml != 0U)
    {
        pmbus_app_assert_alert();
    }

    pmbus_app_refresh_status_and_optional_latch((uint8_t)(g_pmbus_app.status_cml != 0U));
}

void pmbus_app_update_status_cml_source(unsigned char active_bits)
{
    g_pmbus_app.status_cml_source = active_bits;
    g_pmbus_app.status_cml = pmbus_app_apply_fault_source_policy(g_pmbus_app.status_cml,
        g_pmbus_app.status_cml_source,
        PMBUS_STATUS_CML_FAULT_MASK);

    if (g_pmbus_app.status_cml != 0U)
    {
        pmbus_app_assert_alert();
    }

    pmbus_app_refresh_status_and_optional_latch((uint8_t)(g_pmbus_app.status_cml != 0U));
}

void pmbus_app_set_status_other(unsigned char value)
{
    pmbus_app_update_status_other_bits(value, (unsigned char)(~value));
}

void pmbus_app_set_status_mfr_specific(unsigned char value)
{
    pmbus_app_update_status_mfr_specific_bits(value, (unsigned char)(~value));
}

void pmbus_app_set_status_vout(unsigned char value)
{
    pmbus_app_update_status_vout_bits(value, (unsigned char)(~value));
}

void pmbus_app_set_status_iout(unsigned char value)
{
    pmbus_app_update_status_iout_bits(value, (unsigned char)(~value));
}

void pmbus_app_set_status_input(unsigned char value)
{
    pmbus_app_update_status_input_bits(value, (unsigned char)(~value));
}

void pmbus_app_set_status_temperature(unsigned char value)
{
    pmbus_app_update_status_temperature_bits(value, (unsigned char)(~value));
}

void pmbus_app_set_status_fans_1_2(unsigned char value)
{
    pmbus_app_update_status_fans_1_2_bits(value, (unsigned char)(~value));
}

void pmbus_app_update_status_other_bits(unsigned char set_mask, unsigned char clear_mask)
{
    pmbus_app_update_fault_register(&g_pmbus_app.status_other, set_mask, clear_mask);
    g_pmbus_app.status_other_source = (uint8_t)(g_pmbus_app.status_other_source | set_mask);
    g_pmbus_app.status_other_source = (uint8_t)(g_pmbus_app.status_other_source & (uint8_t)(~clear_mask));
    pmbus_app_refresh_status_and_optional_latch((uint8_t)((g_pmbus_app.status_other & PMBUS_STATUS_OTHER_FAULT_MASK) != 0U));
}

void pmbus_app_update_status_mfr_specific_bits(unsigned char set_mask, unsigned char clear_mask)
{
    pmbus_app_update_fault_register(&g_pmbus_app.status_mfr_specific, set_mask, clear_mask);
    g_pmbus_app.status_mfr_specific_source = (uint8_t)(g_pmbus_app.status_mfr_specific_source | set_mask);
    g_pmbus_app.status_mfr_specific_source = (uint8_t)(g_pmbus_app.status_mfr_specific_source & (uint8_t)(~clear_mask));
    pmbus_app_refresh_status_and_optional_latch((uint8_t)((g_pmbus_app.status_mfr_specific & PMBUS_STATUS_MFR_SPECIFIC_FAULT_MASK) != 0U));
}

void pmbus_app_update_status_vout_bits(unsigned char set_mask, unsigned char clear_mask)
{
    pmbus_app_update_fault_register(&g_pmbus_app.status_vout, set_mask, clear_mask);
    g_pmbus_app.status_vout_source = (uint8_t)(g_pmbus_app.status_vout_source | set_mask);
    g_pmbus_app.status_vout_source = (uint8_t)(g_pmbus_app.status_vout_source & (uint8_t)(~clear_mask));

    if ((g_pmbus_app.status_vout & PMBUS_STATUS_VOUT_FAULT_MASK) != 0U)
    {
        pmbus_app_assert_alert();
    }

    pmbus_app_refresh_status_and_optional_latch((uint8_t)((g_pmbus_app.status_vout & PMBUS_STATUS_VOUT_FAULT_MASK) != 0U));
}

void pmbus_app_update_status_vout_source(unsigned char active_bits)
{
    g_pmbus_app.status_vout_source = active_bits;
    g_pmbus_app.status_vout = pmbus_app_apply_fault_source_policy(g_pmbus_app.status_vout,
        g_pmbus_app.status_vout_source,
        PMBUS_STATUS_VOUT_FAULT_MASK);

    if ((g_pmbus_app.status_vout & PMBUS_STATUS_VOUT_FAULT_MASK) != 0U)
    {
        pmbus_app_assert_alert();
    }

    pmbus_app_refresh_status_and_optional_latch((uint8_t)((g_pmbus_app.status_vout & PMBUS_STATUS_VOUT_FAULT_MASK) != 0U));
}

void pmbus_app_set_vout_fault_sources(unsigned char vout_ov_fault,
    unsigned char vout_ov_warning,
    unsigned char vout_uv_warning,
    unsigned char vout_uv_fault,
    unsigned char vout_max_min_warning,
    unsigned char ton_max_fault,
    unsigned char toff_max_warning,
    unsigned char vout_tracking_error)
{
    uint8_t active_bits;

    active_bits = 0U;

    if (vout_ov_fault != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_VOUT_VOUT_OV_FAULT);
    if (vout_ov_warning != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_VOUT_VOUT_OV_WARNING);
    if (vout_uv_warning != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_VOUT_VOUT_UV_WARNING);
    if (vout_uv_fault != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_VOUT_VOUT_UV_FAULT);
    if (vout_max_min_warning != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_VOUT_VOUT_MAX_MIN_WARNING);
    if (ton_max_fault != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_VOUT_TON_MAX_FAULT);
    if (toff_max_warning != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_VOUT_TOFF_MAX_WARNING);
    if (vout_tracking_error != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_VOUT_VOUT_TRACKING_ERROR);

    pmbus_app_update_status_vout_source(active_bits);
}

void pmbus_app_update_status_iout_bits(unsigned char set_mask, unsigned char clear_mask)
{
    pmbus_app_update_fault_register(&g_pmbus_app.status_iout, set_mask, clear_mask);
    g_pmbus_app.status_iout_source = (uint8_t)(g_pmbus_app.status_iout_source | set_mask);
    g_pmbus_app.status_iout_source = (uint8_t)(g_pmbus_app.status_iout_source & (uint8_t)(~clear_mask));

    if ((g_pmbus_app.status_iout & PMBUS_STATUS_IOUT_FAULT_MASK) != 0U)
    {
        pmbus_app_assert_alert();
    }

    pmbus_app_refresh_status_and_optional_latch((uint8_t)((g_pmbus_app.status_iout & PMBUS_STATUS_IOUT_FAULT_MASK) != 0U));
}

void pmbus_app_update_status_iout_source(unsigned char active_bits)
{
    g_pmbus_app.status_iout_source = active_bits;
    g_pmbus_app.status_iout = pmbus_app_apply_fault_source_policy(g_pmbus_app.status_iout,
        g_pmbus_app.status_iout_source,
        PMBUS_STATUS_IOUT_FAULT_MASK);

    if ((g_pmbus_app.status_iout & PMBUS_STATUS_IOUT_FAULT_MASK) != 0U)
    {
        pmbus_app_assert_alert();
    }

    pmbus_app_refresh_status_and_optional_latch((uint8_t)((g_pmbus_app.status_iout & PMBUS_STATUS_IOUT_FAULT_MASK) != 0U));
}

void pmbus_app_set_iout_fault_sources(unsigned char iout_oc_fault,
    unsigned char iout_oc_lv_fault,
    unsigned char iout_oc_warning,
    unsigned char iout_uc_fault,
    unsigned char current_share_fault,
    unsigned char in_power_limiting_mode,
    unsigned char pout_op_fault,
    unsigned char pout_op_warning)
{
    uint8_t active_bits;

    active_bits = 0U;

    if (iout_oc_fault != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_IOUT_IOUT_OC_FAULT);
    if (iout_oc_lv_fault != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_IOUT_IOUT_OC_LV_FAULT);
    if (iout_oc_warning != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_IOUT_IOUT_OC_WARNING);
    if (iout_uc_fault != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_IOUT_IOUT_UC_FAULT);
    if (current_share_fault != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_IOUT_CURRENT_SHARE_FAULT);
    if (in_power_limiting_mode != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_IOUT_IN_POWER_LIMITING_MODE);
    if (pout_op_fault != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_IOUT_POUT_OP_FAULT);
    if (pout_op_warning != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_IOUT_POUT_OP_WARNING);

    pmbus_app_update_status_iout_source(active_bits);
}

void pmbus_app_update_status_input_bits(unsigned char set_mask, unsigned char clear_mask)
{
    pmbus_app_update_fault_register(&g_pmbus_app.status_input, set_mask, clear_mask);
    g_pmbus_app.status_input_source = (uint8_t)(g_pmbus_app.status_input_source | set_mask);
    g_pmbus_app.status_input_source = (uint8_t)(g_pmbus_app.status_input_source & (uint8_t)(~clear_mask));

    if ((g_pmbus_app.status_input & PMBUS_STATUS_INPUT_FAULT_MASK) != 0U)
    {
        pmbus_app_assert_alert();
    }

    pmbus_app_refresh_status_and_optional_latch((uint8_t)((g_pmbus_app.status_input & PMBUS_STATUS_INPUT_FAULT_MASK) != 0U));
}

void pmbus_app_update_status_input_source(unsigned char active_bits)
{
    g_pmbus_app.status_input_source = active_bits;
    g_pmbus_app.status_input = pmbus_app_apply_fault_source_policy(g_pmbus_app.status_input,
        g_pmbus_app.status_input_source,
        PMBUS_STATUS_INPUT_FAULT_MASK);

    if ((g_pmbus_app.status_input & PMBUS_STATUS_INPUT_FAULT_MASK) != 0U)
    {
        pmbus_app_assert_alert();
    }

    pmbus_app_refresh_status_and_optional_latch((uint8_t)((g_pmbus_app.status_input & PMBUS_STATUS_INPUT_FAULT_MASK) != 0U));
}

void pmbus_app_set_input_fault_sources(unsigned char vin_ov_fault,
    unsigned char vin_ov_warning,
    unsigned char vin_uv_warning,
    unsigned char vin_uv_fault,
    unsigned char unit_off_for_insufficient_input_voltage,
    unsigned char iin_oc_fault,
    unsigned char iin_oc_warning,
    unsigned char pin_op_warning)
{
    uint8_t active_bits;

    active_bits = 0U;

    if (vin_ov_fault != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_INPUT_VIN_OV_FAULT);
    if (vin_ov_warning != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_INPUT_VIN_OV_WARNING);
    if (vin_uv_warning != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_INPUT_VIN_UV_WARNING);
    if (vin_uv_fault != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_INPUT_VIN_UV_FAULT);
    if (unit_off_for_insufficient_input_voltage != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_INPUT_UNIT_OFF_FOR_INSUFFICIENT_INPUT_VOLTAGE);
    if (iin_oc_fault != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_INPUT_IIN_OC_FAULT);
    if (iin_oc_warning != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_INPUT_IIN_OC_WARNING);
    if (pin_op_warning != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_INPUT_PIN_OP_WARNING);

    pmbus_app_update_status_input_source(active_bits);
}

void pmbus_app_update_status_temperature_bits(unsigned char set_mask, unsigned char clear_mask)
{
    pmbus_app_update_fault_register(&g_pmbus_app.status_temperature, set_mask, clear_mask);
    g_pmbus_app.status_temperature_source = (uint8_t)(g_pmbus_app.status_temperature_source | set_mask);
    g_pmbus_app.status_temperature_source = (uint8_t)(g_pmbus_app.status_temperature_source & (uint8_t)(~clear_mask));

    if ((g_pmbus_app.status_temperature & PMBUS_STATUS_TEMPERATURE_FAULT_MASK) != 0U)
    {
        pmbus_app_assert_alert();
    }

    pmbus_app_refresh_status_and_optional_latch((uint8_t)((g_pmbus_app.status_temperature & PMBUS_STATUS_TEMPERATURE_FAULT_MASK) != 0U));
}

void pmbus_app_update_status_temperature_source(unsigned char active_bits)
{
    g_pmbus_app.status_temperature_source = active_bits;
    g_pmbus_app.status_temperature = pmbus_app_apply_fault_source_policy(g_pmbus_app.status_temperature,
        g_pmbus_app.status_temperature_source,
        PMBUS_STATUS_TEMPERATURE_FAULT_MASK);

    if ((g_pmbus_app.status_temperature & PMBUS_STATUS_TEMPERATURE_FAULT_MASK) != 0U)
    {
        pmbus_app_assert_alert();
    }

    pmbus_app_refresh_status_and_optional_latch((uint8_t)((g_pmbus_app.status_temperature & PMBUS_STATUS_TEMPERATURE_FAULT_MASK) != 0U));
}

void pmbus_app_set_temperature_fault_sources(unsigned char ot_fault,
    unsigned char ot_warning,
    unsigned char ut_warning,
    unsigned char ut_fault)
{
    uint8_t active_bits;

    active_bits = 0U;

    if (ot_fault != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_TEMPERATURE_OT_FAULT);
    if (ot_warning != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_TEMPERATURE_OT_WARNING);
    if (ut_warning != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_TEMPERATURE_UT_WARNING);
    if (ut_fault != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_TEMPERATURE_UT_FAULT);

    pmbus_app_update_status_temperature_source(active_bits);
}

void pmbus_app_update_status_fans_1_2_bits(unsigned char set_mask, unsigned char clear_mask)
{
    pmbus_app_update_fault_register(&g_pmbus_app.status_fans_1_2, set_mask, clear_mask);
    g_pmbus_app.status_fans_1_2_source = (uint8_t)(g_pmbus_app.status_fans_1_2_source | set_mask);
    g_pmbus_app.status_fans_1_2_source = (uint8_t)(g_pmbus_app.status_fans_1_2_source & (uint8_t)(~clear_mask));

    if ((g_pmbus_app.status_fans_1_2 & PMBUS_STATUS_FANS_1_2_FAULT_MASK) != 0U)
    {
        pmbus_app_assert_alert();
    }

    pmbus_app_refresh_status_and_optional_latch((uint8_t)((g_pmbus_app.status_fans_1_2 & PMBUS_STATUS_FANS_1_2_FAULT_MASK) != 0U));
}

void pmbus_app_update_status_fans_1_2_source(unsigned char active_bits)
{
    g_pmbus_app.status_fans_1_2_source = active_bits;
    g_pmbus_app.status_fans_1_2 = pmbus_app_apply_fault_source_policy(g_pmbus_app.status_fans_1_2,
        g_pmbus_app.status_fans_1_2_source,
        PMBUS_STATUS_FANS_1_2_FAULT_MASK);

    if ((g_pmbus_app.status_fans_1_2 & PMBUS_STATUS_FANS_1_2_FAULT_MASK) != 0U)
    {
        pmbus_app_assert_alert();
    }

    pmbus_app_refresh_status_and_optional_latch((uint8_t)((g_pmbus_app.status_fans_1_2 & PMBUS_STATUS_FANS_1_2_FAULT_MASK) != 0U));
}

void pmbus_app_update_status_other_source(unsigned char active_bits)
{
    g_pmbus_app.status_other_source = active_bits;
    g_pmbus_app.status_other = pmbus_app_apply_fault_source_policy(g_pmbus_app.status_other,
        g_pmbus_app.status_other_source,
        PMBUS_STATUS_OTHER_FAULT_MASK);
    pmbus_app_refresh_status_and_optional_latch((uint8_t)((g_pmbus_app.status_other & PMBUS_STATUS_OTHER_FAULT_MASK) != 0U));
}

void pmbus_app_update_status_mfr_specific_source(unsigned char active_bits)
{
    g_pmbus_app.status_mfr_specific_source = active_bits;
    g_pmbus_app.status_mfr_specific = pmbus_app_apply_fault_source_policy(g_pmbus_app.status_mfr_specific,
        g_pmbus_app.status_mfr_specific_source,
        PMBUS_STATUS_MFR_SPECIFIC_FAULT_MASK);
    pmbus_app_refresh_status_and_optional_latch((uint8_t)((g_pmbus_app.status_mfr_specific & PMBUS_STATUS_MFR_SPECIFIC_FAULT_MASK) != 0U));
}

void pmbus_app_set_fans_1_2_fault_sources(unsigned char fan_1_fault,
    unsigned char fan_2_fault,
    unsigned char fan_1_warning,
    unsigned char fan_2_warning,
    unsigned char fan_1_speed_overridden,
    unsigned char fan_2_speed_overridden,
    unsigned char airflow_fault,
    unsigned char airflow_warning)
{
    uint8_t active_bits;

    active_bits = 0U;

    if (fan_1_fault != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_FANS_1_2_FAN_1_FAULT);
    if (fan_2_fault != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_FANS_1_2_FAN_2_FAULT);
    if (fan_1_warning != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_FANS_1_2_FAN_1_WARNING);
    if (fan_2_warning != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_FANS_1_2_FAN_2_WARNING);
    if (fan_1_speed_overridden != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_FANS_1_2_FAN_1_SPEED_OVERRIDDEN);
    if (fan_2_speed_overridden != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_FANS_1_2_FAN_2_SPEED_OVERRIDDEN);
    if (airflow_fault != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_FANS_1_2_AIRFLOW_FAULT);
    if (airflow_warning != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_FANS_1_2_AIRFLOW_WARNING);

    pmbus_app_update_status_fans_1_2_source(active_bits);
}

void pmbus_app_set_status_other_fault_sources(unsigned char input_a_fuse_or_breaker_fault,
    unsigned char input_b_fuse_or_breaker_fault,
    unsigned char input_a_oring_device_fault,
    unsigned char input_b_oring_device_fault,
    unsigned char output_oring_device_fault,
    unsigned char first_to_assert_smbalert)
{
    uint8_t active_bits;

    active_bits = 0U;

    if (input_a_fuse_or_breaker_fault != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_OTHER_INPUT_A_FUSE_OR_BREAKER_FAULT);
    if (input_b_fuse_or_breaker_fault != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_OTHER_INPUT_B_FUSE_OR_BREAKER_FAULT);
    if (input_a_oring_device_fault != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_OTHER_INPUT_A_ORING_DEVICE_FAULT);
    if (input_b_oring_device_fault != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_OTHER_INPUT_B_ORING_DEVICE_FAULT);
    if (output_oring_device_fault != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_OTHER_OUTPUT_ORING_DEVICE_FAULT);
    if (first_to_assert_smbalert != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_OTHER_FIRST_TO_ASSERT_SMBALERT);

    pmbus_app_update_status_other_source(active_bits);
}

void pmbus_app_set_cml_fault_sources(unsigned char invalid_or_unsupported_command_received,
    unsigned char invalid_or_unsupported_data_received,
    unsigned char packet_error_check_failed,
    unsigned char memory_fault_detected,
    unsigned char processor_fault_detected,
    unsigned char other_communication_fault,
    unsigned char other_memory_or_logic_fault)
{
    uint8_t active_bits;

    active_bits = 0U;

    if (invalid_or_unsupported_command_received != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_CML_INVALID_OR_UNSUPPORTED_COMMAND_RECEIVED);
    if (invalid_or_unsupported_data_received != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_CML_INVALID_OR_UNSUPPORTED_DATA_RECEIVED);
    if (packet_error_check_failed != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_CML_PACKET_ERROR_CHECK_FAILED);
    if (memory_fault_detected != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_CML_MEMORY_FAULT_DETECTED);
    if (processor_fault_detected != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_CML_PROCESSOR_FAULT_DETECTED);
    if (other_communication_fault != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_CML_OTHER_COMMUNICATION_FAULT);
    if (other_memory_or_logic_fault != 0U) active_bits = (uint8_t)(active_bits | PMBUS_STATUS_CML_OTHER_MEMORY_OR_LOGIC_FAULT);

    pmbus_app_update_status_cml_source(active_bits);
}

void pmbus_app_latch_blackbox(void)
{
    pmbus_app_refresh_status_and_optional_latch(0U);
    pmbus_app_copy_bytes(g_pmbus_app.mfr_blackbox_latched,
        g_pmbus_app.mfr_blackbox_live,
        PMBUS_BLACKBOX_BLOCK_SIZE);
    g_pmbus_app.blackbox_latched = 1U;
}

void pmbus_app_update_input_source(unsigned long vin_mv, unsigned long iin_ma, unsigned long pin_mw)
{
    g_pmbus_app.source_vin_mv = (uint32_t)vin_mv;
    g_pmbus_app.source_iin_ma = (uint32_t)iin_ma;
    g_pmbus_app.source_pin_mw = (uint32_t)pin_mw;
}

void pmbus_app_update_output_source(unsigned long vout_mv, unsigned long iout_ma, unsigned long pout_mw)
{
    g_pmbus_app.source_vout_mv = (uint32_t)vout_mv;
    g_pmbus_app.source_iout_ma = (uint32_t)iout_ma;
    g_pmbus_app.source_pout_mw = (uint32_t)pout_mw;
}

void pmbus_app_update_temperature_source(unsigned long temp1_mc, unsigned long temp2_mc, unsigned long temp3_mc)
{
    g_pmbus_app.source_temp1_mc = (uint32_t)temp1_mc;
    g_pmbus_app.source_temp2_mc = (uint32_t)temp2_mc;
    g_pmbus_app.source_temp3_mc = (uint32_t)temp3_mc;
}

void pmbus_app_update_fan_source(unsigned long fan1_rpm, unsigned long fan2_rpm)
{
    g_pmbus_app.source_fan1_rpm = (uint32_t)fan1_rpm;
    g_pmbus_app.source_fan2_rpm = (uint32_t)fan2_rpm;
}

uint8_t pmbus_app_get_page(void)
{
    return g_pmbus_app.page;
}

void pmbus_app_set_page(uint8_t value)
{
    g_pmbus_app.page = value;
}

void pmbus_app_record_page_plus_write(uint8_t page, uint8_t command, uint8_t *data_ptr, uint8_t length)
{
    uint8_t index;

    if (length > PMBUS_MAX_BLOCK_SIZE)
    {
        length = PMBUS_MAX_BLOCK_SIZE;
    }

    g_pmbus_app.page_plus_last_page = page;
    g_pmbus_app.page_plus_last_command = command;
    g_pmbus_app.page_plus_last_length = length;

    for (index = 0U; index < length; index++)
    {
        if (data_ptr != 0)
        {
            g_pmbus_app.page_plus_last_payload[index] = data_ptr[index];
        }
        else
        {
            g_pmbus_app.page_plus_last_payload[index] = 0x00U;
        }
    }
}

void pmbus_app_record_page_plus_read(uint8_t page, uint8_t command)
{
    g_pmbus_app.page_plus_last_page = page;
    g_pmbus_app.page_plus_last_command = command;
    g_pmbus_app.page_plus_last_length = 0U;
}

void pmbus_app_record_store_restore(uint8_t command, uint8_t code)
{
    g_pmbus_app.store_restore_last_command = command;
    g_pmbus_app.store_restore_last_code = code;
    g_pmbus_app.store_restore_count++;
}

uint8_t pmbus_app_get_operation(void)
{
    return g_pmbus_app.operation;
}

void pmbus_app_set_operation(uint8_t value)
{
    g_pmbus_app.operation = value;
    pmbus_app_refresh_status_and_optional_latch(0U);
}

uint8_t pmbus_app_get_on_off_config(void)
{
    return g_pmbus_app.on_off_config;
}

void pmbus_app_set_on_off_config(uint8_t value)
{
    g_pmbus_app.on_off_config = value;
}

uint8_t pmbus_app_get_phase(void)
{
    return g_pmbus_app.phase;
}

void pmbus_app_set_phase(uint8_t value)
{
    g_pmbus_app.phase = value;
}

uint8_t pmbus_app_get_write_protect(void)
{
    return g_pmbus_app.write_protect;
}

void pmbus_app_set_write_protect(uint8_t value)
{
    g_pmbus_app.write_protect = value;
}

uint8_t pmbus_app_get_fan_config_1_2(void)
{
    return g_pmbus_app.fan_config_1_2;
}

void pmbus_app_set_fan_config_1_2(uint8_t value)
{
    g_pmbus_app.fan_config_1_2 = value;
}

uint16_t pmbus_app_get_fan_command_1(void)
{
    return g_pmbus_app.fan_command_1;
}

void pmbus_app_set_fan_command_1(uint16_t value)
{
    g_pmbus_app.fan_command_1 = value;
    pmbus_app_refresh_shadow_data();
}

uint16_t pmbus_app_get_fan_command_2(void)
{
    return g_pmbus_app.fan_command_2;
}

void pmbus_app_set_fan_command_2(uint16_t value)
{
    g_pmbus_app.fan_command_2 = value;
    pmbus_app_refresh_shadow_data();
}

uint8_t pmbus_app_get_fan_config_3_4(void)
{
    return g_pmbus_app.fan_config_3_4;
}

void pmbus_app_set_fan_config_3_4(uint8_t value)
{
    g_pmbus_app.fan_config_3_4 = value;
}

uint16_t pmbus_app_get_fan_command_3(void)
{
    return g_pmbus_app.fan_command_3;
}

void pmbus_app_set_fan_command_3(uint16_t value)
{
    g_pmbus_app.fan_command_3 = value;
}

uint16_t pmbus_app_get_fan_command_4(void)
{
    return g_pmbus_app.fan_command_4;
}

void pmbus_app_set_fan_command_4(uint16_t value)
{
    g_pmbus_app.fan_command_4 = value;
}

uint8_t pmbus_app_get_vout_mode(void)
{
    return g_pmbus_app.vout_mode;
}

void pmbus_app_set_vout_mode(uint8_t value)
{
    g_pmbus_app.vout_mode = value;
    pmbus_app_refresh_shadow_data();
}

uint16_t pmbus_app_get_vout_ov_fault_limit(void)
{
    return g_pmbus_app.vout_ov_fault_limit;
}

void pmbus_app_set_vout_ov_fault_limit(uint16_t value)
{
    g_pmbus_app.vout_ov_fault_limit = value;
    pmbus_app_refresh_shadow_data();
}

uint16_t pmbus_app_get_vout_ov_warn_limit(void)
{
    return g_pmbus_app.vout_ov_warn_limit;
}

void pmbus_app_set_vout_ov_warn_limit(uint16_t value)
{
    g_pmbus_app.vout_ov_warn_limit = value;
    pmbus_app_refresh_shadow_data();
}

uint16_t pmbus_app_get_vout_uv_warn_limit(void)
{
    return g_pmbus_app.vout_uv_warn_limit;
}

void pmbus_app_set_vout_uv_warn_limit(uint16_t value)
{
    g_pmbus_app.vout_uv_warn_limit = value;
    pmbus_app_refresh_shadow_data();
}

uint16_t pmbus_app_get_vout_uv_fault_limit(void)
{
    return g_pmbus_app.vout_uv_fault_limit;
}

void pmbus_app_set_vout_uv_fault_limit(uint16_t value)
{
    g_pmbus_app.vout_uv_fault_limit = value;
    pmbus_app_refresh_shadow_data();
}

uint16_t pmbus_app_get_iout_oc_fault_limit(void)
{
    return g_pmbus_app.iout_oc_fault_limit;
}

void pmbus_app_set_iout_oc_fault_limit(uint16_t value)
{
    g_pmbus_app.iout_oc_fault_limit = value;
    pmbus_app_refresh_shadow_data();
}

uint16_t pmbus_app_get_iout_oc_lv_fault_limit(void)
{
    return g_pmbus_app.iout_oc_lv_fault_limit;
}

void pmbus_app_set_iout_oc_lv_fault_limit(uint16_t value)
{
    g_pmbus_app.iout_oc_lv_fault_limit = value;
}

uint16_t pmbus_app_get_iout_oc_warn_limit(void)
{
    return g_pmbus_app.iout_oc_warn_limit;
}

void pmbus_app_set_iout_oc_warn_limit(uint16_t value)
{
    g_pmbus_app.iout_oc_warn_limit = value;
    pmbus_app_refresh_shadow_data();
}

uint16_t pmbus_app_get_iout_uc_fault_limit(void)
{
    return g_pmbus_app.iout_uc_fault_limit;
}

void pmbus_app_set_iout_uc_fault_limit(uint16_t value)
{
    g_pmbus_app.iout_uc_fault_limit = value;
}

uint16_t pmbus_app_get_ot_fault_limit(void)
{
    return g_pmbus_app.ot_fault_limit;
}

void pmbus_app_set_ot_fault_limit(uint16_t value)
{
    g_pmbus_app.ot_fault_limit = value;
    pmbus_app_refresh_shadow_data();
}

uint16_t pmbus_app_get_ot_warn_limit(void)
{
    return g_pmbus_app.ot_warn_limit;
}

void pmbus_app_set_ot_warn_limit(uint16_t value)
{
    g_pmbus_app.ot_warn_limit = value;
    pmbus_app_refresh_shadow_data();
}

uint16_t pmbus_app_get_ut_warn_limit(void)
{
    return g_pmbus_app.ut_warn_limit;
}

void pmbus_app_set_ut_warn_limit(uint16_t value)
{
    g_pmbus_app.ut_warn_limit = value;
}

uint16_t pmbus_app_get_ut_fault_limit(void)
{
    return g_pmbus_app.ut_fault_limit;
}

void pmbus_app_set_ut_fault_limit(uint16_t value)
{
    g_pmbus_app.ut_fault_limit = value;
}

uint16_t pmbus_app_get_vin_ov_fault_limit(void)
{
    return g_pmbus_app.vin_ov_fault_limit;
}

void pmbus_app_set_vin_ov_fault_limit(uint16_t value)
{
    g_pmbus_app.vin_ov_fault_limit = value;
    pmbus_app_refresh_shadow_data();
}

uint16_t pmbus_app_get_vin_ov_warn_limit(void)
{
    return g_pmbus_app.vin_ov_warn_limit;
}

void pmbus_app_set_vin_ov_warn_limit(uint16_t value)
{
    g_pmbus_app.vin_ov_warn_limit = value;
    pmbus_app_refresh_shadow_data();
}

uint16_t pmbus_app_get_vin_uv_warn_limit(void)
{
    return g_pmbus_app.vin_uv_warn_limit;
}

void pmbus_app_set_vin_uv_warn_limit(uint16_t value)
{
    g_pmbus_app.vin_uv_warn_limit = value;
    pmbus_app_refresh_shadow_data();
}

uint16_t pmbus_app_get_vin_uv_fault_limit(void)
{
    return g_pmbus_app.vin_uv_fault_limit;
}

void pmbus_app_set_vin_uv_fault_limit(uint16_t value)
{
    g_pmbus_app.vin_uv_fault_limit = value;
    pmbus_app_refresh_shadow_data();
}

uint16_t pmbus_app_get_iin_oc_fault_limit(void)
{
    return g_pmbus_app.iin_oc_fault_limit;
}

void pmbus_app_set_iin_oc_fault_limit(uint16_t value)
{
    g_pmbus_app.iin_oc_fault_limit = value;
    pmbus_app_refresh_shadow_data();
}

uint16_t pmbus_app_get_iin_oc_warn_limit(void)
{
    return g_pmbus_app.iin_oc_warn_limit;
}

void pmbus_app_set_iin_oc_warn_limit(uint16_t value)
{
    g_pmbus_app.iin_oc_warn_limit = value;
    pmbus_app_refresh_shadow_data();
}

uint8_t pmbus_app_get_vout_ov_fault_response(void)
{
    return g_pmbus_app.vout_ov_fault_response;
}

void pmbus_app_set_vout_ov_fault_response(uint8_t value)
{
    g_pmbus_app.vout_ov_fault_response = value;
}

uint8_t pmbus_app_get_vout_uv_fault_response(void)
{
    return g_pmbus_app.vout_uv_fault_response;
}

void pmbus_app_set_vout_uv_fault_response(uint8_t value)
{
    g_pmbus_app.vout_uv_fault_response = value;
}

uint8_t pmbus_app_get_iout_oc_fault_response(void)
{
    return g_pmbus_app.iout_oc_fault_response;
}

void pmbus_app_set_iout_oc_fault_response(uint8_t value)
{
    g_pmbus_app.iout_oc_fault_response = value;
}

uint8_t pmbus_app_get_iout_oc_lv_fault_response(void)
{
    return g_pmbus_app.iout_oc_lv_fault_response;
}

void pmbus_app_set_iout_oc_lv_fault_response(uint8_t value)
{
    g_pmbus_app.iout_oc_lv_fault_response = value;
}

uint8_t pmbus_app_get_iout_uc_fault_response(void)
{
    return g_pmbus_app.iout_uc_fault_response;
}

void pmbus_app_set_iout_uc_fault_response(uint8_t value)
{
    g_pmbus_app.iout_uc_fault_response = value;
}

uint8_t pmbus_app_get_ot_fault_response(void)
{
    return g_pmbus_app.ot_fault_response;
}

void pmbus_app_set_ot_fault_response(uint8_t value)
{
    g_pmbus_app.ot_fault_response = value;
}

uint8_t pmbus_app_get_ut_fault_response(void)
{
    return g_pmbus_app.ut_fault_response;
}

void pmbus_app_set_ut_fault_response(uint8_t value)
{
    g_pmbus_app.ut_fault_response = value;
}

uint8_t pmbus_app_get_vin_ov_fault_response(void)
{
    return g_pmbus_app.vin_ov_fault_response;
}

void pmbus_app_set_vin_ov_fault_response(uint8_t value)
{
    g_pmbus_app.vin_ov_fault_response = value;
}

uint8_t pmbus_app_get_vin_uv_fault_response(void)
{
    return g_pmbus_app.vin_uv_fault_response;
}

void pmbus_app_set_vin_uv_fault_response(uint8_t value)
{
    g_pmbus_app.vin_uv_fault_response = value;
}

uint8_t pmbus_app_get_iin_oc_fault_response(void)
{
    return g_pmbus_app.iin_oc_fault_response;
}

void pmbus_app_set_iin_oc_fault_response(uint8_t value)
{
    g_pmbus_app.iin_oc_fault_response = value;
}

uint8_t pmbus_app_get_ton_max_fault_response(void)
{
    return g_pmbus_app.ton_max_fault_response;
}

void pmbus_app_set_ton_max_fault_response(uint8_t value)
{
    g_pmbus_app.ton_max_fault_response = value;
}

uint8_t pmbus_app_get_pout_op_fault_response(void)
{
    return g_pmbus_app.pout_op_fault_response;
}

void pmbus_app_set_pout_op_fault_response(uint8_t value)
{
    g_pmbus_app.pout_op_fault_response = value;
}

uint16_t pmbus_app_get_zone_config(void)
{
    return g_pmbus_app.zone_config;
}

void pmbus_app_set_zone_config(uint16_t value)
{
    g_pmbus_app.zone_config = value;
}

uint16_t pmbus_app_get_zone_active(void)
{
    return g_pmbus_app.zone_active;
}

void pmbus_app_set_zone_active(uint16_t value)
{
    g_pmbus_app.zone_active = value;
}

uint16_t pmbus_app_get_vout_command(void)
{
    return g_pmbus_app.vout_command;
}

void pmbus_app_set_vout_command(uint16_t value)
{
    g_pmbus_app.vout_command = value;
}

uint16_t pmbus_app_get_vout_trim(void)
{
    return g_pmbus_app.vout_trim;
}

void pmbus_app_set_vout_trim(uint16_t value)
{
    g_pmbus_app.vout_trim = value;
}

uint16_t pmbus_app_get_vout_cal_offset(void)
{
    return g_pmbus_app.vout_cal_offset;
}

void pmbus_app_set_vout_cal_offset(uint16_t value)
{
    g_pmbus_app.vout_cal_offset = value;
}

uint16_t pmbus_app_get_vout_max(void)
{
    return g_pmbus_app.vout_max;
}

void pmbus_app_set_vout_max(uint16_t value)
{
    g_pmbus_app.vout_max = value;
}

uint16_t pmbus_app_get_vout_margin_high(void)
{
    return g_pmbus_app.vout_margin_high;
}

void pmbus_app_set_vout_margin_high(uint16_t value)
{
    g_pmbus_app.vout_margin_high = value;
}

uint16_t pmbus_app_get_vout_margin_low(void)
{
    return g_pmbus_app.vout_margin_low;
}

void pmbus_app_set_vout_margin_low(uint16_t value)
{
    g_pmbus_app.vout_margin_low = value;
}

uint16_t pmbus_app_get_vout_transition_rate(void)
{
    return g_pmbus_app.vout_transition_rate;
}

void pmbus_app_set_vout_transition_rate(uint16_t value)
{
    g_pmbus_app.vout_transition_rate = value;
}

uint16_t pmbus_app_get_vout_droop(void)
{
    return g_pmbus_app.vout_droop;
}

void pmbus_app_set_vout_droop(uint16_t value)
{
    g_pmbus_app.vout_droop = value;
}

uint16_t pmbus_app_get_vout_scale_loop(void)
{
    return g_pmbus_app.vout_scale_loop;
}

void pmbus_app_set_vout_scale_loop(uint16_t value)
{
    g_pmbus_app.vout_scale_loop = value;
}

uint16_t pmbus_app_get_vout_scale_monitor(void)
{
    return g_pmbus_app.vout_scale_monitor;
}

void pmbus_app_set_vout_scale_monitor(uint16_t value)
{
    g_pmbus_app.vout_scale_monitor = value;
}

uint16_t pmbus_app_get_vout_min(void)
{
    return g_pmbus_app.vout_min;
}

void pmbus_app_set_vout_min(uint16_t value)
{
    g_pmbus_app.vout_min = value;
}

uint8_t pmbus_app_get_coefficients(unsigned char requested_command, uint8_t **data_ptr)
{
    /* COEFFICIENTS currently returns one fixed DIRECT-format coefficient set
       for any requested command. TODO: Return command-specific coefficients
       if the product exposes DIRECT-format telemetry or config commands. */
    (void)requested_command;
    *data_ptr = g_pmbus_app.coefficients;
    return 5U;
}

uint16_t pmbus_app_get_pout_max(void)
{
    return g_pmbus_app.pout_max;
}

void pmbus_app_set_pout_max(uint16_t value)
{
    g_pmbus_app.pout_max = value;
}

uint16_t pmbus_app_get_max_duty(void)
{
    return g_pmbus_app.max_duty;
}

void pmbus_app_set_max_duty(uint16_t value)
{
    g_pmbus_app.max_duty = value;
}

uint16_t pmbus_app_get_frequency_switch(void)
{
    return g_pmbus_app.frequency_switch;
}

void pmbus_app_set_frequency_switch(uint16_t value)
{
    g_pmbus_app.frequency_switch = value;
}

uint8_t pmbus_app_get_power_mode(void)
{
    return g_pmbus_app.power_mode;
}

void pmbus_app_set_power_mode(uint8_t value)
{
    g_pmbus_app.power_mode = value;
}

uint16_t pmbus_app_get_vin_on(void)
{
    return g_pmbus_app.vin_on;
}

void pmbus_app_set_vin_on(uint16_t value)
{
    g_pmbus_app.vin_on = value;
}

uint16_t pmbus_app_get_vin_off(void)
{
    return g_pmbus_app.vin_off;
}

void pmbus_app_set_vin_off(uint16_t value)
{
    g_pmbus_app.vin_off = value;
}

uint16_t pmbus_app_get_interleave(void)
{
    return g_pmbus_app.interleave;
}

void pmbus_app_set_interleave(uint16_t value)
{
    g_pmbus_app.interleave = value;
}

uint16_t pmbus_app_get_iout_cal_gain(void)
{
    return g_pmbus_app.iout_cal_gain;
}

void pmbus_app_set_iout_cal_gain(uint16_t value)
{
    g_pmbus_app.iout_cal_gain = value;
}

uint16_t pmbus_app_get_iout_cal_offset(void)
{
    return g_pmbus_app.iout_cal_offset;
}

void pmbus_app_set_iout_cal_offset(uint16_t value)
{
    g_pmbus_app.iout_cal_offset = value;
}

uint16_t pmbus_app_get_power_good_on(void)
{
    return g_pmbus_app.power_good_on;
}

void pmbus_app_set_power_good_on(uint16_t value)
{
    g_pmbus_app.power_good_on = value;
}

uint16_t pmbus_app_get_power_good_off(void)
{
    return g_pmbus_app.power_good_off;
}

void pmbus_app_set_power_good_off(uint16_t value)
{
    g_pmbus_app.power_good_off = value;
}

uint16_t pmbus_app_get_ton_delay(void)
{
    return g_pmbus_app.ton_delay;
}

void pmbus_app_set_ton_delay(uint16_t value)
{
    g_pmbus_app.ton_delay = value;
}

uint16_t pmbus_app_get_ton_rise(void)
{
    return g_pmbus_app.ton_rise;
}

void pmbus_app_set_ton_rise(uint16_t value)
{
    g_pmbus_app.ton_rise = value;
}

uint16_t pmbus_app_get_ton_max_fault_limit(void)
{
    return g_pmbus_app.ton_max_fault_limit;
}

void pmbus_app_set_ton_max_fault_limit(uint16_t value)
{
    g_pmbus_app.ton_max_fault_limit = value;
}

uint16_t pmbus_app_get_toff_delay(void)
{
    return g_pmbus_app.toff_delay;
}

void pmbus_app_set_toff_delay(uint16_t value)
{
    g_pmbus_app.toff_delay = value;
}

uint16_t pmbus_app_get_toff_fall(void)
{
    return g_pmbus_app.toff_fall;
}

void pmbus_app_set_toff_fall(uint16_t value)
{
    g_pmbus_app.toff_fall = value;
}

uint16_t pmbus_app_get_toff_max_warn_limit(void)
{
    return g_pmbus_app.toff_max_warn_limit;
}

void pmbus_app_set_toff_max_warn_limit(uint16_t value)
{
    g_pmbus_app.toff_max_warn_limit = value;
}

uint16_t pmbus_app_get_pout_op_fault_limit(void)
{
    return g_pmbus_app.pout_op_fault_limit;
}

void pmbus_app_set_pout_op_fault_limit(uint16_t value)
{
    g_pmbus_app.pout_op_fault_limit = value;
}

uint16_t pmbus_app_get_pout_op_warn_limit(void)
{
    return g_pmbus_app.pout_op_warn_limit;
}

void pmbus_app_set_pout_op_warn_limit(uint16_t value)
{
    g_pmbus_app.pout_op_warn_limit = value;
}

uint16_t pmbus_app_get_pin_op_warn_limit(void)
{
    return g_pmbus_app.pin_op_warn_limit;
}

void pmbus_app_set_pin_op_warn_limit(uint16_t value)
{
    g_pmbus_app.pin_op_warn_limit = value;
}

uint16_t pmbus_app_get_smbalert_mask(void)
{
    return g_pmbus_app.smbalert_mask;
}

void pmbus_app_set_smbalert_mask(uint16_t value)
{
    g_pmbus_app.smbalert_mask = value;
}

uint8_t pmbus_app_get_mfr_cold_redundancy_config(void)
{
    return g_pmbus_app.mfr_cold_redundancy_config;
}

void pmbus_app_set_mfr_cold_redundancy_config(unsigned char value)
{
    g_pmbus_app.mfr_cold_redundancy_config = value;
}

uint8_t pmbus_app_get_mfr_fwupload_mode(void)
{
    return g_pmbus_app.mfr_fwupload_mode;
}

void pmbus_app_set_mfr_fwupload_mode(unsigned char value)
{
    uint8_t mode_enabled;

    mode_enabled = (uint8_t)(value & 0x01U);
    g_pmbus_app.mfr_fwupload_mode = mode_enabled;

    if (mode_enabled != 0U)
    {
        g_pmbus_app.mfr_fwupload_status = PMBUS_FWUPLOAD_STATUS_IN_PROGRESS;
        g_pmbus_app.mfr_fwupload_last_block_len = 0U;
        g_pmbus_app.mfr_fwupload_block_count = 0U;
        g_pmbus_app.mfr_fwupload_expected_sequence = 0U;
        g_pmbus_app.mfr_fwupload_last_sequence = 0U;
        g_pmbus_app.mfr_fwupload_final_seen = 0U;
    }
    else
    {
        if ((g_pmbus_app.mfr_fwupload_status & (PMBUS_FWUPLOAD_STATUS_BAD_IMAGE | PMBUS_FWUPLOAD_STATUS_UNSUPPORTED)) != 0U)
        {
            g_pmbus_app.mfr_fwupload_mode = 1U;
        }
        else if (g_pmbus_app.mfr_fwupload_final_seen != 0U)
        {
            g_pmbus_app.mfr_fwupload_status = PMBUS_FWUPLOAD_STATUS_COMPLETE;
        }
        else if (g_pmbus_app.mfr_fwupload_block_count != 0U)
        {
            g_pmbus_app.mfr_fwupload_status = PMBUS_FWUPLOAD_STATUS_IN_PROGRESS;
        }
    }
}

uint16_t pmbus_app_get_mfr_fwupload_status(void)
{
    return g_pmbus_app.mfr_fwupload_status;
}

uint8_t pmbus_app_store_mfr_fwupload_block(unsigned char *data_ptr, unsigned char length)
{
    uint16_t sequence;
    uint8_t flags;
    uint8_t payload_length;

    if ((g_pmbus_app.mfr_fwupload_mode & 0x01U) == 0U)
    {
        g_pmbus_app.mfr_fwupload_status = PMBUS_FWUPLOAD_STATUS_UNSUPPORTED;
        return 0U;
    }

    if ((length < PMBUS_FWUPLOAD_HEADER_SIZE) || (length > PMBUS_MAX_BLOCK_SIZE))
    {
        g_pmbus_app.mfr_fwupload_status = PMBUS_FWUPLOAD_STATUS_BAD_IMAGE;
        return 0U;
    }

    if (g_pmbus_app.mfr_fwupload_final_seen != 0U)
    {
        g_pmbus_app.mfr_fwupload_status = PMBUS_FWUPLOAD_STATUS_BAD_IMAGE;
        return 0U;
    }

    sequence = (uint16_t)(((uint16_t)data_ptr[1] << 8) | data_ptr[0]);
    flags = data_ptr[2];
    payload_length = data_ptr[3];

    if ((uint8_t)(payload_length + PMBUS_FWUPLOAD_HEADER_SIZE) != length)
    {
        g_pmbus_app.mfr_fwupload_status = PMBUS_FWUPLOAD_STATUS_BAD_IMAGE;
        return 0U;
    }

    if (sequence != g_pmbus_app.mfr_fwupload_expected_sequence)
    {
        g_pmbus_app.mfr_fwupload_status = PMBUS_FWUPLOAD_STATUS_BAD_IMAGE;
        return 0U;
    }

    pmbus_app_copy_bytes(g_pmbus_app.mfr_fwupload_last_block, data_ptr, length);
    g_pmbus_app.mfr_fwupload_last_block_len = length;
    g_pmbus_app.mfr_fwupload_block_count = (uint16_t)(g_pmbus_app.mfr_fwupload_block_count + 1U);
    g_pmbus_app.mfr_fwupload_last_sequence = sequence;
    g_pmbus_app.mfr_fwupload_expected_sequence = (uint16_t)(sequence + 1U);

    if ((flags & PMBUS_FWUPLOAD_FLAG_FINAL_BLOCK) != 0U)
    {
        g_pmbus_app.mfr_fwupload_final_seen = 1U;
        g_pmbus_app.mfr_fwupload_status = PMBUS_FWUPLOAD_STATUS_COMPLETE;
    }
    else
    {
        g_pmbus_app.mfr_fwupload_status = PMBUS_FWUPLOAD_STATUS_IN_PROGRESS;
    }

    return 1U;
}

uint8_t pmbus_app_get_capability(void)
{
    return g_pmbus_app.capability;
}

uint8_t pmbus_app_get_status_byte(void)
{
    return g_pmbus_app.status_byte;
}

uint16_t pmbus_app_get_status_word(void)
{
    return g_pmbus_app.status_word;
}

uint8_t pmbus_app_get_status_vout(void)
{
    return g_pmbus_app.status_vout;
}

uint8_t pmbus_app_get_status_iout(void)
{
    return g_pmbus_app.status_iout;
}

uint8_t pmbus_app_get_status_input(void)
{
    return g_pmbus_app.status_input;
}

uint8_t pmbus_app_get_status_temperature(void)
{
    return g_pmbus_app.status_temperature;
}

uint8_t pmbus_app_get_status_cml(void)
{
    return g_pmbus_app.status_cml;
}

uint8_t pmbus_app_get_status_other(void)
{
    return g_pmbus_app.status_other;
}

uint8_t pmbus_app_get_status_mfr_specific(void)
{
    return g_pmbus_app.status_mfr_specific;
}

uint8_t pmbus_app_get_status_fans_1_2(void)
{
    return g_pmbus_app.status_fans_1_2;
}

uint8_t pmbus_app_get_read_ein(uint8_t **data_ptr)
{
    *data_ptr = g_pmbus_app.read_ein;
    return 6U;
}

uint8_t pmbus_app_get_read_eout(uint8_t **data_ptr)
{
    *data_ptr = g_pmbus_app.read_eout;
    return 6U;
}

uint16_t pmbus_app_get_read_vin(void)
{
    return g_pmbus_app.read_vin;
}

uint16_t pmbus_app_get_read_iin(void)
{
    return g_pmbus_app.read_iin;
}

uint16_t pmbus_app_get_read_vout(void)
{
    return g_pmbus_app.read_vout;
}

uint16_t pmbus_app_get_read_iout(void)
{
    return g_pmbus_app.read_iout;
}

uint16_t pmbus_app_get_read_temperature_1(void)
{
    return g_pmbus_app.read_temperature_1;
}

uint16_t pmbus_app_get_read_temperature_2(void)
{
    return g_pmbus_app.read_temperature_2;
}

uint16_t pmbus_app_get_read_temperature_3(void)
{
    return g_pmbus_app.read_temperature_3;
}

uint16_t pmbus_app_get_read_fan_speed_1(void)
{
    return g_pmbus_app.read_fan_speed_1;
}

uint16_t pmbus_app_get_read_fan_speed_2(void)
{
    return g_pmbus_app.read_fan_speed_2;
}

uint16_t pmbus_app_get_read_pout(void)
{
    return g_pmbus_app.read_pout;
}

uint16_t pmbus_app_get_read_pin(void)
{
    return g_pmbus_app.read_pin;
}

uint8_t pmbus_app_get_pmbus_revision(void)
{
    /* PMBUS_REVISION is intentionally fixed to PMBus 1.3 for this slave.
       TODO: Change only when the implemented command/transaction set is
       intentionally moved to another PMBus revision. */
    return 0x33U;
}

uint8_t pmbus_app_get_mfr_id(uint8_t **data_ptr)
{
    *data_ptr = g_mfr_id;
    return (uint8_t)(sizeof(g_mfr_id) - 1U);
}

uint8_t pmbus_app_get_mfr_model(uint8_t **data_ptr)
{
    *data_ptr = g_mfr_model;
    return (uint8_t)(sizeof(g_mfr_model) - 1U);
}

uint8_t pmbus_app_get_mfr_revision(uint8_t **data_ptr)
{
    *data_ptr = g_mfr_revision;
    return (uint8_t)(sizeof(g_mfr_revision) - 1U);
}

uint8_t pmbus_app_get_mfr_serial(uint8_t **data_ptr)
{
    *data_ptr = g_mfr_serial;
    return (uint8_t)(sizeof(g_mfr_serial) - 1U);
}

uint8_t pmbus_app_get_mfr_blackbox(uint8_t **data_ptr)
{
    if (g_pmbus_app.blackbox_latched != 0U)
    {
        *data_ptr = g_pmbus_app.mfr_blackbox_latched;
    }
    else
    {
        *data_ptr = g_pmbus_app.mfr_blackbox_live;
    }

    return PMBUS_BLACKBOX_BLOCK_SIZE;
}

uint8_t pmbus_app_is_alert_asserted(void)
{
    return g_pmbus_app.alert_asserted;
}

void pmbus_app_assert_alert(void)
{
    g_pmbus_app.alert_asserted = 1U;
    pmbus_io_alert_assert();
}

void pmbus_app_release_alert(void)
{
    g_pmbus_app.alert_asserted = 0U;
    pmbus_io_alert_release();
}
