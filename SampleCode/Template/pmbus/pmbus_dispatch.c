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

#define PMBUS_CMD_FLAG_SEND_BYTE              0x01U
#define PMBUS_CMD_FLAG_WRITE_BYTE             0x02U
#define PMBUS_CMD_FLAG_WRITE_WORD             0x04U
#define PMBUS_CMD_FLAG_BLOCK_WRITE            0x08U
#define PMBUS_CMD_FLAG_BLOCK_WRITE_READ       0x10U
#define PMBUS_CMD_FLAG_PROCESS_CALL           0x20U

#define PMBUS_DESC(cmd, kind, caps, fmt)      { (cmd), (kind), (caps), (fmt) }

typedef struct
{
    uint8_t command;
    uint8_t read_kind;
    uint8_t flags;
    uint8_t query_data_format;
} pmbus_command_descriptor_t;

static uint8_t pmbus_dispatch_is_write_byte_supported(uint8_t command);
static uint8_t pmbus_dispatch_is_write_word_supported(uint8_t command);
static uint8_t pmbus_dispatch_is_send_byte_supported(uint8_t command);
static uint8_t pmbus_dispatch_is_block_write_supported(uint8_t command);
static uint8_t pmbus_dispatch_is_block_write_read_process_call_supported(uint8_t command);
static const pmbus_command_descriptor_t *pmbus_dispatch_find_command_descriptor(uint8_t command);
static uint8_t pmbus_dispatch_command_has_flag(uint8_t command, uint8_t flag);
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
#if PMBUS_ENABLE_CMD_ENERGY
static uint8_t pmbus_dispatch_build_dword_response(uint8_t command, uint8_t *tx_buffer, uint8_t *tx_length);
#endif
static uint8_t pmbus_dispatch_build_block_response(uint8_t command, uint8_t *tx_buffer, uint8_t *tx_length);

/* P2 Table 31 read-only placeholders.
   TODO: Bind these to SKU EEPROM, production data, and real CRPS platform measurements. */
#if PMBUS_ENABLE_CMD_MFR_EXT
static uint8_t g_pmbus_dispatch_mfr_location[] = "MFR_LOCATION_001";
static uint8_t g_pmbus_dispatch_mfr_date[] = "2026-06-14";
static uint8_t g_pmbus_dispatch_app_profile[] = { 0x01U, 0x13U };
static uint8_t g_pmbus_dispatch_efficiency_ll[] = { 0x14U, 0x5AU, 0x32U, 0x5CU, 0x64U, 0x5DU };
static uint8_t g_pmbus_dispatch_efficiency_hl[] = { 0x14U, 0x5CU, 0x32U, 0x5EU, 0x64U, 0x60U };
static uint8_t g_pmbus_dispatch_ic_device_id[] = "M031_PMBUS";
static uint8_t g_pmbus_dispatch_ic_device_rev[] = "REV_001";
#endif

#define PMBUS_DISPATCH_POLICY_BLOCK_SIZE      16U
#define PMBUS_DISPATCH_USER_DATA_COUNT        16U
#define PMBUS_DISPATCH_MFR_POLICY_COUNT       58U

/* CRPS policy namespace:
   USER_DATA and unassigned MFR_SPECIFIC commands are volatile bounded block shadows.
   TODO: bind each product-owned entry to NVM, telemetry, or control logic before final CRPS release. */
#if PMBUS_ENABLE_CMD_POLICY
static uint8_t g_pmbus_dispatch_user_policy_lengths[PMBUS_DISPATCH_USER_DATA_COUNT];
static uint8_t g_pmbus_dispatch_user_policy_data[PMBUS_DISPATCH_USER_DATA_COUNT][PMBUS_DISPATCH_POLICY_BLOCK_SIZE];
static uint8_t g_pmbus_dispatch_mfr_policy_lengths[PMBUS_DISPATCH_MFR_POLICY_COUNT];
static uint8_t g_pmbus_dispatch_mfr_policy_data[PMBUS_DISPATCH_MFR_POLICY_COUNT][PMBUS_DISPATCH_POLICY_BLOCK_SIZE];
static uint8_t g_pmbus_dispatch_extended_selector;
static uint8_t g_pmbus_dispatch_extended_command;
static uint8_t g_pmbus_dispatch_extended_length;
static uint8_t g_pmbus_dispatch_extended_data[PMBUS_DISPATCH_POLICY_BLOCK_SIZE];
#endif
static pmbus_command_descriptor_t g_pmbus_dispatch_dynamic_descriptor;

