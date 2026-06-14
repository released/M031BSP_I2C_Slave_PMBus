#include "pmbus_types.h"
#include "pmbus_dispatch.h"
#include "pmbus_app.h"

typedef enum
{
    PMBUS_RESP_NONE = 0,
    PMBUS_RESP_BYTE,
    PMBUS_RESP_WORD,
    PMBUS_RESP_DWORD,
    PMBUS_RESP_BLOCK
} pmbus_dispatch_response_kind_t;

static uint8_t pmbus_dispatch_is_write_byte_supported(uint8_t command);
static uint8_t pmbus_dispatch_is_write_word_supported(uint8_t command);
static uint8_t pmbus_dispatch_is_send_byte_supported(uint8_t command);
static uint8_t pmbus_dispatch_is_block_write_supported(uint8_t command);
static uint8_t pmbus_dispatch_is_block_write_read_process_call_supported(uint8_t command);
static uint8_t pmbus_dispatch_is_user_data_command(uint8_t command);
static uint8_t pmbus_dispatch_is_mfr_policy_block_command(uint8_t command);
static uint8_t pmbus_dispatch_is_policy_block_command(uint8_t command);
static uint8_t pmbus_dispatch_is_extended_selector(uint8_t command);
static uint8_t pmbus_dispatch_store_policy_block(uint8_t command, uint8_t *payload, uint8_t data_len);
static uint8_t pmbus_dispatch_store_extended_policy_block(uint8_t command, uint8_t *payload, uint8_t data_len);
static uint8_t pmbus_dispatch_build_policy_block_response(uint8_t command, uint8_t *tx_buffer, uint8_t *tx_length);
static uint8_t pmbus_dispatch_build_extended_policy_block_response(uint8_t command, uint8_t *payload, uint8_t data_len, uint8_t *tx_buffer, uint8_t *tx_length);
static uint8_t pmbus_dispatch_is_write_locked(void);
static uint8_t pmbus_dispatch_get_query_data_format(uint8_t command);
static uint8_t pmbus_dispatch_get_query_response(uint8_t command);
static uint8_t pmbus_dispatch_build_query_response(uint8_t *payload, uint8_t data_len, uint8_t *tx_buffer, uint8_t *tx_length);
static uint8_t pmbus_dispatch_build_smbalert_mask_response(uint8_t *payload, uint8_t data_len, uint8_t *tx_buffer, uint8_t *tx_length);
static uint8_t pmbus_dispatch_build_coefficients_response(uint8_t *payload, uint8_t data_len, uint8_t *tx_buffer, uint8_t *tx_length);
static uint8_t pmbus_dispatch_build_page_plus_read_response(uint8_t *payload, uint8_t data_len, uint8_t *tx_buffer, uint8_t *tx_length);
static uint8_t pmbus_dispatch_build_byte_response(uint8_t command, uint8_t *tx_buffer, uint8_t *tx_length);
static uint8_t pmbus_dispatch_build_word_response(uint8_t command, uint8_t *tx_buffer, uint8_t *tx_length);
static uint8_t pmbus_dispatch_build_dword_response(uint8_t command, uint8_t *tx_buffer, uint8_t *tx_length);
static uint8_t pmbus_dispatch_build_block_response(uint8_t command, uint8_t *tx_buffer, uint8_t *tx_length);

/* P2 Table 31 read-only placeholders.
   TODO: Bind these to SKU EEPROM, production data, and real CRPS platform measurements. */
static uint8_t g_pmbus_dispatch_mfr_location[] = "MFR_LOCATION_001";
static uint8_t g_pmbus_dispatch_mfr_date[] = "2026-06-14";
static uint8_t g_pmbus_dispatch_app_profile[] = { 0x01U, 0x13U };
static uint8_t g_pmbus_dispatch_efficiency_ll[] = { 0x14U, 0x5AU, 0x32U, 0x5CU, 0x64U, 0x5DU };
static uint8_t g_pmbus_dispatch_efficiency_hl[] = { 0x14U, 0x5CU, 0x32U, 0x5EU, 0x64U, 0x60U };
static uint8_t g_pmbus_dispatch_ic_device_id[] = "M031_PMBUS";
static uint8_t g_pmbus_dispatch_ic_device_rev[] = "REV_001";

#define PMBUS_DISPATCH_POLICY_BLOCK_SIZE      16U
#define PMBUS_DISPATCH_USER_DATA_COUNT        16U
#define PMBUS_DISPATCH_MFR_POLICY_COUNT       58U

/* CRPS policy namespace:
   USER_DATA and unassigned MFR_SPECIFIC commands are volatile bounded block shadows.
   TODO: bind each product-owned entry to NVM, telemetry, or control logic before final CRPS release. */
static uint8_t g_pmbus_dispatch_user_policy_lengths[PMBUS_DISPATCH_USER_DATA_COUNT];
static uint8_t g_pmbus_dispatch_user_policy_data[PMBUS_DISPATCH_USER_DATA_COUNT][PMBUS_DISPATCH_POLICY_BLOCK_SIZE];
static uint8_t g_pmbus_dispatch_mfr_policy_lengths[PMBUS_DISPATCH_MFR_POLICY_COUNT];
static uint8_t g_pmbus_dispatch_mfr_policy_data[PMBUS_DISPATCH_MFR_POLICY_COUNT][PMBUS_DISPATCH_POLICY_BLOCK_SIZE];
static uint8_t g_pmbus_dispatch_extended_selector;
static uint8_t g_pmbus_dispatch_extended_command;
static uint8_t g_pmbus_dispatch_extended_length;
static uint8_t g_pmbus_dispatch_extended_data[PMBUS_DISPATCH_POLICY_BLOCK_SIZE];

static void pmbus_dispatch_store_word(uint8_t *buffer, uint16_t value)
{
    buffer[0] = (uint8_t)(value & 0x00FFU);
    buffer[1] = (uint8_t)((value >> 8) & 0x00FFU);
}

static void pmbus_dispatch_store_dword(uint8_t *buffer, uint32_t value)
{
    buffer[0] = (uint8_t)(value & 0x000000FFUL);
    buffer[1] = (uint8_t)((value >> 8) & 0x000000FFUL);
    buffer[2] = (uint8_t)((value >> 16) & 0x000000FFUL);
    buffer[3] = (uint8_t)((value >> 24) & 0x000000FFUL);
}

static uint8_t pmbus_dispatch_is_user_data_command(uint8_t command)
{
    if ((command >= PMBUS_COMMAND_USER_DATA_00) &&
        (command <= PMBUS_COMMAND_USER_DATA_15))
    {
        return 1U;
    }

    return 0U;
}

static uint8_t pmbus_dispatch_is_mfr_policy_block_command(uint8_t command)
{
    if ((command < PMBUS_COMMAND_MFR_SPECIFIC_C4) ||
        (command > PMBUS_COMMAND_MFR_SPECIFIC_FD))
    {
        return 0U;
    }

    switch (command)
    {
        case PMBUS_COMMAND_MFR_COLD_REDUNDANCY_CONFIG:
        case PMBUS_COMMAND_MFR_FWUPLOAD_MODE:
        case PMBUS_COMMAND_MFR_FWUPLOAD:
        case PMBUS_COMMAND_MFR_FWUPLOAD_STATUS:
        case PMBUS_COMMAND_MFR_BLACKBOX:
            return 0U;

        default:
            return 1U;
    }
}

static uint8_t pmbus_dispatch_is_policy_block_command(uint8_t command)
{
    if ((pmbus_dispatch_is_user_data_command(command) != 0U) ||
        (pmbus_dispatch_is_mfr_policy_block_command(command) != 0U))
    {
        return 1U;
    }

    return 0U;
}

static uint8_t pmbus_dispatch_is_extended_selector(uint8_t command)
{
    if ((command == PMBUS_COMMAND_MFR_SPECIFIC_COMMAND_EXT) ||
        (command == PMBUS_COMMAND_PMBUS_COMMAND_EXT))
    {
        return 1U;
    }

    return 0U;
}

static uint8_t pmbus_dispatch_store_policy_block(uint8_t command, uint8_t *payload, uint8_t data_len)
{
    uint8_t length;
    uint8_t index;
    uint8_t copy_length;
    uint8_t i;

    if ((payload == 0) || (data_len < 1U) || (payload[0] > PMBUS_MAX_BLOCK_SIZE) ||
        ((uint8_t)(payload[0] + 1U) != data_len))
    {
        return 0U;
    }

    length = payload[0];
    copy_length = length;
    if (copy_length > PMBUS_DISPATCH_POLICY_BLOCK_SIZE)
    {
        copy_length = PMBUS_DISPATCH_POLICY_BLOCK_SIZE;
    }

    if (pmbus_dispatch_is_user_data_command(command) != 0U)
    {
        index = (uint8_t)(command - PMBUS_COMMAND_USER_DATA_00);
        g_pmbus_dispatch_user_policy_lengths[index] = copy_length;
        for (i = 0U; i < copy_length; i++)
        {
            g_pmbus_dispatch_user_policy_data[index][i] = payload[(uint8_t)(i + 1U)];
        }
        return 1U;
    }

    if (pmbus_dispatch_is_mfr_policy_block_command(command) != 0U)
    {
        index = (uint8_t)(command - PMBUS_COMMAND_MFR_SPECIFIC_C4);
        g_pmbus_dispatch_mfr_policy_lengths[index] = copy_length;
        for (i = 0U; i < copy_length; i++)
        {
            g_pmbus_dispatch_mfr_policy_data[index][i] = payload[(uint8_t)(i + 1U)];
        }
        return 1U;
    }

    return 0U;
}

static uint8_t pmbus_dispatch_store_extended_policy_block(uint8_t command, uint8_t *payload, uint8_t data_len)
{
    uint8_t length;
    uint8_t copy_length;
    uint8_t i;

    if ((payload == 0) || (data_len < 2U) || (payload[1] > PMBUS_MAX_BLOCK_SIZE) ||
        ((uint8_t)(payload[1] + 2U) != data_len))
    {
        return 0U;
    }

    length = payload[1];
    copy_length = length;
    if (copy_length > PMBUS_DISPATCH_POLICY_BLOCK_SIZE)
    {
        copy_length = PMBUS_DISPATCH_POLICY_BLOCK_SIZE;
    }

    g_pmbus_dispatch_extended_selector = command;
    g_pmbus_dispatch_extended_command = payload[0];
    g_pmbus_dispatch_extended_length = copy_length;
    for (i = 0U; i < copy_length; i++)
    {
        g_pmbus_dispatch_extended_data[i] = payload[(uint8_t)(i + 2U)];
    }

    return 1U;
}

static uint8_t pmbus_dispatch_build_policy_block_response(uint8_t command, uint8_t *tx_buffer, uint8_t *tx_length)
{
    uint8_t length;
    uint8_t index;
    uint8_t i;
    uint8_t *data_ptr;

    length = 0U;
    data_ptr = (uint8_t *)0;

    if (pmbus_dispatch_is_user_data_command(command) != 0U)
    {
        index = (uint8_t)(command - PMBUS_COMMAND_USER_DATA_00);
        length = g_pmbus_dispatch_user_policy_lengths[index];
        data_ptr = g_pmbus_dispatch_user_policy_data[index];
    }
    else if (pmbus_dispatch_is_mfr_policy_block_command(command) != 0U)
    {
        index = (uint8_t)(command - PMBUS_COMMAND_MFR_SPECIFIC_C4);
        length = g_pmbus_dispatch_mfr_policy_lengths[index];
        data_ptr = g_pmbus_dispatch_mfr_policy_data[index];
    }
    else
    {
        return 0U;
    }

    if (length == 0U)
    {
        tx_buffer[0] = 1U;
        tx_buffer[1] = command;
        *tx_length = 2U;
        return 1U;
    }

    tx_buffer[0] = length;
    for (i = 0U; i < length; i++)
    {
        tx_buffer[(uint8_t)(i + 1U)] = data_ptr[i];
    }

    *tx_length = (uint8_t)(length + 1U);
    return 1U;
}

static uint8_t pmbus_dispatch_build_extended_policy_block_response(uint8_t command, uint8_t *payload, uint8_t data_len, uint8_t *tx_buffer, uint8_t *tx_length)
{
    uint8_t ext_command;
    uint8_t i;

    if ((payload == 0) || (data_len < 1U))
    {
        return 0U;
    }

    ext_command = payload[0];
    if ((g_pmbus_dispatch_extended_selector != command) ||
        (g_pmbus_dispatch_extended_command != ext_command) ||
        (g_pmbus_dispatch_extended_length == 0U))
    {
        tx_buffer[0] = 1U;
        tx_buffer[1] = ext_command;
        *tx_length = 2U;
        return 1U;
    }

    tx_buffer[0] = g_pmbus_dispatch_extended_length;
    for (i = 0U; i < g_pmbus_dispatch_extended_length; i++)
    {
        tx_buffer[(uint8_t)(i + 1U)] = g_pmbus_dispatch_extended_data[i];
    }

    *tx_length = (uint8_t)(g_pmbus_dispatch_extended_length + 1U);
    return 1U;
}

static pmbus_dispatch_response_kind_t pmbus_dispatch_get_read_kind(uint8_t command)
{
    if (pmbus_dispatch_is_policy_block_command(command) != 0U)
    {
        return PMBUS_RESP_BLOCK;
    }

    switch (command)
    {
        case PMBUS_COMMAND_PAGE:
        case PMBUS_COMMAND_OPERATION:
        case PMBUS_COMMAND_ON_OFF_CONFIG:
        case PMBUS_COMMAND_PHASE:
        case PMBUS_COMMAND_WRITE_PROTECT:
        case PMBUS_COMMAND_POWER_MODE:
        case PMBUS_COMMAND_FAN_CONFIG_1_2:
        case PMBUS_COMMAND_FAN_CONFIG_3_4:
        case PMBUS_COMMAND_CAPABILITY:
        case PMBUS_COMMAND_VOUT_MODE:
        case PMBUS_COMMAND_VOUT_OV_FAULT_RESPONSE:
        case PMBUS_COMMAND_VOUT_UV_FAULT_RESPONSE:
        case PMBUS_COMMAND_IOUT_OC_FAULT_RESPONSE:
        case PMBUS_COMMAND_IOUT_OC_LV_FAULT_RESPONSE:
        case PMBUS_COMMAND_IOUT_UC_FAULT_RESPONSE:
        case PMBUS_COMMAND_OT_FAULT_RESPONSE:
        case PMBUS_COMMAND_UT_FAULT_RESPONSE:
        case PMBUS_COMMAND_VIN_OV_FAULT_RESPONSE:
        case PMBUS_COMMAND_VIN_UV_FAULT_RESPONSE:
        case PMBUS_COMMAND_IIN_OC_FAULT_RESPONSE:
        case PMBUS_COMMAND_TON_MAX_FAULT_RESPONSE:
        case PMBUS_COMMAND_POUT_OP_FAULT_RESPONSE:
        case PMBUS_COMMAND_STATUS_BYTE:
        case PMBUS_COMMAND_STATUS_VOUT:
        case PMBUS_COMMAND_STATUS_IOUT:
        case PMBUS_COMMAND_STATUS_INPUT:
        case PMBUS_COMMAND_STATUS_TEMPERATURE:
        case PMBUS_COMMAND_STATUS_CML:
        case PMBUS_COMMAND_STATUS_OTHER:
        case PMBUS_COMMAND_STATUS_MFR_SPECIFIC:
        case PMBUS_COMMAND_STATUS_FANS_1_2:
        case PMBUS_COMMAND_STATUS_FANS_3_4:
        case PMBUS_COMMAND_MFR_COLD_REDUNDANCY_CONFIG:
        case PMBUS_COMMAND_MFR_FWUPLOAD_MODE:
        case PMBUS_COMMAND_PMBUS_REVISION:
        case PMBUS_COMMAND_MFR_PIN_ACCURACY:
            return PMBUS_RESP_BYTE;

        case PMBUS_COMMAND_READ_KWH_IN:
        case PMBUS_COMMAND_READ_KWH_OUT:
            return PMBUS_RESP_DWORD;

        case PMBUS_COMMAND_VOUT_OV_FAULT_LIMIT:
        case PMBUS_COMMAND_ZONE_CONFIG:
        case PMBUS_COMMAND_ZONE_ACTIVE:
        case PMBUS_COMMAND_VOUT_OV_WARN_LIMIT:
        case PMBUS_COMMAND_VOUT_UV_WARN_LIMIT:
        case PMBUS_COMMAND_VOUT_UV_FAULT_LIMIT:
        case PMBUS_COMMAND_IOUT_OC_FAULT_LIMIT:
        case PMBUS_COMMAND_IOUT_OC_WARN_LIMIT:
        case PMBUS_COMMAND_OT_FAULT_LIMIT:
        case PMBUS_COMMAND_OT_WARN_LIMIT:
        case PMBUS_COMMAND_VIN_OV_FAULT_LIMIT:
        case PMBUS_COMMAND_VIN_OV_WARN_LIMIT:
        case PMBUS_COMMAND_VIN_UV_WARN_LIMIT:
        case PMBUS_COMMAND_VIN_UV_FAULT_LIMIT:
        case PMBUS_COMMAND_IIN_OC_FAULT_LIMIT:
        case PMBUS_COMMAND_IIN_OC_WARN_LIMIT:
        case PMBUS_COMMAND_FAN_COMMAND_1:
        case PMBUS_COMMAND_FAN_COMMAND_2:
        case PMBUS_COMMAND_FAN_COMMAND_3:
        case PMBUS_COMMAND_FAN_COMMAND_4:
        case PMBUS_COMMAND_READ_KWH_CONFIG:
        case PMBUS_COMMAND_READ_VCAP:
        case PMBUS_COMMAND_VOUT_COMMAND:
        case PMBUS_COMMAND_VOUT_TRIM:
        case PMBUS_COMMAND_VOUT_CAL_OFFSET:
        case PMBUS_COMMAND_VOUT_MAX:
        case PMBUS_COMMAND_VOUT_MARGIN_HIGH:
        case PMBUS_COMMAND_VOUT_MARGIN_LOW:
        case PMBUS_COMMAND_VOUT_TRANSITION_RATE:
        case PMBUS_COMMAND_VOUT_DROOP:
        case PMBUS_COMMAND_VOUT_SCALE_LOOP:
        case PMBUS_COMMAND_VOUT_SCALE_MONITOR:
        case PMBUS_COMMAND_VOUT_MIN:
        case PMBUS_COMMAND_POUT_MAX:
        case PMBUS_COMMAND_MAX_DUTY:
        case PMBUS_COMMAND_FREQUENCY_SWITCH:
        case PMBUS_COMMAND_VIN_ON:
        case PMBUS_COMMAND_VIN_OFF:
        case PMBUS_COMMAND_INTERLEAVE:
        case PMBUS_COMMAND_IOUT_CAL_GAIN:
        case PMBUS_COMMAND_IOUT_CAL_OFFSET:
        case PMBUS_COMMAND_IOUT_OC_LV_FAULT_LIMIT:
        case PMBUS_COMMAND_IOUT_UC_FAULT_LIMIT:
        case PMBUS_COMMAND_UT_WARN_LIMIT:
        case PMBUS_COMMAND_UT_FAULT_LIMIT:
        case PMBUS_COMMAND_POWER_GOOD_ON:
        case PMBUS_COMMAND_POWER_GOOD_OFF:
        case PMBUS_COMMAND_TON_DELAY:
        case PMBUS_COMMAND_TON_RISE:
        case PMBUS_COMMAND_TON_MAX_FAULT_LIMIT:
        case PMBUS_COMMAND_TOFF_DELAY:
        case PMBUS_COMMAND_TOFF_FALL:
        case PMBUS_COMMAND_TOFF_MAX_WARN_LIMIT:
        case PMBUS_COMMAND_POUT_OP_FAULT_LIMIT:
        case PMBUS_COMMAND_POUT_OP_WARN_LIMIT:
        case PMBUS_COMMAND_PIN_OP_WARN_LIMIT:
        case PMBUS_COMMAND_STATUS_WORD:
        case PMBUS_COMMAND_READ_VIN:
        case PMBUS_COMMAND_READ_IIN:
        case PMBUS_COMMAND_READ_VOUT:
        case PMBUS_COMMAND_READ_IOUT:
        case PMBUS_COMMAND_READ_TEMPERATURE_1:
        case PMBUS_COMMAND_READ_TEMPERATURE_2:
        case PMBUS_COMMAND_READ_TEMPERATURE_3:
        case PMBUS_COMMAND_READ_FAN_SPEED_1:
        case PMBUS_COMMAND_READ_FAN_SPEED_2:
        case PMBUS_COMMAND_READ_FAN_SPEED_3:
        case PMBUS_COMMAND_READ_FAN_SPEED_4:
        case PMBUS_COMMAND_READ_DUTY_CYCLE:
        case PMBUS_COMMAND_READ_FREQUENCY:
        case PMBUS_COMMAND_READ_POUT:
        case PMBUS_COMMAND_READ_PIN:
        case PMBUS_COMMAND_MFR_VIN_MIN:
        case PMBUS_COMMAND_MFR_VIN_MAX:
        case PMBUS_COMMAND_MFR_IIN_MAX:
        case PMBUS_COMMAND_MFR_PIN_MAX:
        case PMBUS_COMMAND_MFR_VOUT_MIN:
        case PMBUS_COMMAND_MFR_VOUT_MAX:
        case PMBUS_COMMAND_MFR_IOUT_MAX:
        case PMBUS_COMMAND_MFR_POUT_MAX:
        case PMBUS_COMMAND_MFR_TAMBIENT_MAX:
        case PMBUS_COMMAND_MFR_TAMBIENT_MIN:
        case PMBUS_COMMAND_MFR_MAX_TEMP_1:
        case PMBUS_COMMAND_MFR_MAX_TEMP_2:
        case PMBUS_COMMAND_MFR_MAX_TEMP_3:
        case PMBUS_COMMAND_MFR_FWUPLOAD_STATUS:
            return PMBUS_RESP_WORD;

        case PMBUS_COMMAND_READ_EIN:
        case PMBUS_COMMAND_READ_EOUT:
        case PMBUS_COMMAND_MFR_ID:
        case PMBUS_COMMAND_MFR_MODEL:
        case PMBUS_COMMAND_MFR_REVISION:
        case PMBUS_COMMAND_MFR_LOCATION:
        case PMBUS_COMMAND_MFR_DATE:
        case PMBUS_COMMAND_MFR_SERIAL:
        case PMBUS_COMMAND_APP_PROFILE_SUPPORT:
        case PMBUS_COMMAND_MFR_EFFICIENCY_LL:
        case PMBUS_COMMAND_MFR_EFFICIENCY_HL:
        case PMBUS_COMMAND_IC_DEVICE_ID:
        case PMBUS_COMMAND_IC_DEVICE_REV:
        case PMBUS_COMMAND_MFR_BLACKBOX:
            return PMBUS_RESP_BLOCK;

        default:
            return PMBUS_RESP_NONE;
    }
}