static const pmbus_command_descriptor_t g_pmbus_command_descriptors[] =
{
#if PMBUS_ENABLE_CMD_CORE
    PMBUS_DESC(PMBUS_COMMAND_PAGE, PMBUS_RESP_BYTE, PMBUS_CMD_FLAG_WRITE_BYTE, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_OPERATION, PMBUS_RESP_BYTE, PMBUS_CMD_FLAG_WRITE_BYTE, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_ON_OFF_CONFIG, PMBUS_RESP_BYTE, PMBUS_CMD_FLAG_WRITE_BYTE, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_CLEAR_FAULTS, PMBUS_RESP_NONE, PMBUS_CMD_FLAG_SEND_BYTE, 0x07U),
    PMBUS_DESC(PMBUS_COMMAND_PHASE, PMBUS_RESP_BYTE, PMBUS_CMD_FLAG_WRITE_BYTE, 0x04U),
#endif
#if PMBUS_ENABLE_CMD_PAGE_PLUS
    PMBUS_DESC(PMBUS_COMMAND_PAGE_PLUS_WRITE, PMBUS_RESP_NONE, PMBUS_CMD_FLAG_BLOCK_WRITE, 0x06U),
    PMBUS_DESC(PMBUS_COMMAND_PAGE_PLUS_READ, PMBUS_RESP_NONE, PMBUS_CMD_FLAG_BLOCK_WRITE_READ, 0x06U),
#endif
#if PMBUS_ENABLE_CMD_ZONE
    PMBUS_DESC(PMBUS_COMMAND_ZONE_CONFIG, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x07U),
    PMBUS_DESC(PMBUS_COMMAND_ZONE_ACTIVE, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x07U),
#endif
#if PMBUS_ENABLE_CMD_CORE
    PMBUS_DESC(PMBUS_COMMAND_WRITE_PROTECT, PMBUS_RESP_BYTE, PMBUS_CMD_FLAG_WRITE_BYTE, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_STORE_DEFAULT_ALL, PMBUS_RESP_NONE, PMBUS_CMD_FLAG_SEND_BYTE, 0x07U),
    PMBUS_DESC(PMBUS_COMMAND_RESTORE_DEFAULT_ALL, PMBUS_RESP_NONE, PMBUS_CMD_FLAG_SEND_BYTE, 0x07U),
    PMBUS_DESC(PMBUS_COMMAND_STORE_DEFAULT_CODE, PMBUS_RESP_NONE, PMBUS_CMD_FLAG_WRITE_BYTE, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_RESTORE_DEFAULT_CODE, PMBUS_RESP_NONE, PMBUS_CMD_FLAG_WRITE_BYTE, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_STORE_USER_ALL, PMBUS_RESP_NONE, PMBUS_CMD_FLAG_SEND_BYTE, 0x07U),
    PMBUS_DESC(PMBUS_COMMAND_RESTORE_USER_ALL, PMBUS_RESP_NONE, PMBUS_CMD_FLAG_SEND_BYTE, 0x07U),
    PMBUS_DESC(PMBUS_COMMAND_STORE_USER_CODE, PMBUS_RESP_NONE, PMBUS_CMD_FLAG_WRITE_BYTE, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_RESTORE_USER_CODE, PMBUS_RESP_NONE, PMBUS_CMD_FLAG_WRITE_BYTE, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_CAPABILITY, PMBUS_RESP_BYTE, 0U, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_QUERY, PMBUS_RESP_NONE, PMBUS_CMD_FLAG_BLOCK_WRITE_READ, 0x07U),
    PMBUS_DESC(PMBUS_COMMAND_SMBALERT_MASK, PMBUS_RESP_NONE, PMBUS_CMD_FLAG_WRITE_WORD | PMBUS_CMD_FLAG_BLOCK_WRITE_READ, 0x07U),
    PMBUS_DESC(PMBUS_COMMAND_VOUT_MODE, PMBUS_RESP_BYTE, PMBUS_CMD_FLAG_WRITE_BYTE, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_VOUT_COMMAND, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD | PMBUS_CMD_FLAG_PROCESS_CALL, 0x00U),
#endif
#if PMBUS_ENABLE_CMD_LIMITS
    PMBUS_DESC(PMBUS_COMMAND_VOUT_TRIM, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_VOUT_CAL_OFFSET, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_VOUT_MAX, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_VOUT_MARGIN_HIGH, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_VOUT_MARGIN_LOW, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_VOUT_TRANSITION_RATE, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_VOUT_DROOP, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_VOUT_SCALE_LOOP, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_VOUT_SCALE_MONITOR, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_VOUT_MIN, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
#endif
#if PMBUS_ENABLE_CMD_COEFFICIENTS
    PMBUS_DESC(PMBUS_COMMAND_COEFFICIENTS, PMBUS_RESP_NONE, PMBUS_CMD_FLAG_BLOCK_WRITE_READ, 0x06U),
#endif
#if PMBUS_ENABLE_CMD_LIMITS
    PMBUS_DESC(PMBUS_COMMAND_POUT_MAX, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_MAX_DUTY, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_FREQUENCY_SWITCH, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
#endif
#if PMBUS_ENABLE_CMD_CORE
    PMBUS_DESC(PMBUS_COMMAND_POWER_MODE, PMBUS_RESP_BYTE, PMBUS_CMD_FLAG_WRITE_BYTE, 0x04U),
#endif
#if PMBUS_ENABLE_CMD_LIMITS
    PMBUS_DESC(PMBUS_COMMAND_VIN_ON, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_VIN_OFF, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_INTERLEAVE, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x07U),
    PMBUS_DESC(PMBUS_COMMAND_IOUT_CAL_GAIN, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_IOUT_CAL_OFFSET, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
#endif
#if PMBUS_ENABLE_CMD_FAN
    PMBUS_DESC(PMBUS_COMMAND_FAN_CONFIG_1_2, PMBUS_RESP_BYTE, PMBUS_CMD_FLAG_WRITE_BYTE, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_FAN_COMMAND_1, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_FAN_COMMAND_2, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_FAN_CONFIG_3_4, PMBUS_RESP_BYTE, PMBUS_CMD_FLAG_WRITE_BYTE, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_FAN_COMMAND_3, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_FAN_COMMAND_4, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
#endif
#if PMBUS_ENABLE_CMD_LIMITS
    PMBUS_DESC(PMBUS_COMMAND_VOUT_OV_FAULT_LIMIT, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_VOUT_OV_FAULT_RESPONSE, PMBUS_RESP_BYTE, PMBUS_CMD_FLAG_WRITE_BYTE, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_VOUT_OV_WARN_LIMIT, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_VOUT_UV_WARN_LIMIT, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_VOUT_UV_FAULT_LIMIT, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_VOUT_UV_FAULT_RESPONSE, PMBUS_RESP_BYTE, PMBUS_CMD_FLAG_WRITE_BYTE, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_IOUT_OC_FAULT_LIMIT, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_IOUT_OC_FAULT_RESPONSE, PMBUS_RESP_BYTE, PMBUS_CMD_FLAG_WRITE_BYTE, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_IOUT_OC_LV_FAULT_LIMIT, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_IOUT_OC_LV_FAULT_RESPONSE, PMBUS_RESP_BYTE, PMBUS_CMD_FLAG_WRITE_BYTE, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_IOUT_OC_WARN_LIMIT, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_IOUT_UC_FAULT_LIMIT, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_IOUT_UC_FAULT_RESPONSE, PMBUS_RESP_BYTE, PMBUS_CMD_FLAG_WRITE_BYTE, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_OT_FAULT_LIMIT, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_OT_FAULT_RESPONSE, PMBUS_RESP_BYTE, PMBUS_CMD_FLAG_WRITE_BYTE, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_OT_WARN_LIMIT, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_UT_WARN_LIMIT, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_UT_FAULT_LIMIT, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_UT_FAULT_RESPONSE, PMBUS_RESP_BYTE, PMBUS_CMD_FLAG_WRITE_BYTE, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_VIN_OV_FAULT_LIMIT, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_VIN_OV_FAULT_RESPONSE, PMBUS_RESP_BYTE, PMBUS_CMD_FLAG_WRITE_BYTE, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_VIN_OV_WARN_LIMIT, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_VIN_UV_WARN_LIMIT, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_VIN_UV_FAULT_LIMIT, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_VIN_UV_FAULT_RESPONSE, PMBUS_RESP_BYTE, PMBUS_CMD_FLAG_WRITE_BYTE, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_IIN_OC_FAULT_LIMIT, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_IIN_OC_FAULT_RESPONSE, PMBUS_RESP_BYTE, PMBUS_CMD_FLAG_WRITE_BYTE, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_IIN_OC_WARN_LIMIT, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_POWER_GOOD_ON, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_POWER_GOOD_OFF, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_TON_DELAY, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_TON_RISE, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_TON_MAX_FAULT_LIMIT, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_TON_MAX_FAULT_RESPONSE, PMBUS_RESP_BYTE, PMBUS_CMD_FLAG_WRITE_BYTE, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_TOFF_DELAY, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_TOFF_FALL, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_TOFF_MAX_WARN_LIMIT, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_POUT_OP_FAULT_LIMIT, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_POUT_OP_FAULT_RESPONSE, PMBUS_RESP_BYTE, PMBUS_CMD_FLAG_WRITE_BYTE, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_POUT_OP_WARN_LIMIT, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_PIN_OP_WARN_LIMIT, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
#endif
#if PMBUS_ENABLE_CMD_STATUS
    PMBUS_DESC(PMBUS_COMMAND_STATUS_BYTE, PMBUS_RESP_BYTE, 0U, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_STATUS_WORD, PMBUS_RESP_WORD, 0U, 0x07U),
    PMBUS_DESC(PMBUS_COMMAND_STATUS_VOUT, PMBUS_RESP_BYTE, 0U, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_STATUS_IOUT, PMBUS_RESP_BYTE, 0U, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_STATUS_INPUT, PMBUS_RESP_BYTE, 0U, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_STATUS_TEMPERATURE, PMBUS_RESP_BYTE, 0U, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_STATUS_CML, PMBUS_RESP_BYTE, 0U, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_STATUS_OTHER, PMBUS_RESP_BYTE, 0U, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_STATUS_MFR_SPECIFIC, PMBUS_RESP_BYTE, 0U, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_STATUS_FANS_1_2, PMBUS_RESP_BYTE, 0U, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_STATUS_FANS_3_4, PMBUS_RESP_BYTE, 0U, 0x04U),
#endif
#if PMBUS_ENABLE_CMD_ENERGY
    PMBUS_DESC(PMBUS_COMMAND_READ_KWH_IN, PMBUS_RESP_DWORD, 0U, 0x03U),
    PMBUS_DESC(PMBUS_COMMAND_READ_KWH_OUT, PMBUS_RESP_DWORD, 0U, 0x03U),
    PMBUS_DESC(PMBUS_COMMAND_READ_KWH_CONFIG, PMBUS_RESP_WORD, 0U, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_READ_EIN, PMBUS_RESP_BLOCK, 0U, 0x06U),
    PMBUS_DESC(PMBUS_COMMAND_READ_EOUT, PMBUS_RESP_BLOCK, 0U, 0x06U),
#endif
#if PMBUS_ENABLE_CMD_TELEMETRY
    PMBUS_DESC(PMBUS_COMMAND_READ_VIN, PMBUS_RESP_WORD, 0U, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_READ_IIN, PMBUS_RESP_WORD, 0U, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_READ_VCAP, PMBUS_RESP_WORD, 0U, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_READ_VOUT, PMBUS_RESP_WORD, 0U, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_READ_IOUT, PMBUS_RESP_WORD, 0U, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_READ_TEMPERATURE_1, PMBUS_RESP_WORD, 0U, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_READ_TEMPERATURE_2, PMBUS_RESP_WORD, 0U, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_READ_TEMPERATURE_3, PMBUS_RESP_WORD, 0U, 0x00U),
#endif
#if PMBUS_ENABLE_CMD_FAN
    PMBUS_DESC(PMBUS_COMMAND_READ_FAN_SPEED_1, PMBUS_RESP_WORD, 0U, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_READ_FAN_SPEED_2, PMBUS_RESP_WORD, 0U, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_READ_FAN_SPEED_3, PMBUS_RESP_WORD, 0U, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_READ_FAN_SPEED_4, PMBUS_RESP_WORD, 0U, 0x00U),
#endif
#if PMBUS_ENABLE_CMD_TELEMETRY
    PMBUS_DESC(PMBUS_COMMAND_READ_DUTY_CYCLE, PMBUS_RESP_WORD, 0U, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_READ_FREQUENCY, PMBUS_RESP_WORD, 0U, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_READ_POUT, PMBUS_RESP_WORD, 0U, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_READ_PIN, PMBUS_RESP_WORD, 0U, 0x00U),
#endif
#if PMBUS_ENABLE_CMD_CORE
    PMBUS_DESC(PMBUS_COMMAND_PMBUS_REVISION, PMBUS_RESP_BYTE, 0U, 0x04U),
#endif
#if PMBUS_ENABLE_CMD_MFR_BASIC
    PMBUS_DESC(PMBUS_COMMAND_MFR_ID, PMBUS_RESP_BLOCK, 0U, 0x07U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_MODEL, PMBUS_RESP_BLOCK, 0U, 0x07U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_REVISION, PMBUS_RESP_BLOCK, 0U, 0x07U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_SERIAL, PMBUS_RESP_BLOCK, 0U, 0x07U),
#endif
#if PMBUS_ENABLE_CMD_MFR_EXT
    PMBUS_DESC(PMBUS_COMMAND_MFR_LOCATION, PMBUS_RESP_BLOCK, 0U, 0x07U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_DATE, PMBUS_RESP_BLOCK, 0U, 0x07U),
    PMBUS_DESC(PMBUS_COMMAND_APP_PROFILE_SUPPORT, PMBUS_RESP_BLOCK, 0U, 0x06U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_VIN_MIN, PMBUS_RESP_WORD, 0U, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_VIN_MAX, PMBUS_RESP_WORD, 0U, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_IIN_MAX, PMBUS_RESP_WORD, 0U, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_PIN_MAX, PMBUS_RESP_WORD, 0U, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_VOUT_MIN, PMBUS_RESP_WORD, 0U, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_VOUT_MAX, PMBUS_RESP_WORD, 0U, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_IOUT_MAX, PMBUS_RESP_WORD, 0U, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_POUT_MAX, PMBUS_RESP_WORD, 0U, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_TAMBIENT_MAX, PMBUS_RESP_WORD, 0U, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_TAMBIENT_MIN, PMBUS_RESP_WORD, 0U, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_EFFICIENCY_LL, PMBUS_RESP_BLOCK, 0U, 0x06U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_EFFICIENCY_HL, PMBUS_RESP_BLOCK, 0U, 0x06U),
#if PMBUS_ENABLE_CMD_CRPS
    PMBUS_DESC(PMBUS_COMMAND_MFR_EFFICIENCY_DATA, PMBUS_RESP_BLOCK, 0U, 0x06U),
#endif
    PMBUS_DESC(PMBUS_COMMAND_MFR_PIN_ACCURACY, PMBUS_RESP_BYTE, 0U, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_IC_DEVICE_ID, PMBUS_RESP_BLOCK, 0U, 0x07U),
    PMBUS_DESC(PMBUS_COMMAND_IC_DEVICE_REV, PMBUS_RESP_BLOCK, 0U, 0x07U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_MAX_TEMP_1, PMBUS_RESP_WORD, 0U, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_MAX_TEMP_2, PMBUS_RESP_WORD, 0U, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_MAX_TEMP_3, PMBUS_RESP_WORD, 0U, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_COLD_REDUNDANCY_CONFIG, PMBUS_RESP_BYTE, PMBUS_CMD_FLAG_WRITE_BYTE, 0x04U),
#if PMBUS_ENABLE_CMD_CRPS
    PMBUS_DESC(PMBUS_COMMAND_MFR_READ_CONFIG_FILE_SIZE, PMBUS_RESP_BLOCK, 0U, 0x06U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_READ_CONFIG_BLOCK_SIZE, PMBUS_RESP_WORD, 0U, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_READ_CONFIG_FILE, PMBUS_RESP_NONE, PMBUS_CMD_FLAG_BLOCK_WRITE_READ, 0x06U),
#endif
    PMBUS_DESC(PMBUS_COMMAND_MFR_HW_COMPATIBILITY, PMBUS_RESP_WORD, 0U, 0x07U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_FWUPLOAD_CAPABILITY, PMBUS_RESP_BYTE, 0U, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_FW_REVISION, PMBUS_RESP_BLOCK, 0U, 0x06U),
#if PMBUS_ENABLE_CMD_CRPS
    PMBUS_DESC(PMBUS_COMMAND_MFR_SPDM, PMBUS_RESP_NONE, PMBUS_CMD_FLAG_BLOCK_WRITE_READ, 0x06U),
#endif
    PMBUS_DESC(PMBUS_COMMAND_MFR_FRU_PROTECTION, PMBUS_RESP_BYTE, PMBUS_CMD_FLAG_WRITE_BYTE, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_REAL_TIME_BLACK_BOX, PMBUS_RESP_BLOCK, PMBUS_CMD_FLAG_BLOCK_WRITE | PMBUS_CMD_FLAG_BLOCK_WRITE_READ, 0x06U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_SYSTEM_BLACK_BOX, PMBUS_RESP_BLOCK, PMBUS_CMD_FLAG_BLOCK_WRITE | PMBUS_CMD_FLAG_BLOCK_WRITE_READ, 0x06U),
#if PMBUS_ENABLE_CMD_CRPS
    PMBUS_DESC(PMBUS_COMMAND_MFR_BLACK_BOX_CONFIG, PMBUS_RESP_BYTE, PMBUS_CMD_FLAG_WRITE_BYTE, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_CLEAR_BLACK_BOX, PMBUS_RESP_NONE, PMBUS_CMD_FLAG_WRITE_BYTE, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_LINE_STATUS, PMBUS_RESP_BYTE, PMBUS_CMD_FLAG_WRITE_BYTE, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_SYSTEM_LED_CNTL, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_FWUPLOAD_BLOCK_SIZE, PMBUS_RESP_WORD, 0U, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_EN_STATUS_SIMULATION_CMD, PMBUS_RESP_BYTE, PMBUS_CMD_FLAG_WRITE_BYTE, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_PEAK_CURRENT_RECORD, PMBUS_RESP_BLOCK, PMBUS_CMD_FLAG_BLOCK_WRITE, 0x06U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_COMPONENT_ID, PMBUS_RESP_BLOCK, 0U, 0x06U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_TOT_POUT_MAX, PMBUS_RESP_WORD, 0U, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_VOUT_MARGINING, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_OCWPL1_SETTING, PMBUS_RESP_BLOCK, PMBUS_CMD_FLAG_BLOCK_WRITE, 0x06U),
#endif
    PMBUS_DESC(PMBUS_COMMAND_MFR_PWOK_WARNING_TIME, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_MAX_IOUT_CAPABILITY, PMBUS_RESP_BLOCK, 0U, 0x06U),
#if PMBUS_ENABLE_CMD_CRPS
    PMBUS_DESC(PMBUS_COMMAND_MFR_FPC_MAIN_MIN_OFF_TIME, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_FPC_12VSB_MIN_OFF_TIME, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x00U),
#endif
#endif
#if PMBUS_ENABLE_CMD_FWUPLOAD
    PMBUS_DESC(PMBUS_COMMAND_MFR_FWUPLOAD_MODE, PMBUS_RESP_BYTE, PMBUS_CMD_FLAG_WRITE_BYTE, 0x04U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_FWUPLOAD, PMBUS_RESP_NONE, PMBUS_CMD_FLAG_BLOCK_WRITE, 0x06U),
    PMBUS_DESC(PMBUS_COMMAND_MFR_FWUPLOAD_STATUS, PMBUS_RESP_WORD, PMBUS_CMD_FLAG_WRITE_WORD, 0x07U),
#endif
#if PMBUS_ENABLE_CMD_MFR_EXT
    PMBUS_DESC(PMBUS_COMMAND_MFR_BLACKBOX, PMBUS_RESP_BLOCK, 0U, 0x06U)
#endif
};

static void pmbus_dispatch_store_word(uint8_t *buffer, uint16_t value)
{
    buffer[0] = (uint8_t)(value & 0x00FFU);
    buffer[1] = (uint8_t)((value >> 8) & 0x00FFU);
}

#if PMBUS_ENABLE_CMD_ENERGY
static void pmbus_dispatch_store_dword(uint8_t *buffer, uint32_t value)
{
    buffer[0] = (uint8_t)(value & 0x000000FFUL);
    buffer[1] = (uint8_t)((value >> 8) & 0x000000FFUL);
    buffer[2] = (uint8_t)((value >> 16) & 0x000000FFUL);
    buffer[3] = (uint8_t)((value >> 24) & 0x000000FFUL);
}
#endif

static const pmbus_command_descriptor_t *pmbus_dispatch_find_command_descriptor(uint8_t command)
{
    uint8_t index;
    uint8_t count;

    count = (uint8_t)(sizeof(g_pmbus_command_descriptors) / sizeof(g_pmbus_command_descriptors[0]));
    for (index = 0U; index < count; index++)
    {
        if (g_pmbus_command_descriptors[index].command == command)
        {
            return &g_pmbus_command_descriptors[index];
        }
    }

    if ((PMBUS_ENABLE_CMD_POLICY != 0U) &&
        (pmbus_dispatch_is_policy_block_command(command) != 0U))
    {
        g_pmbus_dispatch_dynamic_descriptor.command = command;
        g_pmbus_dispatch_dynamic_descriptor.read_kind = PMBUS_RESP_BLOCK;
        g_pmbus_dispatch_dynamic_descriptor.flags = PMBUS_CMD_FLAG_BLOCK_WRITE;
        g_pmbus_dispatch_dynamic_descriptor.query_data_format = 0x07U;
        return &g_pmbus_dispatch_dynamic_descriptor;
    }

    if ((PMBUS_ENABLE_CMD_POLICY != 0U) &&
        (pmbus_dispatch_is_extended_selector(command) != 0U))
    {
        g_pmbus_dispatch_dynamic_descriptor.command = command;
        g_pmbus_dispatch_dynamic_descriptor.read_kind = PMBUS_RESP_NONE;
        g_pmbus_dispatch_dynamic_descriptor.flags = PMBUS_CMD_FLAG_BLOCK_WRITE | PMBUS_CMD_FLAG_BLOCK_WRITE_READ;
        g_pmbus_dispatch_dynamic_descriptor.query_data_format = 0x07U;
        return &g_pmbus_dispatch_dynamic_descriptor;
    }

    return (const pmbus_command_descriptor_t *)0;
}

static uint8_t pmbus_dispatch_command_has_flag(uint8_t command, uint8_t flag)
{
    const pmbus_command_descriptor_t *descriptor;

    descriptor = pmbus_dispatch_find_command_descriptor(command);
    if (descriptor == 0)
    {
        return 0U;
    }

    if ((descriptor->flags & flag) != 0U)
    {
        return 1U;
    }

    return 0U;
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
        case PMBUS_COMMAND_MFR_READ_CONFIG_FILE_SIZE:
        case PMBUS_COMMAND_MFR_READ_CONFIG_BLOCK_SIZE:
        case PMBUS_COMMAND_MFR_READ_CONFIG_FILE:
        case PMBUS_COMMAND_MFR_HW_COMPATIBILITY:
        case PMBUS_COMMAND_MFR_FWUPLOAD_CAPABILITY:
        case PMBUS_COMMAND_MFR_FWUPLOAD_MODE:
        case PMBUS_COMMAND_MFR_FWUPLOAD:
        case PMBUS_COMMAND_MFR_FWUPLOAD_STATUS:
        case PMBUS_COMMAND_MFR_FW_REVISION:
        case PMBUS_COMMAND_MFR_SPDM:
        case PMBUS_COMMAND_MFR_FRU_PROTECTION:
        case PMBUS_COMMAND_MFR_BLACKBOX:
        case PMBUS_COMMAND_MFR_REAL_TIME_BLACK_BOX:
        case PMBUS_COMMAND_MFR_SYSTEM_BLACK_BOX:
        case PMBUS_COMMAND_MFR_BLACK_BOX_CONFIG:
        case PMBUS_COMMAND_MFR_CLEAR_BLACK_BOX:
        case PMBUS_COMMAND_MFR_LINE_STATUS:
        case PMBUS_COMMAND_MFR_SYSTEM_LED_CNTL:
        case PMBUS_COMMAND_MFR_FWUPLOAD_BLOCK_SIZE:
        case PMBUS_COMMAND_MFR_EN_STATUS_SIMULATION_CMD:
        case PMBUS_COMMAND_MFR_PEAK_CURRENT_RECORD:
        case PMBUS_COMMAND_MFR_COMPONENT_ID:
        case PMBUS_COMMAND_MFR_TOT_POUT_MAX:
        case PMBUS_COMMAND_MFR_VOUT_MARGINING:
        case PMBUS_COMMAND_MFR_OCWPL1_SETTING:
        case PMBUS_COMMAND_MFR_PWOK_WARNING_TIME:
        case PMBUS_COMMAND_MFR_MAX_IOUT_CAPABILITY:
        case PMBUS_COMMAND_MFR_FPC_MAIN_MIN_OFF_TIME:
        case PMBUS_COMMAND_MFR_FPC_12VSB_MIN_OFF_TIME:
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
#if PMBUS_ENABLE_CMD_POLICY
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
#else
    command = command;
    payload = payload;
    data_len = data_len;
    return 0U;
#endif
}

static uint8_t pmbus_dispatch_store_extended_policy_block(uint8_t command, uint8_t *payload, uint8_t data_len)
{
#if PMBUS_ENABLE_CMD_POLICY
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
#else
    command = command;
    payload = payload;
    data_len = data_len;
    return 0U;
#endif
}

static uint8_t pmbus_dispatch_build_policy_block_response(uint8_t command, uint8_t *tx_buffer, uint8_t *tx_length)
{
#if PMBUS_ENABLE_CMD_POLICY
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
#else
    command = command;
    tx_buffer = tx_buffer;
    tx_length = tx_length;
    return 0U;
#endif
}

static uint8_t pmbus_dispatch_build_extended_policy_block_response(uint8_t command, uint8_t *payload, uint8_t data_len, uint8_t *tx_buffer, uint8_t *tx_length)
{
#if PMBUS_ENABLE_CMD_POLICY
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
#else
    command = command;
    payload = payload;
    data_len = data_len;
    tx_buffer = tx_buffer;
    tx_length = tx_length;
    return 0U;
#endif
}

static pmbus_dispatch_response_kind_t pmbus_dispatch_get_read_kind(uint8_t command)
{
    const pmbus_command_descriptor_t *descriptor;

    descriptor = pmbus_dispatch_find_command_descriptor(command);
    if (descriptor == 0)
    {
        return PMBUS_RESP_NONE;
    }

    return (pmbus_dispatch_response_kind_t)descriptor->read_kind;
}

static uint8_t pmbus_dispatch_is_write_byte_supported(uint8_t command)
{
    return pmbus_dispatch_command_has_flag(command, PMBUS_CMD_FLAG_WRITE_BYTE);
}

static uint8_t pmbus_dispatch_is_write_word_supported(uint8_t command)
{
    return pmbus_dispatch_command_has_flag(command, PMBUS_CMD_FLAG_WRITE_WORD);
}

uint8_t pmbus_dispatch_is_known_command(uint8_t command)
{
    if (pmbus_dispatch_find_command_descriptor(command) != 0)
    {
        return 1U;
    }

    return 0U;
}

static uint8_t pmbus_dispatch_is_send_byte_supported(uint8_t command)
{
    return pmbus_dispatch_command_has_flag(command, PMBUS_CMD_FLAG_SEND_BYTE);
}

static uint8_t pmbus_dispatch_is_block_write_supported(uint8_t command)
{
    return pmbus_dispatch_command_has_flag(command, PMBUS_CMD_FLAG_BLOCK_WRITE);
}

static uint8_t pmbus_dispatch_is_block_write_read_process_call_supported(uint8_t command)
{
    return pmbus_dispatch_command_has_flag(command, PMBUS_CMD_FLAG_BLOCK_WRITE_READ);
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
    const pmbus_command_descriptor_t *descriptor;

    descriptor = pmbus_dispatch_find_command_descriptor(command);
    if (descriptor == 0)
    {
        return 0x07U;
    }

    return descriptor->query_data_format;
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
         (pmbus_dispatch_command_has_flag(command, PMBUS_CMD_FLAG_PROCESS_CALL) != 0U)))
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

    if (payload == 0)
    {
        return 0U;
    }

    if (data_len == 1U)
    {
        if (payload[0] != 0U)
        {
            return 0U;
        }
    }
    else if (data_len == 2U)
    {
        if ((payload[0] != 1U) || (payload[1] != 0U))
        {
            return 0U;
        }
    }
    else
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

#if PMBUS_ENABLE_CMD_ENERGY
        case PMBUS_RESP_DWORD:
            if (pmbus_dispatch_build_dword_response(target_command, &tx_buffer[1], tx_length) != 0U)
            {
                tx_buffer[0] = *tx_length;
                *tx_length = (uint8_t)(*tx_length + 1U);
                result = 1U;
            }
            break;
#endif

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
#if PMBUS_ENABLE_CMD_CORE
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
#endif

#if PMBUS_ENABLE_CMD_FAN
        case PMBUS_COMMAND_FAN_CONFIG_1_2:
            tx_buffer[0] = pmbus_app_get_fan_config_1_2();
            break;

        case PMBUS_COMMAND_FAN_CONFIG_3_4:
            tx_buffer[0] = pmbus_app_get_fan_config_3_4();
            break;
#endif

#if PMBUS_ENABLE_CMD_CORE
        case PMBUS_COMMAND_CAPABILITY:
            tx_buffer[0] = pmbus_app_get_capability();
            break;

        case PMBUS_COMMAND_VOUT_MODE:
            tx_buffer[0] = pmbus_app_get_vout_mode();
            break;

        case PMBUS_COMMAND_POWER_MODE:
            tx_buffer[0] = pmbus_app_get_power_mode();
            break;
#endif

#if PMBUS_ENABLE_CMD_LIMITS
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
#endif

#if PMBUS_ENABLE_CMD_STATUS
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
#endif

#if PMBUS_ENABLE_CMD_MFR_EXT
        case PMBUS_COMMAND_MFR_COLD_REDUNDANCY_CONFIG:
            tx_buffer[0] = pmbus_app_get_mfr_cold_redundancy_config();
            break;

        case PMBUS_COMMAND_MFR_FWUPLOAD_CAPABILITY:
            tx_buffer[0] = pmbus_app_get_mfr_fwupload_capability();
            break;

        case PMBUS_COMMAND_MFR_FRU_PROTECTION:
            tx_buffer[0] = pmbus_app_get_mfr_fru_protection();
            break;

#if PMBUS_ENABLE_CMD_CRPS
        case PMBUS_COMMAND_MFR_BLACK_BOX_CONFIG:
            tx_buffer[0] = pmbus_app_get_mfr_black_box_config();
            break;

        case PMBUS_COMMAND_MFR_LINE_STATUS:
            tx_buffer[0] = pmbus_app_get_mfr_line_status();
            break;

        case PMBUS_COMMAND_MFR_EN_STATUS_SIMULATION_CMD:
            tx_buffer[0] = pmbus_app_get_mfr_en_status_simulation_cmd();
            break;
#endif
#endif

#if PMBUS_ENABLE_CMD_FWUPLOAD
        case PMBUS_COMMAND_MFR_FWUPLOAD_MODE:
            tx_buffer[0] = pmbus_app_get_mfr_fwupload_mode();
            break;
#endif

#if PMBUS_ENABLE_CMD_CORE
        case PMBUS_COMMAND_PMBUS_REVISION:
            tx_buffer[0] = pmbus_app_get_pmbus_revision();
            break;
#endif

#if PMBUS_ENABLE_CMD_MFR_EXT
        case PMBUS_COMMAND_MFR_PIN_ACCURACY:
            tx_buffer[0] = 50U;
            break;
#endif

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
#if PMBUS_ENABLE_CMD_ZONE
        case PMBUS_COMMAND_ZONE_CONFIG:
            value = pmbus_app_get_zone_config();
            break;

        case PMBUS_COMMAND_ZONE_ACTIVE:
            value = pmbus_app_get_zone_active();
            break;
#endif

#if PMBUS_ENABLE_CMD_LIMITS
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
#endif

#if PMBUS_ENABLE_CMD_FAN
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
#endif

#if PMBUS_ENABLE_CMD_ENERGY
        case PMBUS_COMMAND_READ_KWH_CONFIG:
            value = 0x0000U;
            break;
#endif

#if PMBUS_ENABLE_CMD_TELEMETRY
        case PMBUS_COMMAND_READ_VCAP:
            value = pmbus_app_get_read_vin();
            break;
#endif

#if PMBUS_ENABLE_CMD_CORE
        case PMBUS_COMMAND_VOUT_COMMAND:
            value = pmbus_app_get_vout_command();
            break;
#endif

#if PMBUS_ENABLE_CMD_LIMITS
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
#endif

#if PMBUS_ENABLE_CMD_STATUS
        case PMBUS_COMMAND_STATUS_WORD:
            value = pmbus_app_get_status_word();
            break;
#endif

#if PMBUS_ENABLE_CMD_TELEMETRY
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
#endif

#if PMBUS_ENABLE_CMD_FAN
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
#endif

#if PMBUS_ENABLE_CMD_TELEMETRY
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
#endif

#if PMBUS_ENABLE_CMD_FWUPLOAD
        case PMBUS_COMMAND_MFR_FWUPLOAD_STATUS:
            value = pmbus_app_get_mfr_fwupload_status();
            break;
#endif

#if PMBUS_ENABLE_CMD_MFR_EXT
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

#if PMBUS_ENABLE_CMD_CRPS
        case PMBUS_COMMAND_MFR_READ_CONFIG_BLOCK_SIZE:
            value = pmbus_app_get_mfr_read_config_block_size();
            break;
#endif

        case PMBUS_COMMAND_MFR_HW_COMPATIBILITY:
            value = pmbus_app_get_mfr_hw_compatibility();
            break;

#if PMBUS_ENABLE_CMD_CRPS
        case PMBUS_COMMAND_MFR_SYSTEM_LED_CNTL:
            value = pmbus_app_get_mfr_system_led_cntl();
            break;

        case PMBUS_COMMAND_MFR_FWUPLOAD_BLOCK_SIZE:
            value = pmbus_app_get_mfr_fwupload_block_size();
            break;

        case PMBUS_COMMAND_MFR_TOT_POUT_MAX:
            value = pmbus_app_get_mfr_tot_pout_max();
            break;

        case PMBUS_COMMAND_MFR_VOUT_MARGINING:
            value = pmbus_app_get_mfr_vout_margining();
            break;
#endif

        case PMBUS_COMMAND_MFR_PWOK_WARNING_TIME:
            value = pmbus_app_get_mfr_pwok_warning_time();
            break;

#if PMBUS_ENABLE_CMD_CRPS
        case PMBUS_COMMAND_MFR_FPC_MAIN_MIN_OFF_TIME:
            value = pmbus_app_get_mfr_fpc_main_min_off_time();
            break;

        case PMBUS_COMMAND_MFR_FPC_12VSB_MIN_OFF_TIME:
            value = pmbus_app_get_mfr_fpc_12vsb_min_off_time();
            break;
#endif
#endif

        default:
            return 0U;
    }

    pmbus_dispatch_store_word(tx_buffer, value);
    *tx_length = 2U;
    return 1U;
}

#if PMBUS_ENABLE_CMD_ENERGY
static uint8_t pmbus_dispatch_build_dword_response(uint8_t command, uint8_t *tx_buffer, uint8_t *tx_length)
{
    uint32_t value;

    value = 0UL;

    switch (command)
    {
#if PMBUS_ENABLE_CMD_ENERGY
        case PMBUS_COMMAND_READ_KWH_IN:
            value = 0UL;
            break;

        case PMBUS_COMMAND_READ_KWH_OUT:
            value = 0UL;
            break;
#endif

        default:
            return 0U;
    }

    pmbus_dispatch_store_dword(tx_buffer, value);
    *tx_length = 4U;
    return 1U;
}
#endif

static uint8_t pmbus_dispatch_build_block_response(uint8_t command, uint8_t *tx_buffer, uint8_t *tx_length)
{
    uint8_t *data_ptr;
    uint8_t length;
    uint8_t index;

    data_ptr = (uint8_t *)0;
    length = 0U;

    switch (command)
    {
#if PMBUS_ENABLE_CMD_ENERGY
        case PMBUS_COMMAND_READ_EIN:
            length = pmbus_app_get_read_ein(&data_ptr);
            break;

        case PMBUS_COMMAND_READ_EOUT:
            length = pmbus_app_get_read_eout(&data_ptr);
            break;
#endif

#if PMBUS_ENABLE_CMD_MFR_BASIC
        case PMBUS_COMMAND_MFR_ID:
            length = pmbus_app_get_mfr_id(&data_ptr);
            break;

        case PMBUS_COMMAND_MFR_MODEL:
            length = pmbus_app_get_mfr_model(&data_ptr);
            break;

        case PMBUS_COMMAND_MFR_REVISION:
            length = pmbus_app_get_mfr_revision(&data_ptr);
            break;
#endif

#if PMBUS_ENABLE_CMD_MFR_EXT
        case PMBUS_COMMAND_MFR_LOCATION:
            data_ptr = g_pmbus_dispatch_mfr_location;
            length = (uint8_t)(sizeof(g_pmbus_dispatch_mfr_location) - 1U);
            break;

        case PMBUS_COMMAND_MFR_DATE:
            data_ptr = g_pmbus_dispatch_mfr_date;
            length = (uint8_t)(sizeof(g_pmbus_dispatch_mfr_date) - 1U);
            break;
#endif

#if PMBUS_ENABLE_CMD_MFR_BASIC
        case PMBUS_COMMAND_MFR_SERIAL:
            length = pmbus_app_get_mfr_serial(&data_ptr);
            break;
#endif

#if PMBUS_ENABLE_CMD_MFR_EXT
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

#if PMBUS_ENABLE_CMD_CRPS
        case PMBUS_COMMAND_MFR_EFFICIENCY_DATA:
            length = pmbus_app_get_mfr_efficiency_data(&data_ptr);
            break;
#endif

        case PMBUS_COMMAND_IC_DEVICE_ID:
            data_ptr = g_pmbus_dispatch_ic_device_id;
            length = (uint8_t)(sizeof(g_pmbus_dispatch_ic_device_id) - 1U);
            break;

        case PMBUS_COMMAND_IC_DEVICE_REV:
            data_ptr = g_pmbus_dispatch_ic_device_rev;
            length = (uint8_t)(sizeof(g_pmbus_dispatch_ic_device_rev) - 1U);
            break;

        case PMBUS_COMMAND_MFR_FW_REVISION:
            length = pmbus_app_get_mfr_fw_revision(&data_ptr);
            break;

#if PMBUS_ENABLE_CMD_CRPS
        case PMBUS_COMMAND_MFR_READ_CONFIG_FILE_SIZE:
            length = pmbus_app_get_mfr_read_config_file_size(&data_ptr);
            break;

        case PMBUS_COMMAND_MFR_READ_CONFIG_FILE:
            length = pmbus_app_get_mfr_read_config_file(&data_ptr);
            break;
#endif

        case PMBUS_COMMAND_MFR_BLACKBOX:
            length = pmbus_app_get_mfr_blackbox(&data_ptr);
            break;

#if PMBUS_ENABLE_CMD_CRPS
        case PMBUS_COMMAND_MFR_SPDM:
            length = pmbus_app_get_mfr_spdm(&data_ptr);
            break;
#endif

        case PMBUS_COMMAND_MFR_REAL_TIME_BLACK_BOX:
            length = pmbus_app_get_mfr_real_time_black_box(&data_ptr);
            break;

        case PMBUS_COMMAND_MFR_SYSTEM_BLACK_BOX:
            length = pmbus_app_get_mfr_system_black_box(&data_ptr);
            break;

#if PMBUS_ENABLE_CMD_CRPS
        case PMBUS_COMMAND_MFR_PEAK_CURRENT_RECORD:
            length = pmbus_app_get_mfr_peak_current_record(&data_ptr);
            break;

        case PMBUS_COMMAND_MFR_COMPONENT_ID:
            length = pmbus_app_get_mfr_component_id(&data_ptr);
            break;

        case PMBUS_COMMAND_MFR_OCWPL1_SETTING:
            length = pmbus_app_get_mfr_ocwpl1_setting(&data_ptr);
            break;
#endif

        case PMBUS_COMMAND_MFR_MAX_IOUT_CAPABILITY:
            length = pmbus_app_get_mfr_max_iout_capability(&data_ptr);
            break;
#endif

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

        if ((data_len == 2U) &&
            (pmbus_dispatch_command_has_flag(command, PMBUS_CMD_FLAG_PROCESS_CALL) != 0U))
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
#if PMBUS_ENABLE_CMD_PAGE_PLUS
    pmbus_dispatch_transaction_t nested_transaction;
    uint8_t nested_tx_buffer[PMBUS_MAX_BLOCK_SIZE + 1U];
    uint8_t nested_tx_length;
#endif
    uint16_t word_value;
    uint8_t cml_mask;
#if PMBUS_ENABLE_CMD_PAGE_PLUS
    uint8_t saved_page;
    uint8_t target_command;
    uint8_t target_length;
    uint8_t index;
#endif

    *tx_length = 0U;

    switch (transaction->protocol)
    {
        case PMBUS_PROTOCOL_SEND_BYTE:
#if PMBUS_ENABLE_CMD_CORE
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
#endif
            break;

        case PMBUS_PROTOCOL_WRITE_BYTE:
            if ((transaction->command != PMBUS_COMMAND_WRITE_PROTECT) &&
                (pmbus_dispatch_is_write_locked() != 0U))
            {
                break;
            }

            switch (transaction->command)
            {
#if PMBUS_ENABLE_CMD_CORE
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
#endif

#if PMBUS_ENABLE_CMD_FAN
                case PMBUS_COMMAND_FAN_CONFIG_1_2:
                    pmbus_app_set_fan_config_1_2(transaction->payload[0]);
                    return 1U;

                case PMBUS_COMMAND_FAN_CONFIG_3_4:
                    pmbus_app_set_fan_config_3_4(transaction->payload[0]);
                    return 1U;
#endif

#if PMBUS_ENABLE_CMD_CORE
                case PMBUS_COMMAND_VOUT_MODE:
                    if (pmbus_app_set_vout_mode(transaction->payload[0]) != 0U)
                    {
                        return 1U;
                    }
                    break;

                case PMBUS_COMMAND_POWER_MODE:
                    pmbus_app_set_power_mode(transaction->payload[0]);
                    return 1U;
#endif

#if PMBUS_ENABLE_CMD_LIMITS
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
#endif

#if PMBUS_ENABLE_CMD_MFR_EXT
                case PMBUS_COMMAND_MFR_COLD_REDUNDANCY_CONFIG:
                    pmbus_app_set_mfr_cold_redundancy_config(transaction->payload[0]);
                    return 1U;

                case PMBUS_COMMAND_MFR_FRU_PROTECTION:
                    pmbus_app_set_mfr_fru_protection(transaction->payload[0]);
                    return 1U;

#if PMBUS_ENABLE_CMD_CRPS
                case PMBUS_COMMAND_MFR_BLACK_BOX_CONFIG:
                    pmbus_app_set_mfr_black_box_config(transaction->payload[0]);
                    return 1U;

                case PMBUS_COMMAND_MFR_CLEAR_BLACK_BOX:
                    pmbus_app_clear_mfr_black_box(transaction->payload[0]);
                    return 1U;

                case PMBUS_COMMAND_MFR_LINE_STATUS:
                    pmbus_app_set_mfr_line_status(transaction->payload[0]);
                    return 1U;

                case PMBUS_COMMAND_MFR_EN_STATUS_SIMULATION_CMD:
                    pmbus_app_set_mfr_en_status_simulation_cmd(transaction->payload[0]);
                    return 1U;
#endif
#endif

#if PMBUS_ENABLE_CMD_FWUPLOAD
                case PMBUS_COMMAND_MFR_FWUPLOAD_MODE:
                    pmbus_app_set_mfr_fwupload_mode(transaction->payload[0]);
                    return 1U;
#endif

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
#if PMBUS_ENABLE_CMD_ZONE
                case PMBUS_COMMAND_ZONE_CONFIG:
                    pmbus_app_set_zone_config(word_value);
                    return 1U;

                case PMBUS_COMMAND_ZONE_ACTIVE:
                    pmbus_app_set_zone_active(word_value);
                    return 1U;
#endif

#if PMBUS_ENABLE_CMD_LIMITS
                case PMBUS_COMMAND_VOUT_OV_FAULT_LIMIT:
                    if (pmbus_app_set_vout_ov_fault_limit(word_value) != 0U)
                    {
                        return 1U;
                    }
                    break;

                case PMBUS_COMMAND_VOUT_OV_WARN_LIMIT:
                    if (pmbus_app_set_vout_ov_warn_limit(word_value) != 0U)
                    {
                        return 1U;
                    }
                    break;

                case PMBUS_COMMAND_VOUT_UV_WARN_LIMIT:
                    if (pmbus_app_set_vout_uv_warn_limit(word_value) != 0U)
                    {
                        return 1U;
                    }
                    break;

                case PMBUS_COMMAND_VOUT_UV_FAULT_LIMIT:
                    if (pmbus_app_set_vout_uv_fault_limit(word_value) != 0U)
                    {
                        return 1U;
                    }
                    break;

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
#endif

#if PMBUS_ENABLE_CMD_FAN
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
#endif

#if PMBUS_ENABLE_CMD_CORE
                case PMBUS_COMMAND_VOUT_COMMAND:
                    if (pmbus_app_set_vout_command(word_value) != 0U)
                    {
                        return 1U;
                    }
                    break;
#endif

#if PMBUS_ENABLE_CMD_LIMITS
                case PMBUS_COMMAND_VOUT_TRIM:
                    if (pmbus_app_set_vout_trim(word_value) != 0U)
                    {
                        return 1U;
                    }
                    break;

                case PMBUS_COMMAND_VOUT_CAL_OFFSET:
                    if (pmbus_app_set_vout_cal_offset(word_value) != 0U)
                    {
                        return 1U;
                    }
                    break;

                case PMBUS_COMMAND_VOUT_MAX:
                    if (pmbus_app_set_vout_max(word_value) != 0U)
                    {
                        return 1U;
                    }
                    break;

                case PMBUS_COMMAND_VOUT_MARGIN_HIGH:
                    if (pmbus_app_set_vout_margin_high(word_value) != 0U)
                    {
                        return 1U;
                    }
                    break;

                case PMBUS_COMMAND_VOUT_MARGIN_LOW:
                    if (pmbus_app_set_vout_margin_low(word_value) != 0U)
                    {
                        return 1U;
                    }
                    break;

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
                    if (pmbus_app_set_vout_min(word_value) != 0U)
                    {
                        return 1U;
                    }
                    break;

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
                    if (pmbus_app_set_power_good_on(word_value) != 0U)
                    {
                        return 1U;
                    }
                    break;

                case PMBUS_COMMAND_POWER_GOOD_OFF:
                    if (pmbus_app_set_power_good_off(word_value) != 0U)
                    {
                        return 1U;
                    }
                    break;

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
#endif

#if PMBUS_ENABLE_CMD_CORE
                case PMBUS_COMMAND_SMBALERT_MASK:
                    pmbus_app_set_smbalert_mask(word_value);
                    return 1U;
#endif

#if PMBUS_ENABLE_CMD_MFR_EXT
#if PMBUS_ENABLE_CMD_CRPS
                case PMBUS_COMMAND_MFR_SYSTEM_LED_CNTL:
                    pmbus_app_set_mfr_system_led_cntl(word_value);
                    return 1U;

                case PMBUS_COMMAND_MFR_VOUT_MARGINING:
                    pmbus_app_set_mfr_vout_margining(word_value);
                    return 1U;
#endif

                case PMBUS_COMMAND_MFR_PWOK_WARNING_TIME:
                    pmbus_app_set_mfr_pwok_warning_time(word_value);
                    return 1U;

#if PMBUS_ENABLE_CMD_CRPS
                case PMBUS_COMMAND_MFR_FPC_MAIN_MIN_OFF_TIME:
                    pmbus_app_set_mfr_fpc_main_min_off_time(word_value);
                    return 1U;

                case PMBUS_COMMAND_MFR_FPC_12VSB_MIN_OFF_TIME:
                    pmbus_app_set_mfr_fpc_12vsb_min_off_time(word_value);
                    return 1U;
#endif
#endif

#if PMBUS_ENABLE_CMD_FWUPLOAD
                case PMBUS_COMMAND_MFR_FWUPLOAD_STATUS:
                    pmbus_app_set_mfr_fwupload_status(word_value);
                    return 1U;
#endif

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

#if PMBUS_ENABLE_CMD_ENERGY
        case PMBUS_PROTOCOL_READ_DWORD:
            if (pmbus_dispatch_build_dword_response(transaction->command, tx_buffer, tx_length) != 0U)
            {
                return 1U;
            }
            break;
#endif

        case PMBUS_PROTOCOL_BLOCK_READ:
            if (pmbus_dispatch_build_block_response(transaction->command, tx_buffer, tx_length) != 0U)
            {
                return 1U;
            }
            break;

        case PMBUS_PROTOCOL_PROCESS_CALL:
#if PMBUS_ENABLE_CMD_CORE
            if (transaction->command == PMBUS_COMMAND_VOUT_COMMAND)
            {
                if (pmbus_dispatch_is_write_locked() != 0U)
                {
                    break;
                }

                word_value = (uint16_t)(((uint16_t)transaction->payload[1] << 8) | transaction->payload[0]);
                if (pmbus_app_set_vout_command(word_value) == 0U)
                {
                    break;
                }
                pmbus_dispatch_store_word(tx_buffer, pmbus_app_get_vout_command());
                *tx_length = 2U;
                return 1U;
            }
#endif
            break;

        case PMBUS_PROTOCOL_BLOCK_WRITE:
            if (pmbus_dispatch_is_write_locked() != 0U)
            {
                break;
            }

#if PMBUS_ENABLE_CMD_POLICY
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
#endif
#if PMBUS_ENABLE_CMD_PAGE_PLUS
            if (transaction->command == PMBUS_COMMAND_PAGE_PLUS_WRITE)
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
#endif
#if PMBUS_ENABLE_CMD_FWUPLOAD
            if (transaction->command == PMBUS_COMMAND_MFR_FWUPLOAD)
            {
                if (transaction->data_len >= 1U)
                {
                    if (pmbus_app_store_mfr_fwupload_block(&transaction->payload[1], transaction->payload[0]) != 0U)
                    {
                        return 1U;
                    }
                }
            }
#endif
#if PMBUS_ENABLE_CMD_MFR_EXT
            if ((transaction->command == PMBUS_COMMAND_MFR_REAL_TIME_BLACK_BOX) ||
                (transaction->command == PMBUS_COMMAND_MFR_SYSTEM_BLACK_BOX))
            {
                if (transaction->data_len >= 1U)
                {
                    if (transaction->command == PMBUS_COMMAND_MFR_REAL_TIME_BLACK_BOX)
                    {
                        pmbus_app_set_mfr_real_time_black_box(&transaction->payload[1], transaction->payload[0]);
                    }
                    else
                    {
                        pmbus_app_set_mfr_system_black_box(&transaction->payload[1], transaction->payload[0]);
                    }
                    return 1U;
                }
            }

#if PMBUS_ENABLE_CMD_CRPS
            if ((transaction->command == PMBUS_COMMAND_MFR_PEAK_CURRENT_RECORD) ||
                (transaction->command == PMBUS_COMMAND_MFR_OCWPL1_SETTING))
            {
                if (transaction->data_len >= 1U)
                {
                    if (transaction->command == PMBUS_COMMAND_MFR_PEAK_CURRENT_RECORD)
                    {
                        pmbus_app_set_mfr_peak_current_record(&transaction->payload[1], transaction->payload[0]);
                    }
                    else
                    {
                        pmbus_app_set_mfr_ocwpl1_setting(&transaction->payload[1], transaction->payload[0]);
                    }
                    return 1U;
                }
            }
#endif
#endif
            break;

        case PMBUS_PROTOCOL_BLOCK_WRITE_READ_PROCESS_CALL:
#if PMBUS_ENABLE_CMD_POLICY
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
#endif
#if PMBUS_ENABLE_CMD_CORE
            if (transaction->command == PMBUS_COMMAND_QUERY)
            {
                if (pmbus_dispatch_build_query_response(transaction->payload,
                    transaction->data_len,
                    tx_buffer,
                    tx_length) != 0U)
                {
                    return 1U;
                }
            }
#endif
#if PMBUS_ENABLE_CMD_COEFFICIENTS
            if (transaction->command == PMBUS_COMMAND_COEFFICIENTS)
            {
                if (pmbus_dispatch_build_coefficients_response(transaction->payload,
                    transaction->data_len,
                    tx_buffer,
                    tx_length) != 0U)
                {
                    return 1U;
                }
            }
#endif
#if PMBUS_ENABLE_CMD_CORE
            if (transaction->command == PMBUS_COMMAND_SMBALERT_MASK)
            {
                if (pmbus_dispatch_build_smbalert_mask_response(transaction->payload,
                    transaction->data_len,
                    tx_buffer,
                    tx_length) != 0U)
                {
                    return 1U;
                }
            }
#endif
#if PMBUS_ENABLE_CMD_PAGE_PLUS
            if (transaction->command == PMBUS_COMMAND_PAGE_PLUS_READ)
            {
                if (pmbus_dispatch_build_page_plus_read_response(transaction->payload,
                    transaction->data_len,
                    tx_buffer,
                    tx_length) != 0U)
                {
                    return 1U;
                }
            }
#endif
#if PMBUS_ENABLE_CMD_MFR_EXT
#if PMBUS_ENABLE_CMD_CRPS
            if ((transaction->command == PMBUS_COMMAND_MFR_READ_CONFIG_FILE) ||
                (transaction->command == PMBUS_COMMAND_MFR_SPDM))
            {
                /* Payload is a CRPS/product selector placeholder.
                   TODO: Use it as a real config-file offset or SPDM request. */
                if (transaction->data_len >= 1U)
                {
                    if (pmbus_dispatch_build_block_response(transaction->command, tx_buffer, tx_length) != 0U)
                    {
                        return 1U;
                    }
                }
            }
#endif

            if ((transaction->command == PMBUS_COMMAND_MFR_REAL_TIME_BLACK_BOX) ||
                (transaction->command == PMBUS_COMMAND_MFR_SYSTEM_BLACK_BOX))
            {
                if (pmbus_dispatch_build_block_response(transaction->command, tx_buffer, tx_length) != 0U)
                {
                    return 1U;
                }
            }
#endif
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