static uint8_t pmbus_dispatch_is_write_byte_supported(uint8_t command)
{
    switch (command)
    {
        case PMBUS_COMMAND_PAGE:
        case PMBUS_COMMAND_OPERATION:
        case PMBUS_COMMAND_ON_OFF_CONFIG:
        case PMBUS_COMMAND_PHASE:
        case PMBUS_COMMAND_STORE_DEFAULT_CODE:
        case PMBUS_COMMAND_RESTORE_DEFAULT_CODE:
        case PMBUS_COMMAND_STORE_USER_CODE:
        case PMBUS_COMMAND_RESTORE_USER_CODE:
        case PMBUS_COMMAND_WRITE_PROTECT:
        case PMBUS_COMMAND_VOUT_MODE:
        case PMBUS_COMMAND_POWER_MODE:
        case PMBUS_COMMAND_FAN_CONFIG_1_2:
        case PMBUS_COMMAND_FAN_CONFIG_3_4:
        case PMBUS_COMMAND_VOUT_OV_FAULT_RESPONSE:
        case PMBUS_COMMAND_VOUT_UV_FAULT_RESPONSE:
        case PMBUS_COMMAND_IOUT_OC_FAULT_RESPONSE:
        case PMBUS_COMMAND_IOUT_OC_LV_FAULT_RESPONSE:
        case PMBUS_COMMAND_IOUT_UC_FAULT_RESPONSE:
        case PMBUS_COMMAND_OT_FAULT_RESPONSE:
        case PMBUS_COMMAND_UT_FAULT_RESPONSE:
        case PMBUS_COMMAND_VIN_OV_FAULT_RESPONSE:
        case PMBUS_COMMAND_VIN_UV_FAULT_RESPONSE:
        case PMBUS_COMMAND_IIN_OC_FAULT_RESPONSE:
        case PMBUS_COMMAND_TON_MAX_FAULT_RESPONSE:
        case PMBUS_COMMAND_POUT_OP_FAULT_RESPONSE:
        case PMBUS_COMMAND_MFR_COLD_REDUNDANCY_CONFIG:
        case PMBUS_COMMAND_MFR_FWUPLOAD_MODE:
            return 1U;

        default:
            return 0U;
    }
}

static uint8_t pmbus_dispatch_is_write_word_supported(uint8_t command)
{
    switch (command)
    {
        case PMBUS_COMMAND_VOUT_OV_FAULT_LIMIT:
        case PMBUS_COMMAND_VOUT_OV_WARN_LIMIT:
        case PMBUS_COMMAND_VOUT_UV_WARN_LIMIT:
        case PMBUS_COMMAND_VOUT_UV_FAULT_LIMIT:
        case PMBUS_COMMAND_IOUT_OC_FAULT_LIMIT:
        case PMBUS_COMMAND_IOUT_OC_WARN_LIMIT:
        case PMBUS_COMMAND_OT_FAULT_LIMIT:
        case PMBUS_COMMAND_OT_WARN_LIMIT:
        case PMBUS_COMMAND_VIN_OV_FAULT_LIMIT:
        case PMBUS_COMMAND_VIN_OV_WARN_LIMIT:
        case PMBUS_COMMAND_VIN_UV_WARN_LIMIT:
        case PMBUS_COMMAND_VIN_UV_FAULT_LIMIT:
        case PMBUS_COMMAND_IIN_OC_FAULT_LIMIT:
        case PMBUS_COMMAND_IIN_OC_WARN_LIMIT:
        case PMBUS_COMMAND_FAN_COMMAND_1:
        case PMBUS_COMMAND_FAN_COMMAND_2:
        case PMBUS_COMMAND_FAN_COMMAND_3:
        case PMBUS_COMMAND_FAN_COMMAND_4:
        case PMBUS_COMMAND_VOUT_COMMAND:
        case PMBUS_COMMAND_VOUT_TRIM:
        case PMBUS_COMMAND_VOUT_CAL_OFFSET:
        case PMBUS_COMMAND_VOUT_MAX:
        case PMBUS_COMMAND_VOUT_MARGIN_HIGH:
        case PMBUS_COMMAND_VOUT_MARGIN_LOW:
        case PMBUS_COMMAND_VOUT_TRANSITION_RATE:
        case PMBUS_COMMAND_VOUT_DROOP:
        case PMBUS_COMMAND_VOUT_SCALE_LOOP:
        case PMBUS_COMMAND_VOUT_SCALE_MONITOR:
        case PMBUS_COMMAND_VOUT_MIN:
        case PMBUS_COMMAND_POUT_MAX:
        case PMBUS_COMMAND_MAX_DUTY:
        case PMBUS_COMMAND_FREQUENCY_SWITCH:
        case PMBUS_COMMAND_VIN_ON:
        case PMBUS_COMMAND_VIN_OFF:
        case PMBUS_COMMAND_INTERLEAVE:
        case PMBUS_COMMAND_IOUT_CAL_GAIN:
        case PMBUS_COMMAND_IOUT_CAL_OFFSET:
        case PMBUS_COMMAND_IOUT_OC_LV_FAULT_LIMIT:
        case PMBUS_COMMAND_IOUT_UC_FAULT_LIMIT:
        case PMBUS_COMMAND_UT_WARN_LIMIT:
        case PMBUS_COMMAND_UT_FAULT_LIMIT:
        case PMBUS_COMMAND_POWER_GOOD_ON:
        case PMBUS_COMMAND_POWER_GOOD_OFF:
        case PMBUS_COMMAND_TON_DELAY:
        case PMBUS_COMMAND_TON_RISE:
        case PMBUS_COMMAND_TON_MAX_FAULT_LIMIT:
        case PMBUS_COMMAND_TOFF_DELAY:
        case PMBUS_COMMAND_TOFF_FALL:
        case PMBUS_COMMAND_TOFF_MAX_WARN_LIMIT:
        case PMBUS_COMMAND_POUT_OP_FAULT_LIMIT:
        case PMBUS_COMMAND_POUT_OP_WARN_LIMIT:
        case PMBUS_COMMAND_PIN_OP_WARN_LIMIT:
        case PMBUS_COMMAND_SMBALERT_MASK:
        case PMBUS_COMMAND_ZONE_CONFIG:
        case PMBUS_COMMAND_ZONE_ACTIVE:
            return 1U;

        default:
            return 0U;
    }
}

uint8_t pmbus_dispatch_is_known_command(uint8_t command)
{
    if (pmbus_dispatch_is_extended_selector(command) != 0U)
    {
        return 1U;
    }

    if (pmbus_dispatch_get_read_kind(command) != PMBUS_RESP_NONE)
    {
        return 1U;
    }

    if (pmbus_dispatch_is_send_byte_supported(command) != 0U)
    {
        return 1U;
    }

    if (pmbus_dispatch_is_write_byte_supported(command) != 0U)
    {
        return 1U;
    }

    if (pmbus_dispatch_is_write_word_supported(command) != 0U)
    {
        return 1U;
    }

    if (pmbus_dispatch_is_block_write_supported(command) != 0U)
    {
        return 1U;
    }

    if (pmbus_dispatch_is_block_write_read_process_call_supported(command) != 0U)
    {
        return 1U;
    }

    return 0U;
}

static uint8_t pmbus_dispatch_is_send_byte_supported(uint8_t command)
{
    if (command == PMBUS_COMMAND_CLEAR_FAULTS)
    {
        return 1U;
    }

    if ((command == PMBUS_COMMAND_STORE_DEFAULT_ALL) ||
        (command == PMBUS_COMMAND_RESTORE_DEFAULT_ALL) ||
        (command == PMBUS_COMMAND_STORE_USER_ALL) ||
        (command == PMBUS_COMMAND_RESTORE_USER_ALL))
    {
        return 1U;
    }

    return 0U;
}

static uint8_t pmbus_dispatch_is_block_write_supported(uint8_t command)
{
    if ((pmbus_dispatch_is_policy_block_command(command) != 0U) ||
        (pmbus_dispatch_is_extended_selector(command) != 0U))
    {
        return 1U;
    }

    if ((command == PMBUS_COMMAND_PAGE_PLUS_WRITE) ||
        (command == PMBUS_COMMAND_MFR_FWUPLOAD))
    {
        return 1U;
    }

    return 0U;
}

static uint8_t pmbus_dispatch_is_block_write_read_process_call_supported(uint8_t command)
{
    if (pmbus_dispatch_is_extended_selector(command) != 0U)
    {
        return 1U;
    }

    if ((command == PMBUS_COMMAND_PAGE_PLUS_READ) ||
        (command == PMBUS_COMMAND_QUERY) ||
        (command == PMBUS_COMMAND_COEFFICIENTS) ||
        (command == PMBUS_COMMAND_SMBALERT_MASK))
    {
        return 1U;
    }

    return 0U;
}

static uint8_t pmbus_dispatch_is_write_locked(void)
{
    if (pmbus_app_get_write_protect() != 0U)
    {
        return 1U;
    }

    return 0U;
}

static uint8_t pmbus_dispatch_get_query_data_format(uint8_t command)
{
    if ((pmbus_dispatch_is_policy_block_command(command) != 0U) ||
        (pmbus_dispatch_is_extended_selector(command) != 0U))
    {
        return 0x07U;
    }

    switch (command)
    {
        case PMBUS_COMMAND_PAGE:
        case PMBUS_COMMAND_OPERATION:
        case PMBUS_COMMAND_ON_OFF_CONFIG:
        case PMBUS_COMMAND_PHASE:
        case PMBUS_COMMAND_STORE_DEFAULT_CODE:
        case PMBUS_COMMAND_RESTORE_DEFAULT_CODE:
        case PMBUS_COMMAND_STORE_USER_CODE:
        case PMBUS_COMMAND_RESTORE_USER_CODE:
        case PMBUS_COMMAND_WRITE_PROTECT:
        case PMBUS_COMMAND_CAPABILITY:
        case PMBUS_COMMAND_VOUT_MODE:
        case PMBUS_COMMAND_POWER_MODE:
        case PMBUS_COMMAND_FAN_CONFIG_1_2:
        case PMBUS_COMMAND_FAN_CONFIG_3_4:
        case PMBUS_COMMAND_VOUT_OV_FAULT_RESPONSE:
        case PMBUS_COMMAND_VOUT_UV_FAULT_RESPONSE:
        case PMBUS_COMMAND_IOUT_OC_FAULT_RESPONSE:
        case PMBUS_COMMAND_IOUT_OC_LV_FAULT_RESPONSE:
        case PMBUS_COMMAND_IOUT_UC_FAULT_RESPONSE:
        case PMBUS_COMMAND_OT_FAULT_RESPONSE:
        case PMBUS_COMMAND_UT_FAULT_RESPONSE:
        case PMBUS_COMMAND_VIN_OV_FAULT_RESPONSE:
        case PMBUS_COMMAND_VIN_UV_FAULT_RESPONSE:
        case PMBUS_COMMAND_IIN_OC_FAULT_RESPONSE:
        case PMBUS_COMMAND_TON_MAX_FAULT_RESPONSE:
        case PMBUS_COMMAND_POUT_OP_FAULT_RESPONSE:
        case PMBUS_COMMAND_STATUS_BYTE:
        case PMBUS_COMMAND_STATUS_VOUT:
        case PMBUS_COMMAND_STATUS_IOUT:
        case PMBUS_COMMAND_STATUS_INPUT:
        case PMBUS_COMMAND_STATUS_TEMPERATURE:
        case PMBUS_COMMAND_STATUS_CML:
        case PMBUS_COMMAND_STATUS_OTHER:
        case PMBUS_COMMAND_STATUS_MFR_SPECIFIC:
        case PMBUS_COMMAND_STATUS_FANS_1_2:
        case PMBUS_COMMAND_STATUS_FANS_3_4:
        case PMBUS_COMMAND_MFR_COLD_REDUNDANCY_CONFIG:
        case PMBUS_COMMAND_MFR_FWUPLOAD_MODE:
        case PMBUS_COMMAND_PMBUS_REVISION:
        case PMBUS_COMMAND_MFR_PIN_ACCURACY:
            return 0x04U;

        case PMBUS_COMMAND_READ_KWH_IN:
        case PMBUS_COMMAND_READ_KWH_OUT:
            return 0x03U;

        case PMBUS_COMMAND_VOUT_OV_FAULT_LIMIT:
        case PMBUS_COMMAND_VOUT_OV_WARN_LIMIT:
        case PMBUS_COMMAND_VOUT_UV_WARN_LIMIT:
        case PMBUS_COMMAND_VOUT_UV_FAULT_LIMIT:
        case PMBUS_COMMAND_IOUT_OC_FAULT_LIMIT:
        case PMBUS_COMMAND_IOUT_OC_WARN_LIMIT:
        case PMBUS_COMMAND_OT_FAULT_LIMIT:
        case PMBUS_COMMAND_OT_WARN_LIMIT:
        case PMBUS_COMMAND_VIN_OV_FAULT_LIMIT:
        case PMBUS_COMMAND_VIN_OV_WARN_LIMIT:
        case PMBUS_COMMAND_VIN_UV_WARN_LIMIT:
        case PMBUS_COMMAND_VIN_UV_FAULT_LIMIT:
        case PMBUS_COMMAND_IIN_OC_FAULT_LIMIT:
        case PMBUS_COMMAND_IIN_OC_WARN_LIMIT:
        case PMBUS_COMMAND_FAN_COMMAND_1:
        case PMBUS_COMMAND_FAN_COMMAND_2:
        case PMBUS_COMMAND_FAN_COMMAND_3:
        case PMBUS_COMMAND_FAN_COMMAND_4:
        case PMBUS_COMMAND_VOUT_COMMAND:
        case PMBUS_COMMAND_VOUT_TRIM:
        case PMBUS_COMMAND_VOUT_CAL_OFFSET:
        case PMBUS_COMMAND_VOUT_MAX:
        case PMBUS_COMMAND_VOUT_MARGIN_HIGH:
        case PMBUS_COMMAND_VOUT_MARGIN_LOW:
        case PMBUS_COMMAND_VOUT_TRANSITION_RATE:
        case PMBUS_COMMAND_VOUT_DROOP:
        case PMBUS_COMMAND_VOUT_SCALE_LOOP:
        case PMBUS_COMMAND_VOUT_SCALE_MONITOR:
        case PMBUS_COMMAND_VOUT_MIN:
        case PMBUS_COMMAND_POUT_MAX:
        case PMBUS_COMMAND_MAX_DUTY:
        case PMBUS_COMMAND_FREQUENCY_SWITCH:
        case PMBUS_COMMAND_VIN_ON:
        case PMBUS_COMMAND_VIN_OFF:
        case PMBUS_COMMAND_IOUT_CAL_GAIN:
        case PMBUS_COMMAND_IOUT_CAL_OFFSET:
        case PMBUS_COMMAND_IOUT_OC_LV_FAULT_LIMIT:
        case PMBUS_COMMAND_IOUT_UC_FAULT_LIMIT:
        case PMBUS_COMMAND_UT_WARN_LIMIT:
        case PMBUS_COMMAND_UT_FAULT_LIMIT:
        case PMBUS_COMMAND_POWER_GOOD_ON:
        case PMBUS_COMMAND_POWER_GOOD_OFF:
        case PMBUS_COMMAND_TON_DELAY:
        case PMBUS_COMMAND_TON_RISE:
        case PMBUS_COMMAND_TON_MAX_FAULT_LIMIT:
        case PMBUS_COMMAND_TOFF_DELAY:
        case PMBUS_COMMAND_TOFF_FALL:
        case PMBUS_COMMAND_TOFF_MAX_WARN_LIMIT:
        case PMBUS_COMMAND_POUT_OP_FAULT_LIMIT:
        case PMBUS_COMMAND_POUT_OP_WARN_LIMIT:
        case PMBUS_COMMAND_PIN_OP_WARN_LIMIT:
        case PMBUS_COMMAND_READ_VIN:
        case PMBUS_COMMAND_READ_IIN:
        case PMBUS_COMMAND_READ_KWH_CONFIG:
        case PMBUS_COMMAND_READ_VCAP:
        case PMBUS_COMMAND_READ_VOUT:
        case PMBUS_COMMAND_READ_IOUT:
        case PMBUS_COMMAND_READ_TEMPERATURE_1:
        case PMBUS_COMMAND_READ_TEMPERATURE_2:
        case PMBUS_COMMAND_READ_TEMPERATURE_3:
        case PMBUS_COMMAND_READ_FAN_SPEED_1:
        case PMBUS_COMMAND_READ_FAN_SPEED_2:
        case PMBUS_COMMAND_READ_FAN_SPEED_3:
        case PMBUS_COMMAND_READ_FAN_SPEED_4:
        case PMBUS_COMMAND_READ_DUTY_CYCLE:
        case PMBUS_COMMAND_READ_FREQUENCY:
        case PMBUS_COMMAND_READ_POUT:
        case PMBUS_COMMAND_READ_PIN:
        case PMBUS_COMMAND_MFR_VIN_MIN:
        case PMBUS_COMMAND_MFR_VIN_MAX:
        case PMBUS_COMMAND_MFR_IIN_MAX:
        case PMBUS_COMMAND_MFR_PIN_MAX:
        case PMBUS_COMMAND_MFR_VOUT_MIN:
        case PMBUS_COMMAND_MFR_VOUT_MAX:
        case PMBUS_COMMAND_MFR_IOUT_MAX:
        case PMBUS_COMMAND_MFR_POUT_MAX:
        case PMBUS_COMMAND_MFR_TAMBIENT_MAX:
        case PMBUS_COMMAND_MFR_TAMBIENT_MIN:
        case PMBUS_COMMAND_MFR_MAX_TEMP_1:
        case PMBUS_COMMAND_MFR_MAX_TEMP_2:
        case PMBUS_COMMAND_MFR_MAX_TEMP_3:
            return 0x00U;

        case PMBUS_COMMAND_STATUS_WORD:
        case PMBUS_COMMAND_MFR_FWUPLOAD_STATUS:
        case PMBUS_COMMAND_SMBALERT_MASK:
        case PMBUS_COMMAND_ZONE_CONFIG:
        case PMBUS_COMMAND_ZONE_ACTIVE:
        case PMBUS_COMMAND_INTERLEAVE:
            return 0x07U;

        case PMBUS_COMMAND_PAGE_PLUS_WRITE:
        case PMBUS_COMMAND_PAGE_PLUS_READ:
        case PMBUS_COMMAND_COEFFICIENTS:
        case PMBUS_COMMAND_READ_EIN:
        case PMBUS_COMMAND_READ_EOUT:
        case PMBUS_COMMAND_APP_PROFILE_SUPPORT:
        case PMBUS_COMMAND_MFR_EFFICIENCY_LL:
        case PMBUS_COMMAND_MFR_EFFICIENCY_HL:
        case PMBUS_COMMAND_MFR_BLACKBOX:
        case PMBUS_COMMAND_MFR_FWUPLOAD:
            return 0x06U;

        case PMBUS_COMMAND_MFR_ID:
        case PMBUS_COMMAND_MFR_MODEL:
        case PMBUS_COMMAND_MFR_REVISION:
        case PMBUS_COMMAND_MFR_LOCATION:
        case PMBUS_COMMAND_MFR_DATE:
        case PMBUS_COMMAND_MFR_SERIAL:
        case PMBUS_COMMAND_IC_DEVICE_ID:
        case PMBUS_COMMAND_IC_DEVICE_REV:
            return 0x07U;

        default:
            return 0x07U;
    }
}

static uint8_t pmbus_dispatch_get_query_response(uint8_t command)
{
    uint8_t response;
    pmbus_dispatch_response_kind_t read_kind;

    response = 0U;

    if (pmbus_dispatch_is_known_command(command) == 0U)
    {
        return 0U;
    }

    response = 0x80U;
    read_kind = pmbus_dispatch_get_read_kind(command);

    if ((read_kind != PMBUS_RESP_NONE) ||
        (pmbus_dispatch_is_block_write_read_process_call_supported(command) != 0U))
    {
        response = (uint8_t)(response | 0x20U);
    }

    if ((command != PMBUS_COMMAND_QUERY) &&
        ((pmbus_dispatch_is_send_byte_supported(command) != 0U) ||
         (pmbus_dispatch_is_write_byte_supported(command) != 0U) ||
         (pmbus_dispatch_is_write_word_supported(command) != 0U) ||
         (pmbus_dispatch_is_block_write_supported(command) != 0U) ||
         (command == PMBUS_COMMAND_VOUT_COMMAND)))
    {
        response = (uint8_t)(response | 0x40U);
    }

    if (command == PMBUS_COMMAND_SMBALERT_MASK)
    {
        response = (uint8_t)(response | 0x40U);
    }

    response = (uint8_t)(response | (uint8_t)(pmbus_dispatch_get_query_data_format(command) << 2));
    return response;
}

static uint8_t pmbus_dispatch_build_query_response(uint8_t *payload, uint8_t data_len, uint8_t *tx_buffer, uint8_t *tx_length)
{
    if ((payload == 0) || (data_len != 2U) || (payload[0] != 1U))
    {
        return 0U;
    }

    tx_buffer[0] = 1U;
    tx_buffer[1] = pmbus_dispatch_get_query_response(payload[1]);
    *tx_length = 2U;
    return 1U;
}

static uint8_t pmbus_dispatch_build_smbalert_mask_response(uint8_t *payload, uint8_t data_len, uint8_t *tx_buffer, uint8_t *tx_length)
{
    uint16_t value;

    if ((payload == 0) || (data_len != 1U) || (payload[0] != 0U))
    {
        return 0U;
    }

    value = pmbus_app_get_smbalert_mask();
    tx_buffer[0] = 2U;
    pmbus_dispatch_store_word(&tx_buffer[1], value);
    *tx_length = 3U;
    return 1U;
}

static uint8_t pmbus_dispatch_build_coefficients_response(uint8_t *payload, uint8_t data_len, uint8_t *tx_buffer, uint8_t *tx_length)
{
    uint8_t *data_ptr;
    uint8_t length;
    uint8_t index;

    data_ptr = (uint8_t *)0;
    length = 0U;

    if ((payload == 0) || (data_len != 2U) || (payload[0] != 1U))
    {
        return 0U;
    }

    length = pmbus_app_get_coefficients(payload[1], &data_ptr);
    if ((data_ptr == 0) || (length != 5U))
    {
        return 0U;
    }

    tx_buffer[0] = length;
    for (index = 0U; index < length; index++)
    {
        tx_buffer[(uint8_t)(index + 1U)] = data_ptr[index];
    }

    *tx_length = (uint8_t)(length + 1U);
    return 1U;
}

static uint8_t pmbus_dispatch_build_page_plus_read_response(uint8_t *payload, uint8_t data_len, uint8_t *tx_buffer, uint8_t *tx_length)
{
    pmbus_dispatch_response_kind_t read_kind;
    uint8_t saved_page;
    uint8_t page;
    uint8_t target_command;
    uint8_t result;

    if ((payload == 0) ||
        (data_len < 3U) ||
        (payload[0] < 2U) ||
        ((uint8_t)(payload[0] + 1U) != data_len))
    {
        return 0U;
    }

    page = payload[1];
    target_command = payload[2];
    if ((target_command == PMBUS_COMMAND_PAGE_PLUS_WRITE) ||
        (target_command == PMBUS_COMMAND_PAGE_PLUS_READ))
    {
        return 0U;
    }

    pmbus_app_record_page_plus_read(page, target_command);
    saved_page = pmbus_app_get_page();
    pmbus_app_set_page(page);

    result = 0U;
    read_kind = pmbus_dispatch_get_read_kind(target_command);

    switch (read_kind)
    {
        case PMBUS_RESP_BYTE:
            if (pmbus_dispatch_build_byte_response(target_command, &tx_buffer[1], tx_length) != 0U)
            {
                tx_buffer[0] = *tx_length;
                *tx_length = (uint8_t)(*tx_length + 1U);
                result = 1U;
            }
            break;

        case PMBUS_RESP_WORD:
            if (pmbus_dispatch_build_word_response(target_command, &tx_buffer[1], tx_length) != 0U)
            {
                tx_buffer[0] = *tx_length;
                *tx_length = (uint8_t)(*tx_length + 1U);
                result = 1U;
            }
            break;

        case PMBUS_RESP_DWORD:
            if (pmbus_dispatch_build_dword_response(target_command, &tx_buffer[1], tx_length) != 0U)
            {
                tx_buffer[0] = *tx_length;
                *tx_length = (uint8_t)(*tx_length + 1U);
                result = 1U;
            }
            break;

        case PMBUS_RESP_BLOCK:
            if (pmbus_dispatch_build_block_response(target_command, tx_buffer, tx_length) != 0U)
            {
                result = 1U;
            }
            break;

        default:
            break;
    }

    pmbus_app_set_page(saved_page);
    return result;
}

static uint8_t pmbus_dispatch_build_byte_response(uint8_t command, uint8_t *tx_buffer, uint8_t *tx_length)
{
    switch (command)
    {
        case PMBUS_COMMAND_PAGE:
            tx_buffer[0] = pmbus_app_get_page();
            break;

        case PMBUS_COMMAND_OPERATION:
            tx_buffer[0] = pmbus_app_get_operation();
            break;

        case PMBUS_COMMAND_ON_OFF_CONFIG:
            tx_buffer[0] = pmbus_app_get_on_off_config();
            break;

        case PMBUS_COMMAND_PHASE:
            tx_buffer[0] = pmbus_app_get_phase();
            break;

        case PMBUS_COMMAND_WRITE_PROTECT:
            tx_buffer[0] = pmbus_app_get_write_protect();
            break;

        case PMBUS_COMMAND_FAN_CONFIG_1_2:
            tx_buffer[0] = pmbus_app_get_fan_config_1_2();
            break;

        case PMBUS_COMMAND_FAN_CONFIG_3_4:
            tx_buffer[0] = pmbus_app_get_fan_config_3_4();
            break;

        case PMBUS_COMMAND_CAPABILITY:
            tx_buffer[0] = pmbus_app_get_capability();
            break;

        case PMBUS_COMMAND_VOUT_MODE:
            tx_buffer[0] = pmbus_app_get_vout_mode();
            break;

        case PMBUS_COMMAND_POWER_MODE:
            tx_buffer[0] = pmbus_app_get_power_mode();
            break;

        case PMBUS_COMMAND_VOUT_OV_FAULT_RESPONSE:
            tx_buffer[0] = pmbus_app_get_vout_ov_fault_response();
            break;

        case PMBUS_COMMAND_VOUT_UV_FAULT_RESPONSE:
            tx_buffer[0] = pmbus_app_get_vout_uv_fault_response();
            break;

        case PMBUS_COMMAND_IOUT_OC_FAULT_RESPONSE:
            tx_buffer[0] = pmbus_app_get_iout_oc_fault_response();
            break;

        case PMBUS_COMMAND_IOUT_OC_LV_FAULT_RESPONSE:
            tx_buffer[0] = pmbus_app_get_iout_oc_lv_fault_response();
            break;

        case PMBUS_COMMAND_IOUT_UC_FAULT_RESPONSE:
            tx_buffer[0] = pmbus_app_get_iout_uc_fault_response();
            break;

        case PMBUS_COMMAND_OT_FAULT_RESPONSE:
            tx_buffer[0] = pmbus_app_get_ot_fault_response();
            break;

        case PMBUS_COMMAND_UT_FAULT_RESPONSE:
            tx_buffer[0] = pmbus_app_get_ut_fault_response();
            break;

        case PMBUS_COMMAND_VIN_OV_FAULT_RESPONSE:
            tx_buffer[0] = pmbus_app_get_vin_ov_fault_response();
            break;

        case PMBUS_COMMAND_VIN_UV_FAULT_RESPONSE:
            tx_buffer[0] = pmbus_app_get_vin_uv_fault_response();
            break;

        case PMBUS_COMMAND_IIN_OC_FAULT_RESPONSE:
            tx_buffer[0] = pmbus_app_get_iin_oc_fault_response();
            break;

        case PMBUS_COMMAND_TON_MAX_FAULT_RESPONSE:
            tx_buffer[0] = pmbus_app_get_ton_max_fault_response();
            break;

        case PMBUS_COMMAND_POUT_OP_FAULT_RESPONSE:
            tx_buffer[0] = pmbus_app_get_pout_op_fault_response();
            break;

        case PMBUS_COMMAND_STATUS_BYTE:
            tx_buffer[0] = pmbus_app_get_status_byte();
            break;

        case PMBUS_COMMAND_STATUS_VOUT:
            tx_buffer[0] = pmbus_app_get_status_vout();
            break;

        case PMBUS_COMMAND_STATUS_IOUT:
            tx_buffer[0] = pmbus_app_get_status_iout();
            break;

        case PMBUS_COMMAND_STATUS_INPUT:
            tx_buffer[0] = pmbus_app_get_status_input();
            break;

        case PMBUS_COMMAND_STATUS_TEMPERATURE:
            tx_buffer[0] = pmbus_app_get_status_temperature();
            break;

        case PMBUS_COMMAND_STATUS_CML:
            tx_buffer[0] = pmbus_app_get_status_cml();
            break;

        case PMBUS_COMMAND_STATUS_OTHER:
            tx_buffer[0] = pmbus_app_get_status_other();
            break;

        case PMBUS_COMMAND_STATUS_MFR_SPECIFIC:
            tx_buffer[0] = pmbus_app_get_status_mfr_specific();
            break;

        case PMBUS_COMMAND_STATUS_FANS_1_2:
            tx_buffer[0] = pmbus_app_get_status_fans_1_2();
            break;

        case PMBUS_COMMAND_STATUS_FANS_3_4:
            tx_buffer[0] = 0x00U;
            break;

        case PMBUS_COMMAND_MFR_COLD_REDUNDANCY_CONFIG:
            tx_buffer[0] = pmbus_app_get_mfr_cold_redundancy_config();
            break;

        case PMBUS_COMMAND_MFR_FWUPLOAD_MODE:
            tx_buffer[0] = pmbus_app_get_mfr_fwupload_mode();
            break;

        case PMBUS_COMMAND_PMBUS_REVISION:
            tx_buffer[0] = pmbus_app_get_pmbus_revision();
            break;

        case PMBUS_COMMAND_MFR_PIN_ACCURACY:
            tx_buffer[0] = 50U;
            break;

        default:
            return 0U;
    }

    *tx_length = 1U;
    return 1U;
}

static uint8_t pmbus_dispatch_build_word_response(uint8_t command, uint8_t *tx_buffer, uint8_t *tx_length)
{
    uint16_t value;

    value = 0U;

    switch (command)
    {
        case PMBUS_COMMAND_ZONE_CONFIG:
            value = pmbus_app_get_zone_config();
            break;

        case PMBUS_COMMAND_ZONE_ACTIVE:
            value = pmbus_app_get_zone_active();
            break;

        case PMBUS_COMMAND_VOUT_OV_FAULT_LIMIT:
            value = pmbus_app_get_vout_ov_fault_limit();
            break;

        case PMBUS_COMMAND_VOUT_OV_WARN_LIMIT:
            value = pmbus_app_get_vout_ov_warn_limit();
            break;

        case PMBUS_COMMAND_VOUT_UV_WARN_LIMIT:
            value = pmbus_app_get_vout_uv_warn_limit();
            break;

        case PMBUS_COMMAND_VOUT_UV_FAULT_LIMIT:
            value = pmbus_app_get_vout_uv_fault_limit();
            break;

        case PMBUS_COMMAND_IOUT_OC_FAULT_LIMIT:
            value = pmbus_app_get_iout_oc_fault_limit();
            break;

        case PMBUS_COMMAND_IOUT_OC_LV_FAULT_LIMIT:
            value = pmbus_app_get_iout_oc_lv_fault_limit();
            break;

        case PMBUS_COMMAND_IOUT_OC_WARN_LIMIT:
            value = pmbus_app_get_iout_oc_warn_limit();
            break;

        case PMBUS_COMMAND_IOUT_UC_FAULT_LIMIT:
            value = pmbus_app_get_iout_uc_fault_limit();
            break;

        case PMBUS_COMMAND_OT_FAULT_LIMIT:
            value = pmbus_app_get_ot_fault_limit();
            break;

        case PMBUS_COMMAND_OT_WARN_LIMIT:
            value = pmbus_app_get_ot_warn_limit();
            break;

        case PMBUS_COMMAND_UT_WARN_LIMIT:
            value = pmbus_app_get_ut_warn_limit();
            break;

        case PMBUS_COMMAND_UT_FAULT_LIMIT:
            value = pmbus_app_get_ut_fault_limit();
            break;

        case PMBUS_COMMAND_VIN_OV_FAULT_LIMIT:
            value = pmbus_app_get_vin_ov_fault_limit();
            break;

        case PMBUS_COMMAND_VIN_OV_WARN_LIMIT:
            value = pmbus_app_get_vin_ov_warn_limit();
            break;

        case PMBUS_COMMAND_VIN_UV_WARN_LIMIT:
            value = pmbus_app_get_vin_uv_warn_limit();
            break;

        case PMBUS_COMMAND_VIN_UV_FAULT_LIMIT:
            value = pmbus_app_get_vin_uv_fault_limit();
            break;

        case PMBUS_COMMAND_IIN_OC_FAULT_LIMIT:
            value = pmbus_app_get_iin_oc_fault_limit();
            break;

        case PMBUS_COMMAND_IIN_OC_WARN_LIMIT:
            value = pmbus_app_get_iin_oc_warn_limit();
            break;

        case PMBUS_COMMAND_FAN_COMMAND_1:
            value = pmbus_app_get_fan_command_1();
            break;

        case PMBUS_COMMAND_FAN_COMMAND_2:
            value = pmbus_app_get_fan_command_2();
            break;

        case PMBUS_COMMAND_FAN_COMMAND_3:
            value = pmbus_app_get_fan_command_3();
            break;

        case PMBUS_COMMAND_FAN_COMMAND_4:
            value = pmbus_app_get_fan_command_4();
            break;

        case PMBUS_COMMAND_READ_KWH_CONFIG:
            value = 0x0000U;
            break;

        case PMBUS_COMMAND_READ_VCAP:
            value = pmbus_app_get_read_vin();
            break;

        case PMBUS_COMMAND_VOUT_COMMAND:
            value = pmbus_app_get_vout_command();
            break;

        case PMBUS_COMMAND_VOUT_TRIM:
            value = pmbus_app_get_vout_trim();
            break;

        case PMBUS_COMMAND_VOUT_CAL_OFFSET:
            value = pmbus_app_get_vout_cal_offset();
            break;

        case PMBUS_COMMAND_VOUT_MAX:
            value = pmbus_app_get_vout_max();
            break;

        case PMBUS_COMMAND_VOUT_MARGIN_HIGH:
            value = pmbus_app_get_vout_margin_high();
            break;

        case PMBUS_COMMAND_VOUT_MARGIN_LOW:
            value = pmbus_app_get_vout_margin_low();
            break;

        case PMBUS_COMMAND_VOUT_TRANSITION_RATE:
            value = pmbus_app_get_vout_transition_rate();
            break;

        case PMBUS_COMMAND_VOUT_DROOP:
            value = pmbus_app_get_vout_droop();
            break;

        case PMBUS_COMMAND_VOUT_SCALE_LOOP:
            value = pmbus_app_get_vout_scale_loop();
            break;

        case PMBUS_COMMAND_VOUT_SCALE_MONITOR:
            value = pmbus_app_get_vout_scale_monitor();
            break;

        case PMBUS_COMMAND_VOUT_MIN:
            value = pmbus_app_get_vout_min();
            break;

        case PMBUS_COMMAND_POUT_MAX:
            value = pmbus_app_get_pout_max();
            break;

        case PMBUS_COMMAND_MAX_DUTY:
            value = pmbus_app_get_max_duty();
            break;

        case PMBUS_COMMAND_FREQUENCY_SWITCH:
            value = pmbus_app_get_frequency_switch();
            break;

        case PMBUS_COMMAND_VIN_ON:
            value = pmbus_app_get_vin_on();
            break;

        case PMBUS_COMMAND_VIN_OFF:
            value = pmbus_app_get_vin_off();
            break;

        case PMBUS_COMMAND_INTERLEAVE:
            value = pmbus_app_get_interleave();
            break;

        case PMBUS_COMMAND_IOUT_CAL_GAIN:
            value = pmbus_app_get_iout_cal_gain();
            break;

        case PMBUS_COMMAND_IOUT_CAL_OFFSET:
            value = pmbus_app_get_iout_cal_offset();
            break;

        case PMBUS_COMMAND_POWER_GOOD_ON:
            value = pmbus_app_get_power_good_on();
            break;

        case PMBUS_COMMAND_POWER_GOOD_OFF:
            value = pmbus_app_get_power_good_off();
            break;

        case PMBUS_COMMAND_TON_DELAY:
            value = pmbus_app_get_ton_delay();
            break;

        case PMBUS_COMMAND_TON_RISE:
            value = pmbus_app_get_ton_rise();
            break;

        case PMBUS_COMMAND_TON_MAX_FAULT_LIMIT:
            value = pmbus_app_get_ton_max_fault_limit();
            break;

        case PMBUS_COMMAND_TOFF_DELAY:
            value = pmbus_app_get_toff_delay();
            break;

        case PMBUS_COMMAND_TOFF_FALL:
            value = pmbus_app_get_toff_fall();
            break;

        case PMBUS_COMMAND_TOFF_MAX_WARN_LIMIT:
            value = pmbus_app_get_toff_max_warn_limit();
            break;

        case PMBUS_COMMAND_POUT_OP_FAULT_LIMIT:
            value = pmbus_app_get_pout_op_fault_limit();
            break;

        case PMBUS_COMMAND_POUT_OP_WARN_LIMIT:
            value = pmbus_app_get_pout_op_warn_limit();
            break;

        case PMBUS_COMMAND_PIN_OP_WARN_LIMIT:
            value = pmbus_app_get_pin_op_warn_limit();
            break;

        case PMBUS_COMMAND_STATUS_WORD:
            value = pmbus_app_get_status_word();
            break;

        case PMBUS_COMMAND_READ_VIN:
            value = pmbus_app_get_read_vin();
            break;

        case PMBUS_COMMAND_READ_IIN:
            value = pmbus_app_get_read_iin();
            break;

        case PMBUS_COMMAND_READ_VOUT:
            value = pmbus_app_get_read_vout();
            break;

        case PMBUS_COMMAND_READ_IOUT:
            value = pmbus_app_get_read_iout();
            break;

        case PMBUS_COMMAND_READ_TEMPERATURE_1:
            value = pmbus_app_get_read_temperature_1();
            break;

        case PMBUS_COMMAND_READ_TEMPERATURE_2:
            value = pmbus_app_get_read_temperature_2();
            break;

        case PMBUS_COMMAND_READ_TEMPERATURE_3:
            value = pmbus_app_get_read_temperature_3();
            break;

        case PMBUS_COMMAND_READ_FAN_SPEED_1:
            value = pmbus_app_get_read_fan_speed_1();
            break;

        case PMBUS_COMMAND_READ_FAN_SPEED_2:
            value = pmbus_app_get_read_fan_speed_2();
            break;

        case PMBUS_COMMAND_READ_FAN_SPEED_3:
            value = 0x0000U;
            break;

        case PMBUS_COMMAND_READ_FAN_SPEED_4:
            value = 0x0000U;
            break;

        case PMBUS_COMMAND_READ_DUTY_CYCLE:
            value = 0x0000U;
            break;

        case PMBUS_COMMAND_READ_FREQUENCY:
            value = pmbus_app_get_frequency_switch();
            break;

        case PMBUS_COMMAND_READ_POUT:
            value = pmbus_app_get_read_pout();
            break;

        case PMBUS_COMMAND_READ_PIN:
            value = pmbus_app_get_read_pin();
            break;

        case PMBUS_COMMAND_MFR_FWUPLOAD_STATUS:
            value = pmbus_app_get_mfr_fwupload_status();
            break;

        case PMBUS_COMMAND_MFR_VIN_MIN:
            value = pmbus_app_get_vin_on();
            break;

        case PMBUS_COMMAND_MFR_VIN_MAX:
            value = pmbus_app_get_vin_ov_fault_limit();
            break;

        case PMBUS_COMMAND_MFR_IIN_MAX:
            value = pmbus_app_get_iin_oc_fault_limit();
            break;

        case PMBUS_COMMAND_MFR_PIN_MAX:
            value = pmbus_app_get_pin_op_warn_limit();
            break;

        case PMBUS_COMMAND_MFR_VOUT_MIN:
            value = pmbus_app_get_vout_min();
            break;

        case PMBUS_COMMAND_MFR_VOUT_MAX:
            value = pmbus_app_get_vout_max();
            break;

        case PMBUS_COMMAND_MFR_IOUT_MAX:
            value = pmbus_app_get_iout_oc_fault_limit();
            break;

        case PMBUS_COMMAND_MFR_POUT_MAX:
            value = pmbus_app_get_pout_max();
            break;

        case PMBUS_COMMAND_MFR_TAMBIENT_MAX:
            value = pmbus_app_get_ot_fault_limit();
            break;

        case PMBUS_COMMAND_MFR_TAMBIENT_MIN:
            value = pmbus_app_get_ut_fault_limit();
            break;

        case PMBUS_COMMAND_MFR_MAX_TEMP_1:
            value = pmbus_app_get_read_temperature_1();
            break;

        case PMBUS_COMMAND_MFR_MAX_TEMP_2:
            value = pmbus_app_get_read_temperature_2();
            break;

        case PMBUS_COMMAND_MFR_MAX_TEMP_3:
            value = pmbus_app_get_read_temperature_3();
            break;

        default:
            return 0U;
    }

    pmbus_dispatch_store_word(tx_buffer, value);
    *tx_length = 2U;
    return 1U;
}

static uint8_t pmbus_dispatch_build_dword_response(uint8_t command, uint8_t *tx_buffer, uint8_t *tx_length)
{
    uint32_t value;

    value = 0UL;

    switch (command)
    {
        case PMBUS_COMMAND_READ_KWH_IN:
            value = 0UL;
            break;

        case PMBUS_COMMAND_READ_KWH_OUT:
            value = 0UL;
            break;

        default:
            return 0U;
    }

    pmbus_dispatch_store_dword(tx_buffer, value);
    *tx_length = 4U;
    return 1U;
}

static uint8_t pmbus_dispatch_build_block_response(uint8_t command, uint8_t *tx_buffer, uint8_t *tx_length)
{
    uint8_t *data_ptr;
    uint8_t length;
    uint8_t index;

    data_ptr = (uint8_t *)0;
    length = 0U;

    switch (command)
    {
        case PMBUS_COMMAND_READ_EIN:
            length = pmbus_app_get_read_ein(&data_ptr);
            break;

        case PMBUS_COMMAND_READ_EOUT:
            length = pmbus_app_get_read_eout(&data_ptr);
            break;

        case PMBUS_COMMAND_MFR_ID:
            length = pmbus_app_get_mfr_id(&data_ptr);
            break;

        case PMBUS_COMMAND_MFR_MODEL:
            length = pmbus_app_get_mfr_model(&data_ptr);
            break;

        case PMBUS_COMMAND_MFR_REVISION:
            length = pmbus_app_get_mfr_revision(&data_ptr);
            break;

        case PMBUS_COMMAND_MFR_LOCATION:
            data_ptr = g_pmbus_dispatch_mfr_location;
            length = (uint8_t)(sizeof(g_pmbus_dispatch_mfr_location) - 1U);
            break;

        case PMBUS_COMMAND_MFR_DATE:
            data_ptr = g_pmbus_dispatch_mfr_date;
            length = (uint8_t)(sizeof(g_pmbus_dispatch_mfr_date) - 1U);
            break;

        case PMBUS_COMMAND_MFR_SERIAL:
            length = pmbus_app_get_mfr_serial(&data_ptr);
            break;

        case PMBUS_COMMAND_APP_PROFILE_SUPPORT:
            data_ptr = g_pmbus_dispatch_app_profile;
            length = (uint8_t)sizeof(g_pmbus_dispatch_app_profile);
            break;

        case PMBUS_COMMAND_MFR_EFFICIENCY_LL:
            data_ptr = g_pmbus_dispatch_efficiency_ll;
            length = (uint8_t)sizeof(g_pmbus_dispatch_efficiency_ll);
            break;

        case PMBUS_COMMAND_MFR_EFFICIENCY_HL:
            data_ptr = g_pmbus_dispatch_efficiency_hl;
            length = (uint8_t)sizeof(g_pmbus_dispatch_efficiency_hl);
            break;

        case PMBUS_COMMAND_IC_DEVICE_ID:
            data_ptr = g_pmbus_dispatch_ic_device_id;
            length = (uint8_t)(sizeof(g_pmbus_dispatch_ic_device_id) - 1U);
            break;

        case PMBUS_COMMAND_IC_DEVICE_REV:
            data_ptr = g_pmbus_dispatch_ic_device_rev;
            length = (uint8_t)(sizeof(g_pmbus_dispatch_ic_device_rev) - 1U);
            break;

        case PMBUS_COMMAND_MFR_BLACKBOX:
            length = pmbus_app_get_mfr_blackbox(&data_ptr);
            break;

        default:
            if (pmbus_dispatch_build_policy_block_response(command, tx_buffer, tx_length) != 0U)
            {
                return 1U;
            }
            return 0U;
    }

    if (length > PMBUS_MAX_BLOCK_SIZE)
    {
        length = PMBUS_MAX_BLOCK_SIZE;
    }

    tx_buffer[0] = length;

    for (index = 0U; index < length; index++)
    {
        tx_buffer[index + 1U] = data_ptr[index];
    }

    *tx_length = (uint8_t)(length + 1U);
    return 1U;
}

pmbus_dispatch_protocol_t pmbus_dispatch_detect_protocol(uint8_t command, uint8_t data_len, uint8_t *payload, uint8_t repeated_start)
{
    pmbus_dispatch_response_kind_t read_kind;

    read_kind = pmbus_dispatch_get_read_kind(command);

    if (repeated_start != 0U)
    {
        if ((data_len >= 1U) &&
            (payload != 0) &&
            (pmbus_dispatch_is_extended_selector(command) != 0U))
        {
            return PMBUS_PROTOCOL_BLOCK_WRITE_READ_PROCESS_CALL;
        }

        if ((data_len >= 1U) &&
            (payload != 0) &&
            (pmbus_dispatch_is_block_write_read_process_call_supported(command) != 0U) &&
            (payload[0] <= PMBUS_MAX_BLOCK_SIZE) &&
            ((uint8_t)(payload[0] + 1U) == data_len))
        {
            return PMBUS_PROTOCOL_BLOCK_WRITE_READ_PROCESS_CALL;
        }

        if ((command == PMBUS_COMMAND_VOUT_COMMAND) && (data_len == 2U))
        {
            return PMBUS_PROTOCOL_PROCESS_CALL;
        }

        if (data_len == 0U)
        {
            switch (read_kind)
            {
                case PMBUS_RESP_BYTE:
                    return PMBUS_PROTOCOL_READ_BYTE;

                case PMBUS_RESP_WORD:
                    return PMBUS_PROTOCOL_READ_WORD;

                case PMBUS_RESP_DWORD:
                    return PMBUS_PROTOCOL_READ_DWORD;

                case PMBUS_RESP_BLOCK:
                    return PMBUS_PROTOCOL_BLOCK_READ;

                default:
                    return PMBUS_PROTOCOL_UNKNOWN;
            }
        }

        return PMBUS_PROTOCOL_UNKNOWN;
    }

    if ((data_len == 0U) && (pmbus_dispatch_is_send_byte_supported(command) != 0U))
    {
        return PMBUS_PROTOCOL_SEND_BYTE;
    }

    if ((data_len == 1U) && (pmbus_dispatch_is_write_byte_supported(command) != 0U))
    {
        return PMBUS_PROTOCOL_WRITE_BYTE;
    }

    if ((data_len == 2U) && (pmbus_dispatch_is_write_word_supported(command) != 0U))
    {
        return PMBUS_PROTOCOL_WRITE_WORD;
    }

    if ((data_len >= 1U) && (payload != 0))
    {
        if ((pmbus_dispatch_is_extended_selector(command) != 0U) &&
            (data_len >= 2U) &&
            (payload[1] <= PMBUS_MAX_BLOCK_SIZE) &&
            ((uint8_t)(payload[1] + 2U) == data_len))
        {
            return PMBUS_PROTOCOL_BLOCK_WRITE;
        }

        if ((pmbus_dispatch_is_block_write_supported(command) != 0U) &&
            (payload[0] <= PMBUS_MAX_BLOCK_SIZE) &&
            ((uint8_t)(payload[0] + 1U) == data_len))
        {
            return PMBUS_PROTOCOL_BLOCK_WRITE;
        }
    }

    return PMBUS_PROTOCOL_UNKNOWN;
}

void pmbus_dispatch_prepare_error_response(uint8_t command, uint8_t *tx_buffer, uint8_t *tx_length)
{
    pmbus_dispatch_response_kind_t read_kind;

    if (pmbus_dispatch_is_block_write_read_process_call_supported(command) != 0U)
    {
        tx_buffer[0] = 0x00U;
        *tx_length = 1U;
        return;
    }

    read_kind = pmbus_dispatch_get_read_kind(command);

    switch (read_kind)
    {
        case PMBUS_RESP_BYTE:
            tx_buffer[0] = 0x00U;
            *tx_length = 1U;
            break;

        case PMBUS_RESP_WORD:
            tx_buffer[0] = 0x00U;
            tx_buffer[1] = 0x00U;
            *tx_length = 2U;
            break;

        case PMBUS_RESP_DWORD:
            tx_buffer[0] = 0x00U;
            tx_buffer[1] = 0x00U;
            tx_buffer[2] = 0x00U;
            tx_buffer[3] = 0x00U;
            *tx_length = 4U;
            break;

        case PMBUS_RESP_BLOCK:
            tx_buffer[0] = 0x00U;
            *tx_length = 1U;
            break;

        default:
            tx_buffer[0] = 0x00U;
            *tx_length = 1U;
            break;
    }
}

uint8_t pmbus_dispatch_execute(pmbus_dispatch_transaction_t *transaction, uint8_t *tx_buffer, uint8_t *tx_length)
{
    pmbus_dispatch_transaction_t nested_transaction;
    uint8_t nested_tx_buffer[PMBUS_MAX_BLOCK_SIZE + 1U];
    uint8_t nested_tx_length;
    uint16_t word_value;
    uint8_t cml_mask;
    uint8_t saved_page;
    uint8_t target_command;
    uint8_t target_length;
    uint8_t index;

    *tx_length = 0U;

    switch (transaction->protocol)
    {
        case PMBUS_PROTOCOL_SEND_BYTE:
            if (transaction->command == PMBUS_COMMAND_CLEAR_FAULTS)
            {
                pmbus_app_clear_faults();
                return 1U;
            }

            if ((transaction->command == PMBUS_COMMAND_STORE_DEFAULT_ALL) ||
                (transaction->command == PMBUS_COMMAND_RESTORE_DEFAULT_ALL) ||
                (transaction->command == PMBUS_COMMAND_STORE_USER_ALL) ||
                (transaction->command == PMBUS_COMMAND_RESTORE_USER_ALL))
            {
                if (pmbus_dispatch_is_write_locked() != 0U)
                {
                    break;
                }

                pmbus_app_record_store_restore(transaction->command, 0U);
                return 1U;
            }
            break;

        case PMBUS_PROTOCOL_WRITE_BYTE:
            if ((transaction->command != PMBUS_COMMAND_WRITE_PROTECT) &&
                (pmbus_dispatch_is_write_locked() != 0U))
            {
                break;
            }

            switch (transaction->command)
            {
                case PMBUS_COMMAND_PAGE:
                    pmbus_app_set_page(transaction->payload[0]);
                    return 1U;

                case PMBUS_COMMAND_OPERATION:
                    pmbus_app_set_operation(transaction->payload[0]);
                    return 1U;

                case PMBUS_COMMAND_ON_OFF_CONFIG:
                    pmbus_app_set_on_off_config(transaction->payload[0]);
                    return 1U;

                case PMBUS_COMMAND_PHASE:
                    pmbus_app_set_phase(transaction->payload[0]);
                    return 1U;

                case PMBUS_COMMAND_STORE_DEFAULT_CODE:
                case PMBUS_COMMAND_RESTORE_DEFAULT_CODE:
                case PMBUS_COMMAND_STORE_USER_CODE:
                case PMBUS_COMMAND_RESTORE_USER_CODE:
                    pmbus_app_record_store_restore(transaction->command, transaction->payload[0]);
                    return 1U;

                case PMBUS_COMMAND_WRITE_PROTECT:
                    pmbus_app_set_write_protect(transaction->payload[0]);
                    return 1U;

                case PMBUS_COMMAND_FAN_CONFIG_1_2:
                    pmbus_app_set_fan_config_1_2(transaction->payload[0]);
                    return 1U;

                case PMBUS_COMMAND_FAN_CONFIG_3_4:
                    pmbus_app_set_fan_config_3_4(transaction->payload[0]);
                    return 1U;

                case PMBUS_COMMAND_VOUT_MODE:
                    pmbus_app_set_vout_mode(transaction->payload[0]);
                    return 1U;

                case PMBUS_COMMAND_POWER_MODE:
                    pmbus_app_set_power_mode(transaction->payload[0]);
                    return 1U;

                case PMBUS_COMMAND_VOUT_OV_FAULT_RESPONSE:
                    pmbus_app_set_vout_ov_fault_response(transaction->payload[0]);
                    return 1U;

                case PMBUS_COMMAND_VOUT_UV_FAULT_RESPONSE:
                    pmbus_app_set_vout_uv_fault_response(transaction->payload[0]);
                    return 1U;

                case PMBUS_COMMAND_IOUT_OC_FAULT_RESPONSE:
                    pmbus_app_set_iout_oc_fault_response(transaction->payload[0]);
                    return 1U;

                case PMBUS_COMMAND_IOUT_OC_LV_FAULT_RESPONSE:
                    pmbus_app_set_iout_oc_lv_fault_response(transaction->payload[0]);
                    return 1U;

                case PMBUS_COMMAND_IOUT_UC_FAULT_RESPONSE:
                    pmbus_app_set_iout_uc_fault_response(transaction->payload[0]);
                    return 1U;

                case PMBUS_COMMAND_OT_FAULT_RESPONSE:
                    pmbus_app_set_ot_fault_response(transaction->payload[0]);
                    return 1U;

                case PMBUS_COMMAND_UT_FAULT_RESPONSE:
                    pmbus_app_set_ut_fault_response(transaction->payload[0]);
                    return 1U;

                case PMBUS_COMMAND_VIN_OV_FAULT_RESPONSE:
                    pmbus_app_set_vin_ov_fault_response(transaction->payload[0]);
                    return 1U;

                case PMBUS_COMMAND_VIN_UV_FAULT_RESPONSE:
                    pmbus_app_set_vin_uv_fault_response(transaction->payload[0]);
                    return 1U;

                case PMBUS_COMMAND_IIN_OC_FAULT_RESPONSE:
                    pmbus_app_set_iin_oc_fault_response(transaction->payload[0]);
                    return 1U;

                case PMBUS_COMMAND_TON_MAX_FAULT_RESPONSE:
                    pmbus_app_set_ton_max_fault_response(transaction->payload[0]);
                    return 1U;

                case PMBUS_COMMAND_POUT_OP_FAULT_RESPONSE:
                    pmbus_app_set_pout_op_fault_response(transaction->payload[0]);
                    return 1U;

                case PMBUS_COMMAND_MFR_COLD_REDUNDANCY_CONFIG:
                    pmbus_app_set_mfr_cold_redundancy_config(transaction->payload[0]);
                    return 1U;

                case PMBUS_COMMAND_MFR_FWUPLOAD_MODE:
                    pmbus_app_set_mfr_fwupload_mode(transaction->payload[0]);
                    return 1U;

                default:
                    break;
            }
            break;

        case PMBUS_PROTOCOL_WRITE_WORD:
            if (pmbus_dispatch_is_write_locked() != 0U)
            {
                break;
            }

            word_value = (uint16_t)(((uint16_t)transaction->payload[1] << 8) | transaction->payload[0]);

            switch (transaction->command)
            {
                case PMBUS_COMMAND_ZONE_CONFIG:
                    pmbus_app_set_zone_config(word_value);
                    return 1U;

                case PMBUS_COMMAND_ZONE_ACTIVE:
                    pmbus_app_set_zone_active(word_value);
                    return 1U;

                case PMBUS_COMMAND_VOUT_OV_FAULT_LIMIT:
                    pmbus_app_set_vout_ov_fault_limit(word_value);
                    return 1U;

                case PMBUS_COMMAND_VOUT_OV_WARN_LIMIT:
                    pmbus_app_set_vout_ov_warn_limit(word_value);
                    return 1U;

                case PMBUS_COMMAND_VOUT_UV_WARN_LIMIT:
                    pmbus_app_set_vout_uv_warn_limit(word_value);
                    return 1U;

                case PMBUS_COMMAND_VOUT_UV_FAULT_LIMIT:
                    pmbus_app_set_vout_uv_fault_limit(word_value);
                    return 1U;

                case PMBUS_COMMAND_IOUT_OC_FAULT_LIMIT:
                    pmbus_app_set_iout_oc_fault_limit(word_value);
                    return 1U;

                case PMBUS_COMMAND_IOUT_OC_LV_FAULT_LIMIT:
                    pmbus_app_set_iout_oc_lv_fault_limit(word_value);
                    return 1U;

                case PMBUS_COMMAND_IOUT_OC_WARN_LIMIT:
                    pmbus_app_set_iout_oc_warn_limit(word_value);
                    return 1U;

                case PMBUS_COMMAND_IOUT_UC_FAULT_LIMIT:
                    pmbus_app_set_iout_uc_fault_limit(word_value);
                    return 1U;

                case PMBUS_COMMAND_OT_FAULT_LIMIT:
                    pmbus_app_set_ot_fault_limit(word_value);
                    return 1U;

                case PMBUS_COMMAND_OT_WARN_LIMIT:
                    pmbus_app_set_ot_warn_limit(word_value);
                    return 1U;

                case PMBUS_COMMAND_UT_WARN_LIMIT:
                    pmbus_app_set_ut_warn_limit(word_value);
                    return 1U;

                case PMBUS_COMMAND_UT_FAULT_LIMIT:
                    pmbus_app_set_ut_fault_limit(word_value);
                    return 1U;

                case PMBUS_COMMAND_VIN_OV_FAULT_LIMIT:
                    pmbus_app_set_vin_ov_fault_limit(word_value);
                    return 1U;

                case PMBUS_COMMAND_VIN_OV_WARN_LIMIT:
                    pmbus_app_set_vin_ov_warn_limit(word_value);
                    return 1U;

                case PMBUS_COMMAND_VIN_UV_WARN_LIMIT:
                    pmbus_app_set_vin_uv_warn_limit(word_value);
                    return 1U;

                case PMBUS_COMMAND_VIN_UV_FAULT_LIMIT:
                    pmbus_app_set_vin_uv_fault_limit(word_value);
                    return 1U;

                case PMBUS_COMMAND_IIN_OC_FAULT_LIMIT:
                    pmbus_app_set_iin_oc_fault_limit(word_value);
                    return 1U;

                case PMBUS_COMMAND_IIN_OC_WARN_LIMIT:
                    pmbus_app_set_iin_oc_warn_limit(word_value);
                    return 1U;

                case PMBUS_COMMAND_FAN_COMMAND_1:
                    pmbus_app_set_fan_command_1(word_value);
                    return 1U;

                case PMBUS_COMMAND_FAN_COMMAND_2:
                    pmbus_app_set_fan_command_2(word_value);
                    return 1U;

                case PMBUS_COMMAND_FAN_COMMAND_3:
                    pmbus_app_set_fan_command_3(word_value);
                    return 1U;

                case PMBUS_COMMAND_FAN_COMMAND_4:
                    pmbus_app_set_fan_command_4(word_value);
                    return 1U;

                case PMBUS_COMMAND_VOUT_COMMAND:
                    pmbus_app_set_vout_command(word_value);
                    return 1U;

                case PMBUS_COMMAND_VOUT_TRIM:
                    pmbus_app_set_vout_trim(word_value);
                    return 1U;

                case PMBUS_COMMAND_VOUT_CAL_OFFSET:
                    pmbus_app_set_vout_cal_offset(word_value);
                    return 1U;

                case PMBUS_COMMAND_VOUT_MAX:
                    pmbus_app_set_vout_max(word_value);
                    return 1U;

                case PMBUS_COMMAND_VOUT_MARGIN_HIGH:
                    pmbus_app_set_vout_margin_high(word_value);
                    return 1U;

                case PMBUS_COMMAND_VOUT_MARGIN_LOW:
                    pmbus_app_set_vout_margin_low(word_value);
                    return 1U;

                case PMBUS_COMMAND_VOUT_TRANSITION_RATE:
                    pmbus_app_set_vout_transition_rate(word_value);
                    return 1U;

                case PMBUS_COMMAND_VOUT_DROOP:
                    pmbus_app_set_vout_droop(word_value);
                    return 1U;

                case PMBUS_COMMAND_VOUT_SCALE_LOOP:
                    pmbus_app_set_vout_scale_loop(word_value);
                    return 1U;

                case PMBUS_COMMAND_VOUT_SCALE_MONITOR:
                    pmbus_app_set_vout_scale_monitor(word_value);
                    return 1U;

                case PMBUS_COMMAND_VOUT_MIN:
                    pmbus_app_set_vout_min(word_value);
                    return 1U;

                case PMBUS_COMMAND_POUT_MAX:
                    pmbus_app_set_pout_max(word_value);
                    return 1U;

                case PMBUS_COMMAND_MAX_DUTY:
                    pmbus_app_set_max_duty(word_value);
                    return 1U;

                case PMBUS_COMMAND_FREQUENCY_SWITCH:
                    pmbus_app_set_frequency_switch(word_value);
                    return 1U;

                case PMBUS_COMMAND_VIN_ON:
                    pmbus_app_set_vin_on(word_value);
                    return 1U;

                case PMBUS_COMMAND_VIN_OFF:
                    pmbus_app_set_vin_off(word_value);
                    return 1U;

                case PMBUS_COMMAND_INTERLEAVE:
                    pmbus_app_set_interleave(word_value);
                    return 1U;

                case PMBUS_COMMAND_IOUT_CAL_GAIN:
                    pmbus_app_set_iout_cal_gain(word_value);
                    return 1U;

                case PMBUS_COMMAND_IOUT_CAL_OFFSET:
                    pmbus_app_set_iout_cal_offset(word_value);
                    return 1U;

                case PMBUS_COMMAND_POWER_GOOD_ON:
                    pmbus_app_set_power_good_on(word_value);
                    return 1U;

                case PMBUS_COMMAND_POWER_GOOD_OFF:
                    pmbus_app_set_power_good_off(word_value);
                    return 1U;

                case PMBUS_COMMAND_TON_DELAY:
                    pmbus_app_set_ton_delay(word_value);
                    return 1U;

                case PMBUS_COMMAND_TON_RISE:
                    pmbus_app_set_ton_rise(word_value);
                    return 1U;

                case PMBUS_COMMAND_TON_MAX_FAULT_LIMIT:
                    pmbus_app_set_ton_max_fault_limit(word_value);
                    return 1U;

                case PMBUS_COMMAND_TOFF_DELAY:
                    pmbus_app_set_toff_delay(word_value);
                    return 1U;

                case PMBUS_COMMAND_TOFF_FALL:
                    pmbus_app_set_toff_fall(word_value);
                    return 1U;

                case PMBUS_COMMAND_TOFF_MAX_WARN_LIMIT:
                    pmbus_app_set_toff_max_warn_limit(word_value);
                    return 1U;

                case PMBUS_COMMAND_POUT_OP_FAULT_LIMIT:
                    pmbus_app_set_pout_op_fault_limit(word_value);
                    return 1U;

                case PMBUS_COMMAND_POUT_OP_WARN_LIMIT:
                    pmbus_app_set_pout_op_warn_limit(word_value);
                    return 1U;

                case PMBUS_COMMAND_PIN_OP_WARN_LIMIT:
                    pmbus_app_set_pin_op_warn_limit(word_value);
                    return 1U;

                case PMBUS_COMMAND_SMBALERT_MASK:
                    pmbus_app_set_smbalert_mask(word_value);
                    return 1U;

                default:
                    break;
            }
            break;

        case PMBUS_PROTOCOL_READ_BYTE:
            if (pmbus_dispatch_build_byte_response(transaction->command, tx_buffer, tx_length) != 0U)
            {
                return 1U;
            }
            break;

        case PMBUS_PROTOCOL_READ_WORD:
            if (pmbus_dispatch_build_word_response(transaction->command, tx_buffer, tx_length) != 0U)
            {
                return 1U;
            }
            break;

        case PMBUS_PROTOCOL_READ_DWORD:
            if (pmbus_dispatch_build_dword_response(transaction->command, tx_buffer, tx_length) != 0U)
            {
                return 1U;
            }
            break;

        case PMBUS_PROTOCOL_BLOCK_READ:
            if (pmbus_dispatch_build_block_response(transaction->command, tx_buffer, tx_length) != 0U)
            {
                return 1U;
            }
            break;

        case PMBUS_PROTOCOL_PROCESS_CALL:
            if (transaction->command == PMBUS_COMMAND_VOUT_COMMAND)
            {
                if (pmbus_dispatch_is_write_locked() != 0U)
                {
                    break;
                }

                word_value = (uint16_t)(((uint16_t)transaction->payload[1] << 8) | transaction->payload[0]);
                pmbus_app_set_vout_command(word_value);
                pmbus_dispatch_store_word(tx_buffer, pmbus_app_get_vout_command());
                *tx_length = 2U;
                return 1U;
            }
            break;

        case PMBUS_PROTOCOL_BLOCK_WRITE:
            if (pmbus_dispatch_is_write_locked() != 0U)
            {
                break;
            }

            if (pmbus_dispatch_is_policy_block_command(transaction->command) != 0U)
            {
                if (pmbus_dispatch_store_policy_block(transaction->command,
                    transaction->payload,
                    transaction->data_len) != 0U)
                {
                    return 1U;
                }
            }
            else if (pmbus_dispatch_is_extended_selector(transaction->command) != 0U)
            {
                if (pmbus_dispatch_store_extended_policy_block(transaction->command,
                    transaction->payload,
                    transaction->data_len) != 0U)
                {
                    return 1U;
                }
            }
            else if (transaction->command == PMBUS_COMMAND_PAGE_PLUS_WRITE)
            {
                if ((transaction->data_len >= 3U) &&
                    (transaction->payload[0] >= 2U) &&
                    ((uint8_t)(transaction->payload[0] + 1U) == transaction->data_len))
                {
                    saved_page = pmbus_app_get_page();
                    target_command = transaction->payload[2];
                    target_length = (uint8_t)(transaction->payload[0] - 2U);

                    if ((target_command != PMBUS_COMMAND_PAGE_PLUS_WRITE) &&
                        (target_command != PMBUS_COMMAND_PAGE_PLUS_READ))
                    {
                        pmbus_app_record_page_plus_write(transaction->payload[1],
                            target_command,
                            &transaction->payload[3],
                            target_length);

                        nested_transaction.command = target_command;
                        nested_transaction.data_len = target_length;
                        nested_transaction.repeated_start = 0U;
                        nested_transaction.pec_present = transaction->pec_present;
                        nested_transaction.pec_valid = transaction->pec_valid;

                        for (index = 0U; index < target_length; index++)
                        {
                            nested_transaction.payload[index] = transaction->payload[(uint8_t)(index + 3U)];
                        }

                        nested_transaction.protocol = pmbus_dispatch_detect_protocol(target_command,
                            target_length,
                            nested_transaction.payload,
                            0U);

                        pmbus_app_set_page(transaction->payload[1]);

                        if (pmbus_dispatch_execute(&nested_transaction,
                            nested_tx_buffer,
                            &nested_tx_length) != 0U)
                        {
                            pmbus_app_set_page(saved_page);
                            return 1U;
                        }

                        pmbus_app_set_page(saved_page);
                    }
                }
            }
            else if (transaction->command == PMBUS_COMMAND_MFR_FWUPLOAD)
            {
                if (transaction->data_len >= 1U)
                {
                    if (pmbus_app_store_mfr_fwupload_block(&transaction->payload[1], transaction->payload[0]) != 0U)
                    {
                        return 1U;
                    }
                }
            }
            break;

        case PMBUS_PROTOCOL_BLOCK_WRITE_READ_PROCESS_CALL:
            if (pmbus_dispatch_is_extended_selector(transaction->command) != 0U)
            {
                if (pmbus_dispatch_build_extended_policy_block_response(transaction->command,
                    transaction->payload,
                    transaction->data_len,
                    tx_buffer,
                    tx_length) != 0U)
                {
                    return 1U;
                }
            }
            else if (transaction->command == PMBUS_COMMAND_QUERY)
            {
                if (pmbus_dispatch_build_query_response(transaction->payload,
                    transaction->data_len,
                    tx_buffer,
                    tx_length) != 0U)
                {
                    return 1U;
                }
            }
            else if (transaction->command == PMBUS_COMMAND_COEFFICIENTS)
            {
                if (pmbus_dispatch_build_coefficients_response(transaction->payload,
                    transaction->data_len,
                    tx_buffer,
                    tx_length) != 0U)
                {
                    return 1U;
                }
            }
            else if (transaction->command == PMBUS_COMMAND_SMBALERT_MASK)
            {
                if (pmbus_dispatch_build_smbalert_mask_response(transaction->payload,
                    transaction->data_len,
                    tx_buffer,
                    tx_length) != 0U)
                {
                    return 1U;
                }
            }
            else if (transaction->command == PMBUS_COMMAND_PAGE_PLUS_READ)
            {
                if (pmbus_dispatch_build_page_plus_read_response(transaction->payload,
                    transaction->data_len,
                    tx_buffer,
                    tx_length) != 0U)
                {
                    return 1U;
                }
            }
            break;

        default:
            break;
    }

    if (pmbus_dispatch_is_known_command(transaction->command) != 0U)
    {
        cml_mask = PMBUS_STATUS_CML_INVALID_OR_UNSUPPORTED_DATA_RECEIVED;
    }
    else
    {
        cml_mask = PMBUS_STATUS_CML_INVALID_OR_UNSUPPORTED_COMMAND_RECEIVED;
    }

    pmbus_app_set_status_cml(cml_mask);

    if (transaction->repeated_start != 0U)
    {
        pmbus_dispatch_prepare_error_response(transaction->command, tx_buffer, tx_length);
    }

    return 0U;
}
