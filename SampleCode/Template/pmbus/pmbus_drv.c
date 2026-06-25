#include <stdio.h>
#include "pmbus_types.h"
#include "board_config.h"
#include "pmbus_dispatch.h"
#include "pmbus_app.h"
#include "pmbus_io.h"
#include "pmbus_pec.h"
#include "pmbus_protocol.h"
#include "pmbus_profile_names.h"
#include "pmbus_semantics.h"

extern uint32_t get_tick(void);

typedef enum
{
    PMBUS_DEBUG_EVENT_STATUS = 0,
    PMBUS_DEBUG_EVENT_RX_FRAME,
    PMBUS_DEBUG_EVENT_RX_OVERFLOW,
    PMBUS_DEBUG_EVENT_TX_READY,
    PMBUS_DEBUG_EVENT_UNSUPPORTED,
    PMBUS_DEBUG_EVENT_PEC_ERROR,
    PMBUS_DEBUG_EVENT_WRITE_DONE,
    PMBUS_DEBUG_EVENT_RECOVER,
    PMBUS_DEBUG_EVENT_RECOVER_FAIL,
    PMBUS_DEBUG_EVENT_ARA_ALIAS,
    PMBUS_DEBUG_EVENT_ARP,
    PMBUS_DEBUG_EVENT_ZONE,
    PMBUS_DEBUG_EVENT_CLOCK_LOW_TIMEOUT
} pmbus_debug_event_id_t;

typedef enum
{
    PMBUS_RECOVER_REASON_NONE = 0,
    PMBUS_RECOVER_REASON_TIMEOUT,
    PMBUS_RECOVER_REASON_BUS_ERROR,
    PMBUS_RECOVER_REASON_RX_OVERFLOW
} pmbus_recover_reason_t;

typedef enum
{
    PMBUS_FRAME_CLASS_NONE = 0,
    PMBUS_FRAME_CLASS_WRITE_ONLY,
    PMBUS_FRAME_CLASS_READ_ONLY,
    PMBUS_FRAME_CLASS_AMBIGUOUS
} pmbus_frame_class_t;

typedef enum
{
    PMBUS_RECOVER_STATE_IDLE = 0,
    PMBUS_RECOVER_STATE_PENDING,
    PMBUS_RECOVER_STATE_BACKOFF,
    PMBUS_RECOVER_STATE_STUCK_BUS
} pmbus_recover_state_t;

typedef enum
{
    PMBUS_REQUEST_TARGET_NORMAL = 0,
    PMBUS_REQUEST_TARGET_ARA,
    PMBUS_REQUEST_TARGET_ARP,
    PMBUS_REQUEST_TARGET_ZONE_READ,
    PMBUS_REQUEST_TARGET_ZONE_WRITE
} pmbus_request_target_t;

#define PMBUS_ARP_COMMAND_RESET_DEVICE        0x00U
#define PMBUS_ARP_COMMAND_PREPARE_TO_ARP      0x01U
#define PMBUS_ARP_COMMAND_GET_UDID            0x03U
#define PMBUS_ARP_COMMAND_ASSIGN_ADDRESS      0x04U
#define PMBUS_ARP_COMMAND_DIRECTED_GET_UDID   0x05U
#define PMBUS_ARP_UDID_LENGTH                 17U

typedef struct
{
    uint8_t event_id;
    uint8_t value0;
    uint8_t value1;
} pmbus_debug_event_t;

typedef struct
{
    uint8_t command;
    const char *name;
} pmbus_command_name_t;

#define PMBUS_DEBUG_FRAME_QUEUE_SIZE          4U
#define PMBUS_DEBUG_TX_QUEUE_SIZE             4U

typedef struct
{
    uint8_t address_7bit;
    uint8_t raw_len;
    uint8_t payload_len;
    uint8_t protocol;
    uint8_t repeated_start;
    uint8_t pec_present;
    uint8_t pec_valid;
    uint8_t raw[PMBUS_RX_BUFFER_SIZE];
} pmbus_debug_frame_snapshot_t;

typedef struct
{
    uint8_t command;
    uint8_t protocol;
    uint8_t command_length;
    uint8_t pec_present;
    uint8_t raw_len;
    uint8_t command_bytes[4];
    uint8_t raw[PMBUS_TX_BUFFER_SIZE];
} pmbus_debug_tx_snapshot_t;

static volatile uint8_t g_rx_length;
static volatile uint8_t g_tx_length;
static volatile uint8_t g_tx_index;
static volatile uint8_t g_write_frame_pending;
static volatile uint32_t g_write_frame_pending_tick;
static volatile uint8_t g_last_command;
static volatile uint8_t g_last_command_valid;
static volatile uint8_t g_last_read_used_pec;
static volatile uint8_t g_current_slave_address_7bit;
static volatile uint8_t g_request_address_7bit;
static volatile uint8_t g_request_target;
static volatile uint8_t g_pending_request_address_7bit;
static volatile uint8_t g_pending_request_target;
static volatile uint8_t g_ara_alias_active;
static volatile uint8_t g_ara_alias_inhibit;
static volatile uint8_t g_arp_prepared;
static volatile uint8_t g_arp_address_resolved;
static volatile uint8_t g_arp_last_command;
static volatile uint8_t g_address_change_pending;
static volatile uint8_t g_pending_slave_address_7bit;
static volatile uint8_t g_tx_finish_bus_error_guard;
static volatile uint8_t g_recover_pending;
static volatile uint8_t g_recover_reason;
static volatile uint8_t g_recover_state;
static volatile uint8_t g_recover_attempt_count;
static volatile uint8_t g_recover_backoff_count;
static volatile uint8_t g_timeout_fault_count;
static volatile uint16_t g_software_scl_low_ms;
static volatile uint8_t g_software_scl_low_state;
static volatile uint8_t g_software_scl_low_monitor_enabled;
static volatile uint8_t g_bus_error_fault_count;
static volatile uint8_t g_rx_overflow_fault_count;
static volatile uint8_t g_recover_count;
static volatile uint8_t g_recover_fail_count;

static uint8_t g_rx_buffer[PMBUS_RX_BUFFER_SIZE];
static uint8_t g_tx_buffer[PMBUS_TX_BUFFER_SIZE];
static uint8_t g_arp_udid[PMBUS_ARP_UDID_LENGTH];
static pmbus_dispatch_transaction_t g_dispatch_transaction;

static volatile uint8_t g_debug_head;
static volatile uint8_t g_debug_tail;
static pmbus_debug_event_t g_debug_queue[PMBUS_DEBUG_QUEUE_SIZE];
static volatile uint8_t g_debug_frame_head;
static volatile uint8_t g_debug_frame_tail;
static pmbus_debug_frame_snapshot_t g_debug_frame_queue[PMBUS_DEBUG_FRAME_QUEUE_SIZE];
static volatile uint8_t g_debug_tx_head;
static volatile uint8_t g_debug_tx_tail;
static pmbus_debug_tx_snapshot_t g_debug_tx_queue[PMBUS_DEBUG_TX_QUEUE_SIZE];

static const pmbus_command_name_t g_pmbus_command_names[] =
{
    { PMBUS_COMMAND_PAGE, "PAGE" },
    { PMBUS_COMMAND_OPERATION, "OPERATION" },
    { PMBUS_COMMAND_ON_OFF_CONFIG, "ON_OFF_CONFIG" },
    { PMBUS_COMMAND_CLEAR_FAULTS, "CLEAR_FAULTS" },
    { PMBUS_COMMAND_PHASE, "PHASE" },
    { PMBUS_COMMAND_PAGE_PLUS_WRITE, "PAGE_PLUS_WRITE" },
    { PMBUS_COMMAND_PAGE_PLUS_READ, "PAGE_PLUS_READ" },
    { PMBUS_COMMAND_ZONE_CONFIG, "ZONE_CONFIG" },
    { PMBUS_COMMAND_ZONE_ACTIVE, "ZONE_ACTIVE" },
    { PMBUS_COMMAND_WRITE_PROTECT, "WRITE_PROTECT" },
    { PMBUS_COMMAND_STORE_DEFAULT_ALL, "STORE_DEFAULT_ALL" },
    { PMBUS_COMMAND_RESTORE_DEFAULT_ALL, "RESTORE_DEFAULT_ALL" },
    { PMBUS_COMMAND_STORE_DEFAULT_CODE, "STORE_DEFAULT_CODE" },
    { PMBUS_COMMAND_RESTORE_DEFAULT_CODE, "RESTORE_DEFAULT_CODE" },
    { PMBUS_COMMAND_STORE_USER_ALL, "STORE_USER_ALL" },
    { PMBUS_COMMAND_RESTORE_USER_ALL, "RESTORE_USER_ALL" },
    { PMBUS_COMMAND_STORE_USER_CODE, "STORE_USER_CODE" },
    { PMBUS_COMMAND_RESTORE_USER_CODE, "RESTORE_USER_CODE" },
    { PMBUS_COMMAND_CAPABILITY, "CAPABILITY" },
    { PMBUS_COMMAND_QUERY, "QUERY" },
    { PMBUS_COMMAND_SMBALERT_MASK, "SMBALERT_MASK" },
    { PMBUS_COMMAND_VOUT_MODE, "VOUT_MODE" },
    { PMBUS_COMMAND_VOUT_COMMAND, "VOUT_COMMAND" },
    { PMBUS_COMMAND_VOUT_TRIM, "VOUT_TRIM" },
    { PMBUS_COMMAND_VOUT_CAL_OFFSET, "VOUT_CAL_OFFSET" },
    { PMBUS_COMMAND_VOUT_MAX, "VOUT_MAX" },
    { PMBUS_COMMAND_VOUT_MARGIN_HIGH, "VOUT_MARGIN_HIGH" },
    { PMBUS_COMMAND_VOUT_MARGIN_LOW, "VOUT_MARGIN_LOW" },
    { PMBUS_COMMAND_VOUT_TRANSITION_RATE, "VOUT_TRANSITION_RATE" },
    { PMBUS_COMMAND_VOUT_DROOP, "VOUT_DROOP" },
    { PMBUS_COMMAND_VOUT_SCALE_LOOP, "VOUT_SCALE_LOOP" },
    { PMBUS_COMMAND_VOUT_SCALE_MONITOR, "VOUT_SCALE_MONITOR" },
    { PMBUS_COMMAND_VOUT_MIN, "VOUT_MIN" },
    { PMBUS_COMMAND_COEFFICIENTS, "COEFFICIENTS" },
    { PMBUS_COMMAND_POUT_MAX, "POUT_MAX" },
    { PMBUS_COMMAND_MAX_DUTY, "MAX_DUTY" },
    { PMBUS_COMMAND_FREQUENCY_SWITCH, "FREQUENCY_SWITCH" },
    { PMBUS_COMMAND_POWER_MODE, "POWER_MODE" },
    { PMBUS_COMMAND_VIN_ON, "VIN_ON" },
    { PMBUS_COMMAND_VIN_OFF, "VIN_OFF" },
    { PMBUS_COMMAND_INTERLEAVE, "INTERLEAVE" },
    { PMBUS_COMMAND_IOUT_CAL_GAIN, "IOUT_CAL_GAIN" },
    { PMBUS_COMMAND_IOUT_CAL_OFFSET, "IOUT_CAL_OFFSET" },
    { PMBUS_COMMAND_FAN_CONFIG_1_2, "FAN_CONFIG_1_2" },
    { PMBUS_COMMAND_FAN_COMMAND_1, "FAN_COMMAND_1" },
    { PMBUS_COMMAND_FAN_COMMAND_2, "FAN_COMMAND_2" },
    { PMBUS_COMMAND_FAN_CONFIG_3_4, "FAN_CONFIG_3_4" },
    { PMBUS_COMMAND_FAN_COMMAND_3, "FAN_COMMAND_3" },
    { PMBUS_COMMAND_FAN_COMMAND_4, "FAN_COMMAND_4" },
    { PMBUS_COMMAND_VOUT_OV_FAULT_LIMIT, "VOUT_OV_FAULT_LIMIT" },
    { PMBUS_COMMAND_VOUT_OV_FAULT_RESPONSE, "VOUT_OV_FAULT_RESPONSE" },
    { PMBUS_COMMAND_VOUT_OV_WARN_LIMIT, "VOUT_OV_WARN_LIMIT" },
    { PMBUS_COMMAND_VOUT_UV_WARN_LIMIT, "VOUT_UV_WARN_LIMIT" },
    { PMBUS_COMMAND_VOUT_UV_FAULT_LIMIT, "VOUT_UV_FAULT_LIMIT" },
    { PMBUS_COMMAND_VOUT_UV_FAULT_RESPONSE, "VOUT_UV_FAULT_RESPONSE" },
    { PMBUS_COMMAND_IOUT_OC_FAULT_LIMIT, "IOUT_OC_FAULT_LIMIT" },
    { PMBUS_COMMAND_IOUT_OC_FAULT_RESPONSE, "IOUT_OC_FAULT_RESPONSE" },
    { PMBUS_COMMAND_IOUT_OC_LV_FAULT_LIMIT, "IOUT_OC_LV_FAULT_LIMIT" },
    { PMBUS_COMMAND_IOUT_OC_LV_FAULT_RESPONSE, "IOUT_OC_LV_FAULT_RESPONSE" },
    { PMBUS_COMMAND_IOUT_OC_WARN_LIMIT, "IOUT_OC_WARN_LIMIT" },
    { PMBUS_COMMAND_IOUT_UC_FAULT_LIMIT, "IOUT_UC_FAULT_LIMIT" },
    { PMBUS_COMMAND_IOUT_UC_FAULT_RESPONSE, "IOUT_UC_FAULT_RESPONSE" },
    { PMBUS_COMMAND_OT_FAULT_LIMIT, "OT_FAULT_LIMIT" },
    { PMBUS_COMMAND_OT_FAULT_RESPONSE, "OT_FAULT_RESPONSE" },
    { PMBUS_COMMAND_OT_WARN_LIMIT, "OT_WARN_LIMIT" },
    { PMBUS_COMMAND_UT_WARN_LIMIT, "UT_WARN_LIMIT" },
    { PMBUS_COMMAND_UT_FAULT_LIMIT, "UT_FAULT_LIMIT" },
    { PMBUS_COMMAND_UT_FAULT_RESPONSE, "UT_FAULT_RESPONSE" },
    { PMBUS_COMMAND_VIN_OV_FAULT_LIMIT, "VIN_OV_FAULT_LIMIT" },
    { PMBUS_COMMAND_VIN_OV_FAULT_RESPONSE, "VIN_OV_FAULT_RESPONSE" },
    { PMBUS_COMMAND_VIN_OV_WARN_LIMIT, "VIN_OV_WARN_LIMIT" },
    { PMBUS_COMMAND_VIN_UV_WARN_LIMIT, "VIN_UV_WARN_LIMIT" },
    { PMBUS_COMMAND_VIN_UV_FAULT_LIMIT, "VIN_UV_FAULT_LIMIT" },
    { PMBUS_COMMAND_VIN_UV_FAULT_RESPONSE, "VIN_UV_FAULT_RESPONSE" },
    { PMBUS_COMMAND_IIN_OC_FAULT_LIMIT, "IIN_OC_FAULT_LIMIT" },
    { PMBUS_COMMAND_IIN_OC_FAULT_RESPONSE, "IIN_OC_FAULT_RESPONSE" },
    { PMBUS_COMMAND_IIN_OC_WARN_LIMIT, "IIN_OC_WARN_LIMIT" },
    { PMBUS_COMMAND_POWER_GOOD_ON, "POWER_GOOD_ON" },
    { PMBUS_COMMAND_POWER_GOOD_OFF, "POWER_GOOD_OFF" },
    { PMBUS_COMMAND_TON_DELAY, "TON_DELAY" },
    { PMBUS_COMMAND_TON_RISE, "TON_RISE" },
    { PMBUS_COMMAND_TON_MAX_FAULT_LIMIT, "TON_MAX_FAULT_LIMIT" },
    { PMBUS_COMMAND_TON_MAX_FAULT_RESPONSE, "TON_MAX_FAULT_RESPONSE" },
    { PMBUS_COMMAND_TOFF_DELAY, "TOFF_DELAY" },
    { PMBUS_COMMAND_TOFF_FALL, "TOFF_FALL" },
    { PMBUS_COMMAND_TOFF_MAX_WARN_LIMIT, "TOFF_MAX_WARN_LIMIT" },
    { PMBUS_COMMAND_POUT_OP_FAULT_LIMIT, "POUT_OP_FAULT_LIMIT" },
    { PMBUS_COMMAND_POUT_OP_FAULT_RESPONSE, "POUT_OP_FAULT_RESPONSE" },
    { PMBUS_COMMAND_POUT_OP_WARN_LIMIT, "POUT_OP_WARN_LIMIT" },
    { PMBUS_COMMAND_PIN_OP_WARN_LIMIT, "PIN_OP_WARN_LIMIT" },
    { PMBUS_COMMAND_STATUS_BYTE, "STATUS_BYTE" },
    { PMBUS_COMMAND_STATUS_WORD, "STATUS_WORD" },
    { PMBUS_COMMAND_STATUS_VOUT, "STATUS_VOUT" },
    { PMBUS_COMMAND_STATUS_IOUT, "STATUS_IOUT" },
    { PMBUS_COMMAND_STATUS_INPUT, "STATUS_INPUT" },
    { PMBUS_COMMAND_STATUS_TEMPERATURE, "STATUS_TEMPERATURE" },
    { PMBUS_COMMAND_STATUS_CML, "STATUS_CML" },
    { PMBUS_COMMAND_STATUS_OTHER, "STATUS_OTHER" },
    { PMBUS_COMMAND_STATUS_MFR_SPECIFIC, "STATUS_MFR_SPECIFIC" },
    { PMBUS_COMMAND_STATUS_FANS_1_2, "STATUS_FANS_1_2" },
    { PMBUS_COMMAND_STATUS_FANS_3_4, "STATUS_FANS_3_4" },
    { PMBUS_COMMAND_READ_KWH_IN, "READ_KWH_IN" },
    { PMBUS_COMMAND_READ_KWH_OUT, "READ_KWH_OUT" },
    { PMBUS_COMMAND_READ_KWH_CONFIG, "READ_KWH_CONFIG" },
    { PMBUS_COMMAND_READ_EIN, "READ_EIN" },
    { PMBUS_COMMAND_READ_EOUT, "READ_EOUT" },
    { PMBUS_COMMAND_READ_VIN, "READ_VIN" },
    { PMBUS_COMMAND_READ_IIN, "READ_IIN" },
    { PMBUS_COMMAND_READ_VCAP, "READ_VCAP" },
    { PMBUS_COMMAND_READ_VOUT, "READ_VOUT" },
    { PMBUS_COMMAND_READ_IOUT, "READ_IOUT" },
    { PMBUS_COMMAND_READ_TEMPERATURE_1, "READ_TEMPERATURE_1" },
    { PMBUS_COMMAND_READ_TEMPERATURE_2, "READ_TEMPERATURE_2" },
    { PMBUS_COMMAND_READ_TEMPERATURE_3, "READ_TEMPERATURE_3" },
    { PMBUS_COMMAND_READ_FAN_SPEED_1, "READ_FAN_SPEED_1" },
    { PMBUS_COMMAND_READ_FAN_SPEED_2, "READ_FAN_SPEED_2" },
    { PMBUS_COMMAND_READ_FAN_SPEED_3, "READ_FAN_SPEED_3" },
    { PMBUS_COMMAND_READ_FAN_SPEED_4, "READ_FAN_SPEED_4" },
    { PMBUS_COMMAND_READ_DUTY_CYCLE, "READ_DUTY_CYCLE" },
    { PMBUS_COMMAND_READ_FREQUENCY, "READ_FREQUENCY" },
    { PMBUS_COMMAND_READ_POUT, "READ_POUT" },
    { PMBUS_COMMAND_READ_PIN, "READ_PIN" },
    { PMBUS_COMMAND_PMBUS_REVISION, "PMBUS_REVISION" },
    { PMBUS_COMMAND_MFR_ID, "MFR_ID" },
    { PMBUS_COMMAND_MFR_MODEL, "MFR_MODEL" },
    { PMBUS_COMMAND_MFR_REVISION, "MFR_REVISION" },
    { PMBUS_COMMAND_MFR_LOCATION, "MFR_LOCATION" },
    { PMBUS_COMMAND_MFR_DATE, "MFR_DATE" },
    { PMBUS_COMMAND_MFR_SERIAL, "MFR_SERIAL" },
    { PMBUS_COMMAND_APP_PROFILE_SUPPORT, "APP_PROFILE_SUPPORT" },
    { PMBUS_COMMAND_MFR_VIN_MIN, "MFR_VIN_MIN" },
    { PMBUS_COMMAND_MFR_VIN_MAX, "MFR_VIN_MAX" },
    { PMBUS_COMMAND_MFR_IIN_MAX, "MFR_IIN_MAX" },
    { PMBUS_COMMAND_MFR_PIN_MAX, "MFR_PIN_MAX" },
    { PMBUS_COMMAND_MFR_VOUT_MIN, "MFR_VOUT_MIN" },
    { PMBUS_COMMAND_MFR_VOUT_MAX, "MFR_VOUT_MAX" },
    { PMBUS_COMMAND_MFR_IOUT_MAX, "MFR_IOUT_MAX" },
    { PMBUS_COMMAND_MFR_POUT_MAX, "MFR_POUT_MAX" },
    { PMBUS_COMMAND_MFR_TAMBIENT_MAX, "MFR_TAMBIENT_MAX" },
    { PMBUS_COMMAND_MFR_TAMBIENT_MIN, "MFR_TAMBIENT_MIN" },
    { PMBUS_COMMAND_MFR_EFFICIENCY_LL, "MFR_EFFICIENCY_LL" },
    { PMBUS_COMMAND_MFR_EFFICIENCY_HL, "MFR_EFFICIENCY_HL" },
    { PMBUS_COMMAND_MFR_EFFICIENCY_DATA, "MFR_EFFICIENCY_DATA" },
    { PMBUS_COMMAND_MFR_PIN_ACCURACY, "MFR_PIN_ACCURACY" },
    { PMBUS_COMMAND_IC_DEVICE_ID, "IC_DEVICE_ID" },
    { PMBUS_COMMAND_IC_DEVICE_REV, "IC_DEVICE_REV" },
    { PMBUS_COMMAND_MFR_MAX_TEMP_1, "MFR_MAX_TEMP_1" },
    { PMBUS_COMMAND_MFR_MAX_TEMP_2, "MFR_MAX_TEMP_2" },
    { PMBUS_COMMAND_MFR_MAX_TEMP_3, "MFR_MAX_TEMP_3" },
    { PMBUS_COMMAND_MFR_COLD_REDUNDANCY_CONFIG, "MFR_COLD_REDUNDANCY_CONFIG" },
    { PMBUS_COMMAND_MFR_READ_CONFIG_FILE_SIZE, "MFR_READ_CONFIG_FILE_SIZE" },
    { PMBUS_COMMAND_MFR_READ_CONFIG_BLOCK_SIZE, "MFR_READ_CONFIG_BLOCK_SIZE" },
    { PMBUS_COMMAND_MFR_READ_CONFIG_FILE, "MFR_READ_CONFIG_FILE" },
    { PMBUS_COMMAND_MFR_HW_COMPATIBILITY, "MFR_HW_COMPATIBILITY" },
    { PMBUS_COMMAND_MFR_FWUPLOAD_CAPABILITY, "MFR_FWUPLOAD_CAPABILITY" },
    { PMBUS_COMMAND_MFR_FWUPLOAD_MODE, "MFR_FWUPLOAD_MODE" },
    { PMBUS_COMMAND_MFR_FWUPLOAD, "MFR_FWUPLOAD" },
    { PMBUS_COMMAND_MFR_FWUPLOAD_STATUS, "MFR_FWUPLOAD_STATUS" },
    { PMBUS_COMMAND_MFR_FW_REVISION, "MFR_FW_REVISION" },
    { PMBUS_COMMAND_MFR_SPDM, "MFR_SPDM" },
    { PMBUS_COMMAND_MFR_FRU_PROTECTION, "MFR_FRU_PROTECTION" },
    { PMBUS_COMMAND_MFR_BLACKBOX, "MFR_BLACKBOX" },
    { PMBUS_COMMAND_MFR_REAL_TIME_BLACK_BOX, "MFR_REAL_TIME_BLACK_BOX" },
    { PMBUS_COMMAND_MFR_SYSTEM_BLACK_BOX, "MFR_SYSTEM_BLACK_BOX" },
    { PMBUS_COMMAND_MFR_BLACK_BOX_CONFIG, "MFR_BLACK_BOX_CONFIG" },
    { PMBUS_COMMAND_MFR_CLEAR_BLACK_BOX, "MFR_CLEAR_BLACK_BOX" },
    { PMBUS_COMMAND_MFR_LINE_STATUS, "MFR_LINE_STATUS" },
    { PMBUS_COMMAND_MFR_SYSTEM_LED_CNTL, "MFR_SYSTEM_LED_CNTL" },
    { PMBUS_COMMAND_MFR_FWUPLOAD_BLOCK_SIZE, "MFR_FWUPLOAD_BLOCK_SIZE" },
    { PMBUS_COMMAND_MFR_EN_STATUS_SIMULATION_CMD, "MFR_EN_STATUS_SIMULATION_CMD" },
    { PMBUS_COMMAND_MFR_PEAK_CURRENT_RECORD, "MFR_PEAK_CURRENT_RECORD" },
    { PMBUS_COMMAND_MFR_COMPONENT_ID, "MFR_COMPONENT_ID" },
    { PMBUS_COMMAND_MFR_TOT_POUT_MAX, "MFR_TOT_POUT_MAX" },
    { PMBUS_COMMAND_MFR_VOUT_MARGINING, "MFR_VOUT_MARGINING" },
    { PMBUS_COMMAND_MFR_OCWPL1_SETTING, "MFR_OCWPL1_SETTING" },
    { PMBUS_COMMAND_MFR_PWOK_WARNING_TIME, "MFR_PWOK_WARNING_TIME" },
    { PMBUS_COMMAND_MFR_MAX_IOUT_CAPABILITY, "MFR_MAX_IOUT_CAPABILITY" },
    { PMBUS_COMMAND_MFR_FPC_MAIN_MIN_OFF_TIME, "MFR_FPC_MAIN_MIN_OFF_TIME" },
    { PMBUS_COMMAND_MFR_FPC_12VSB_MIN_OFF_TIME, "MFR_FPC_12VSB_MIN_OFF_TIME" }
};

static void pmbus_drv_capture_debug_frame(uint8_t raw_length, pmbus_dispatch_transaction_t *transaction);
static void pmbus_drv_print_debug_frame(const pmbus_debug_frame_snapshot_t *snapshot);
static void pmbus_drv_capture_debug_tx(uint8_t command, uint8_t protocol, uint8_t command_length, uint8_t pec_present);
static void pmbus_drv_print_debug_tx(const pmbus_debug_tx_snapshot_t *snapshot);
static const char *pmbus_drv_get_command_name(uint8_t command);
static const char *pmbus_drv_get_protocol_name(uint8_t protocol);
static void pmbus_drv_queue_event(uint8_t event_id, uint8_t value0, uint8_t value1);
static void pmbus_drv_reset_tx(void);
static void pmbus_drv_reset_rx(void);
static void pmbus_drv_set_active_address(uint8_t address_7bit);
static void pmbus_drv_configure_alias_addresses(void);
static void pmbus_drv_restore_normal_address(void);
static void pmbus_drv_update_request_target(void);
static void pmbus_drv_save_pending_request_context(void);
static void pmbus_drv_restore_pending_request_context(void);
static void pmbus_drv_prepare_ara_response(void);
static void pmbus_drv_prepare_arp_response(void);
static void pmbus_drv_process_arp_frame(uint8_t repeated_start);
static void pmbus_drv_prepare_zone_read_response(void);
static void pmbus_drv_process_zone_write_frame(uint8_t repeated_start);
static void pmbus_drv_update_ara_alias_state(void);
static uint8_t pmbus_drv_bus_lines_released(void);
static uint8_t pmbus_drv_bus_clear(void);
static uint8_t pmbus_drv_recover_bus(void);
static void pmbus_drv_set_recover_pending(uint8_t reason);
static void pmbus_drv_reset_clock_low_monitor(void);
static void pmbus_drv_check_clock_low_timeout_1ms(void);
static void pmbus_drv_append_tx_pec(uint8_t command_length);
static void pmbus_drv_append_read_only_tx_pec(uint8_t address_7bit);
static uint8_t pmbus_drv_should_append_read_pec(uint8_t repeated_start);
static void pmbus_drv_load_next_tx_byte(void);
static uint8_t pmbus_drv_compute_write_pec(uint8_t frame_length_without_pec);
static pmbus_frame_class_t pmbus_drv_classify_current_frame(void);
static void pmbus_drv_prepare_default_read(void);
static void pmbus_drv_process_frame(uint8_t repeated_start);


static void pmbus_drv_queue_event(uint8_t event_id, uint8_t value0, uint8_t value1)
{
#if PMBUS_DEBUG_ENABLE
    uint8_t irq_state;
    uint8_t next_head;

    irq_state = pmbus_io_irq_save_disable();
    next_head = (uint8_t)(g_debug_head + 1U);
    if (next_head >= PMBUS_DEBUG_QUEUE_SIZE)
    {
        next_head = 0U;
    }

    if (next_head != g_debug_tail)
    {
        g_debug_queue[g_debug_head].event_id = event_id;
        g_debug_queue[g_debug_head].value0 = value0;
        g_debug_queue[g_debug_head].value1 = value1;
        g_debug_head = next_head;
    }
    pmbus_io_irq_restore(irq_state);
#else
    event_id = event_id;
    value0 = value0;
    value1 = value1;
#endif
}

static void pmbus_drv_capture_debug_frame(uint8_t raw_length, pmbus_dispatch_transaction_t *transaction)
{
#if PMBUS_DEBUG_ENABLE && PMBUS_DEBUG_PRINT_RX_FRAME
    uint8_t irq_state;
    uint8_t index;
    uint8_t capped_length;
    uint8_t next_frame_head;
    uint8_t next_debug_head;
    pmbus_debug_frame_snapshot_t *snapshot;

    capped_length = raw_length;
    if (capped_length > PMBUS_RX_BUFFER_SIZE)
    {
        capped_length = PMBUS_RX_BUFFER_SIZE;
    }

    irq_state = pmbus_io_irq_save_disable();
    next_frame_head = (uint8_t)(g_debug_frame_head + 1U);
    if (next_frame_head >= PMBUS_DEBUG_FRAME_QUEUE_SIZE)
    {
        next_frame_head = 0U;
    }

    next_debug_head = (uint8_t)(g_debug_head + 1U);
    if (next_debug_head >= PMBUS_DEBUG_QUEUE_SIZE)
    {
        next_debug_head = 0U;
    }

    if ((next_frame_head == g_debug_frame_tail) || (next_debug_head == g_debug_tail))
    {
        pmbus_io_irq_restore(irq_state);
        return;
    }

    snapshot = &g_debug_frame_queue[g_debug_frame_head];
    snapshot->address_7bit = g_pending_request_address_7bit;
    snapshot->raw_len = capped_length;
    snapshot->payload_len = transaction->data_len;
    snapshot->protocol = (uint8_t)transaction->protocol;
    snapshot->repeated_start = transaction->repeated_start;
    snapshot->pec_present = transaction->pec_present;
    snapshot->pec_valid = transaction->pec_valid;

    for (index = 0U; index < capped_length; index++)
    {
        snapshot->raw[index] = g_rx_buffer[index];
    }

    g_debug_queue[g_debug_head].event_id = PMBUS_DEBUG_EVENT_RX_FRAME;
    g_debug_queue[g_debug_head].value0 = g_debug_frame_head;
    g_debug_queue[g_debug_head].value1 = 0U;
    g_debug_frame_head = next_frame_head;
    g_debug_head = next_debug_head;
    pmbus_io_irq_restore(irq_state);
#else
    raw_length = raw_length;
    transaction = transaction;
#endif
}

static const char *pmbus_drv_get_command_name(uint8_t command)
{
    uint8_t index;
    uint8_t count;
    static char policy_name[24];
    const char *profile_name;

    profile_name = pmbus_profile_get_command_name(command, policy_name, (uint8_t)sizeof(policy_name));
    if (profile_name != (const char *)0)
    {
        return profile_name;
    }

    count = (uint8_t)(sizeof(g_pmbus_command_names) / sizeof(g_pmbus_command_names[0]));
    for (index = 0U; index < count; index++)
    {
        if (g_pmbus_command_names[index].command == command)
        {
            return g_pmbus_command_names[index].name;
        }
    }

    if ((command >= PMBUS_COMMAND_USER_DATA_00) &&
        (command <= PMBUS_COMMAND_USER_DATA_15))
    {
        sprintf(policy_name, "USER_DATA_%02u", (unsigned int)(command - PMBUS_COMMAND_USER_DATA_00));
        return policy_name;
    }

    if ((command >= 0xC0U) &&
        (command <= PMBUS_COMMAND_MFR_SPECIFIC_FD))
    {
        sprintf(policy_name, "MFR_SPECIFIC_%02X", (unsigned int)command);
        return policy_name;
    }

    if (command == PMBUS_COMMAND_MFR_SPECIFIC_COMMAND_EXT)
    {
        return "MFR_SPECIFIC_COMMAND_EXT";
    }

    if (command == PMBUS_COMMAND_PMBUS_COMMAND_EXT)
    {
        return "PMBUS_COMMAND_EXT";
    }

    return "UNKNOWN";
}

static const char *pmbus_drv_get_protocol_name(uint8_t protocol)
{
    switch (protocol)
    {
        case PMBUS_PROTOCOL_SEND_BYTE:
            return "SEND_BYTE";

        case PMBUS_PROTOCOL_WRITE_BYTE:
            return "WRITE_BYTE";

        case PMBUS_PROTOCOL_WRITE_WORD:
            return "WRITE_WORD";

        case PMBUS_PROTOCOL_READ_BYTE:
            return "READ_BYTE";

        case PMBUS_PROTOCOL_READ_WORD:
            return "READ_WORD";

        case PMBUS_PROTOCOL_READ_DWORD:
            return "READ_DWORD";

        case PMBUS_PROTOCOL_BLOCK_WRITE:
            return "BLOCK_WRITE";

        case PMBUS_PROTOCOL_BLOCK_READ:
            return "BLOCK_READ";

        case PMBUS_PROTOCOL_PROCESS_CALL:
            return "PROCESS_CALL";

        case PMBUS_PROTOCOL_BLOCK_WRITE_READ_PROCESS_CALL:
            return "BLOCK_WRITE_READ_PROCESS_CALL";

        default:
            return "UNKNOWN";
    }
}

static void pmbus_drv_print_debug_frame(const pmbus_debug_frame_snapshot_t *snapshot)
{
#if PMBUS_DEBUG_ENABLE
    uint8_t index;
    uint8_t command;
    uint8_t address_byte;
    const char *command_name;

    command = 0U;
    if (snapshot->raw_len > 0U)
    {
        command = snapshot->raw[0];
    }
    command_name = pmbus_drv_get_command_name(command);
    address_byte = PMBUS_ADDRESS_7BIT_TO_WRITE(snapshot->address_7bit);

    PMBUS_DEBUG_PRINT("PMBus RX cmd=0x%02X (%s) raw=%u payload=%u proto=%u rs=%u pec=%u valid=%u\r\n",
        (unsigned int)command,
        command_name,
        (unsigned int)snapshot->raw_len,
        (unsigned int)snapshot->payload_len,
        (unsigned int)snapshot->protocol,
        (unsigned int)snapshot->repeated_start,
        (unsigned int)snapshot->pec_present,
        (unsigned int)snapshot->pec_valid);

    PMBUS_DEBUG_PRINT("address=0x%02X:", (unsigned int)address_byte);
    for (index = 0U; index < snapshot->raw_len; index++)
    {
        PMBUS_DEBUG_PRINT("[0x%02X],", (unsigned int)snapshot->raw[index]);

        if ((index+1)%8 ==0)
        {
            PMBUS_DEBUG_PRINT("\r\n");
        }       

    }
    PMBUS_DEBUG_PRINT("\r\n");
#endif
}

static long pmbus_drv_div_round_signed(long numerator, long denominator)
{
    if (numerator >= 0L)
    {
        return (numerator + (denominator / 2L)) / denominator;
    }

    return -((-numerator + (denominator / 2L)) / denominator);
}

static int16_t pmbus_drv_sign_extend_linear11_mantissa(uint16_t raw_value)
{
    int16_t mantissa;

    mantissa = (int16_t)(raw_value & 0x07FFU);
    if ((mantissa & 0x0400) != 0)
    {
        mantissa = (int16_t)(mantissa | (int16_t)0xF800);
    }

    return mantissa;
}

static int8_t pmbus_drv_sign_extend_linear11_exponent(uint16_t raw_value)
{
    int8_t exponent;

    exponent = (int8_t)((raw_value >> 11) & 0x1FU);
    if ((exponent & 0x10) != 0)
    {
        exponent = (int8_t)(exponent | (int8_t)0xE0);
    }

    return exponent;
}

static int8_t pmbus_drv_decode_vout_exponent(uint8_t vout_mode)
{
    int8_t exponent;

    exponent = (int8_t)(vout_mode & PMBUS_VOUT_MODE_PARAMETER_MASK);
    if ((exponent & 0x10) != 0)
    {
        exponent = (int8_t)(exponent | (int8_t)0xE0);
    }

    return exponent;
}

static const char *pmbus_drv_get_vout_mode_format_name(uint8_t vout_mode)
{
    switch (vout_mode & PMBUS_VOUT_MODE_FORMAT_MASK)
    {
        case PMBUS_VOUT_MODE_FORMAT_ULINEAR16:
            return "ULINEAR16";

        case PMBUS_VOUT_MODE_FORMAT_VID:
            return "VID";

        case PMBUS_VOUT_MODE_FORMAT_DIRECT:
            return "DIRECT";

        case PMBUS_VOUT_MODE_FORMAT_IEEE_HALF:
            return "IEEE_HALF";

        default:
            return "UNKNOWN";
    }
}

static long pmbus_drv_linear11_to_fixed4(uint16_t raw_value, uint16_t scale_divisor)
{
    int16_t mantissa;
    int8_t exponent;
    long scaled_value;
    long denominator;

    mantissa = pmbus_drv_sign_extend_linear11_mantissa(raw_value);
    exponent = pmbus_drv_sign_extend_linear11_exponent(raw_value);
    scaled_value = (long)mantissa * 10000L;
    scaled_value = pmbus_drv_div_round_signed(scaled_value, (long)scale_divisor);

    if (exponent >= 0)
    {
        scaled_value = scaled_value * (1L << exponent);
    }
    else
    {
        denominator = (1L << (-exponent));
        scaled_value = pmbus_drv_div_round_signed(scaled_value, denominator);
    }

    return scaled_value;
}

static unsigned long pmbus_drv_ulinear16_to_fixed4(uint16_t raw_value, int8_t exponent)
{
    unsigned long scaled_value;
    unsigned long denominator;

    scaled_value = (unsigned long)raw_value * 10000UL;
    if (exponent >= 0)
    {
        scaled_value = scaled_value * (1UL << exponent);
    }
    else
    {
        denominator = (1UL << (-exponent));
        scaled_value = (scaled_value + (denominator / 2UL)) / denominator;
    }

    return scaled_value;
}

static long pmbus_drv_pow10_signed(int8_t exponent)
{
    long value;
    int8_t index;

    value = 1L;
    if (exponent < 0)
    {
        exponent = (int8_t)(-exponent);
    }

    for (index = 0; index < exponent; index++)
    {
        value *= 10L;
    }

    return value;
}

static int16_t pmbus_drv_decode_s16(uint16_t raw_value)
{
    return (int16_t)raw_value;
}

static int8_t pmbus_drv_decode_s8(uint8_t raw_value)
{
    return (int8_t)raw_value;
}

static uint8_t pmbus_drv_direct_to_fixed4(uint8_t command, uint16_t raw_value, long *fixed4_value)
{
    uint8_t *coefficients;
    uint8_t coefficient_length;
    int16_t m;
    int16_t b;
    int8_t r;
    long y;
    long numerator;
    long denominator;
    long power10;

    if (fixed4_value == 0)
    {
        return 0U;
    }

    coefficient_length = pmbus_app_get_coefficients(command, &coefficients);
    if ((coefficient_length < 5U) || (coefficients == 0))
    {
        return 0U;
    }

    m = pmbus_drv_decode_s16((uint16_t)(((uint16_t)coefficients[1] << 8) | coefficients[0]));
    b = pmbus_drv_decode_s16((uint16_t)(((uint16_t)coefficients[3] << 8) | coefficients[2]));
    r = pmbus_drv_decode_s8(coefficients[4]);
    if (m == 0)
    {
        return 0U;
    }

    y = (long)pmbus_drv_decode_s16(raw_value);
    if (r >= 0)
    {
        power10 = pmbus_drv_pow10_signed(r);
        numerator = (y * 10000L) - ((long)b * 10000L * power10);
        denominator = (long)m * power10;
    }
    else
    {
        power10 = pmbus_drv_pow10_signed(r);
        numerator = ((y * power10) - (long)b) * 10000L;
        denominator = (long)m;
    }

    if (denominator == 0L)
    {
        return 0U;
    }

    *fixed4_value = pmbus_drv_div_round_signed(numerator, denominator);
    return 1U;
}

static uint8_t pmbus_drv_ieee_half_to_fixed4(uint16_t raw_value, long *fixed4_value)
{
    uint8_t sign;
    uint8_t exponent;
    uint16_t fraction;
    long mantissa;
    int8_t shift;
    long value;

    if (fixed4_value == 0)
    {
        return 0U;
    }

    sign = (uint8_t)((raw_value & 0x8000U) != 0U);
    exponent = (uint8_t)((raw_value >> 10) & 0x1FU);
    fraction = (uint16_t)(raw_value & 0x03FFU);

    if (exponent == 0x1FU)
    {
        return 0U;
    }

    if (exponent == 0U)
    {
        mantissa = (long)fraction;
        shift = -24;
    }
    else
    {
        mantissa = 1024L + (long)fraction;
        shift = (int8_t)((int8_t)exponent - 25);
    }

    value = mantissa * 10000L;
    if (shift >= 0)
    {
        value <<= shift;
    }
    else
    {
        value = pmbus_drv_div_round_signed(value, (1L << (-shift)));
    }

    if (sign != 0U)
    {
        value = -value;
    }

    *fixed4_value = value;
    return 1U;
}

static void pmbus_drv_print_fixed4_value(long fixed4_value)
{
    unsigned long abs_value;
    long integer_part;
    unsigned long fraction_part;

    if (fixed4_value < 0L)
    {
        abs_value = (unsigned long)(-fixed4_value);
        integer_part = -(long)(abs_value / 10000UL);
    }
    else
    {
        abs_value = (unsigned long)fixed4_value;
        integer_part = (long)(abs_value / 10000UL);
    }

    fraction_part = abs_value % 10000UL;
    PMBUS_DEBUG_PRINT("%ld.%04lu", integer_part, fraction_part);
}

static uint8_t pmbus_drv_compute_tx_pec(const pmbus_debug_tx_snapshot_t *snapshot, uint8_t *crc_out)
{
    uint8_t crc;
    uint8_t device_write_address;
    uint8_t device_read_address;
    uint8_t index;
    uint8_t tx_data_length;

    if ((snapshot->pec_present == 0U) || (snapshot->raw_len == 0U) || (snapshot->command_length == 0U))
    {
        return 0U;
    }

    tx_data_length = snapshot->raw_len;
    if (tx_data_length == 0U)
    {
        return 0U;
    }
    tx_data_length = (uint8_t)(tx_data_length - 1U);

    device_write_address = PMBUS_ADDRESS_7BIT_TO_WRITE(g_request_address_7bit);
    device_read_address = PMBUS_ADDRESS_7BIT_TO_READ(g_request_address_7bit);

    crc = 0U;
    crc = pmbus_pec_update(crc, device_write_address);
    for (index = 0U; index < snapshot->command_length; index++)
    {
        crc = pmbus_pec_update(crc, snapshot->command_bytes[index]);
    }

    crc = pmbus_pec_update(crc, device_read_address);
    for (index = 0U; index < tx_data_length; index++)
    {
        crc = pmbus_pec_update(crc, snapshot->raw[index]);
    }

    *crc_out = crc;
    return 1U;
}

static void pmbus_drv_capture_debug_tx(uint8_t command, uint8_t protocol, uint8_t command_length, uint8_t pec_present)
{
#if PMBUS_DEBUG_ENABLE && PMBUS_DEBUG_PRINT_TX_READY
    uint8_t irq_state;
    uint8_t next_tx_head;
    uint8_t next_debug_head;
    uint8_t index;
    pmbus_debug_tx_snapshot_t *snapshot;

    irq_state = pmbus_io_irq_save_disable();
    next_tx_head = (uint8_t)(g_debug_tx_head + 1U);
    if (next_tx_head >= PMBUS_DEBUG_TX_QUEUE_SIZE)
    {
        next_tx_head = 0U;
    }

    next_debug_head = (uint8_t)(g_debug_head + 1U);
    if (next_debug_head >= PMBUS_DEBUG_QUEUE_SIZE)
    {
        next_debug_head = 0U;
    }

    if ((next_tx_head == g_debug_tx_tail) || (next_debug_head == g_debug_tail))
    {
        pmbus_io_irq_restore(irq_state);
        return;
    }

    snapshot = &g_debug_tx_queue[g_debug_tx_head];
    snapshot->command = command;
    snapshot->protocol = protocol;
    snapshot->pec_present = pec_present;
    snapshot->raw_len = g_tx_length;
    snapshot->command_length = command_length;
    if (snapshot->command_length > 4U)
    {
        snapshot->command_length = 4U;
    }

    for (index = 0U; index < snapshot->command_length; index++)
    {
        snapshot->command_bytes[index] = g_rx_buffer[index];
    }

    for (index = 0U; index < g_tx_length; index++)
    {
        snapshot->raw[index] = g_tx_buffer[index];
    }

    g_debug_queue[g_debug_head].event_id = PMBUS_DEBUG_EVENT_TX_READY;
    g_debug_queue[g_debug_head].value0 = g_debug_tx_head;
    g_debug_queue[g_debug_head].value1 = 0U;
    g_debug_tx_head = next_tx_head;
    g_debug_head = next_debug_head;
    pmbus_io_irq_restore(irq_state);
#else
    command = command;
    protocol = protocol;
    command_length = command_length;
    pec_present = pec_present;
#endif
}

static void pmbus_drv_print_debug_tx(const pmbus_debug_tx_snapshot_t *snapshot)
{
#if PMBUS_DEBUG_ENABLE && PMBUS_DEBUG_PRINT_TX_READY
    const char *command_name;
    const char *protocol_name;
    uint16_t raw_word;
    long fixed4_value;
    unsigned long ufixed4_value;
    uint8_t crc;
    uint8_t crc_valid;
    uint8_t pec_value;
    uint8_t block_length;
    uint8_t print_length;
    uint8_t index;
    int8_t exponent;
    uint8_t vout_mode;
    uint8_t vout_format;

    command_name = pmbus_drv_get_command_name(snapshot->command);
    protocol_name = pmbus_drv_get_protocol_name(snapshot->protocol);
    PMBUS_DEBUG_PRINT("PMBus TX cmd=0x%02X (%s) proto=%u (%s) len=%u ",
        (unsigned int)snapshot->command,
        command_name,
        (unsigned int)snapshot->protocol,
        protocol_name,
        (unsigned int)snapshot->raw_len);

    if (snapshot->raw_len >= 2U)
    {
        raw_word = (uint16_t)(((uint16_t)snapshot->raw[1] << 8) | snapshot->raw[0]);
    }
    else
    {
        raw_word = 0U;
    }

    switch (snapshot->command)
    {
        case PMBUS_COMMAND_READ_VIN:
        case PMBUS_COMMAND_READ_IIN:
        case PMBUS_COMMAND_READ_IOUT:
        case PMBUS_COMMAND_READ_TEMPERATURE_1:
        case PMBUS_COMMAND_READ_TEMPERATURE_2:
        case PMBUS_COMMAND_READ_TEMPERATURE_3:
        case PMBUS_COMMAND_READ_POUT:
        case PMBUS_COMMAND_READ_PIN:
            fixed4_value = pmbus_drv_linear11_to_fixed4(raw_word, 1U);
            PMBUS_DEBUG_PRINT("value=");
            pmbus_drv_print_fixed4_value(fixed4_value);
            PMBUS_DEBUG_PRINT(" | raw=0x%04X", (unsigned int)raw_word);
            break;

        case PMBUS_COMMAND_VOUT_COMMAND:
        case PMBUS_COMMAND_VOUT_TRIM:
        case PMBUS_COMMAND_VOUT_CAL_OFFSET:
        case PMBUS_COMMAND_VOUT_MAX:
        case PMBUS_COMMAND_VOUT_MARGIN_HIGH:
        case PMBUS_COMMAND_VOUT_MARGIN_LOW:
        case PMBUS_COMMAND_VOUT_MIN:
        case PMBUS_COMMAND_VOUT_OV_FAULT_LIMIT:
        case PMBUS_COMMAND_VOUT_OV_WARN_LIMIT:
        case PMBUS_COMMAND_VOUT_UV_WARN_LIMIT:
        case PMBUS_COMMAND_VOUT_UV_FAULT_LIMIT:
        case PMBUS_COMMAND_POWER_GOOD_ON:
        case PMBUS_COMMAND_POWER_GOOD_OFF:
        case PMBUS_COMMAND_READ_VOUT:
        case PMBUS_COMMAND_MFR_VOUT_MIN:
        case PMBUS_COMMAND_MFR_VOUT_MAX:
            vout_mode = pmbus_app_get_vout_mode();
            vout_format = (uint8_t)(vout_mode & PMBUS_VOUT_MODE_FORMAT_MASK);
            if (vout_format == PMBUS_VOUT_MODE_FORMAT_ULINEAR16)
            {
                exponent = pmbus_drv_decode_vout_exponent(vout_mode);
                ufixed4_value = pmbus_drv_ulinear16_to_fixed4(raw_word, exponent);
                PMBUS_DEBUG_PRINT("value=%lu.%04lu | mode=%s | exp=%d | raw=0x%04X",
                    ufixed4_value / 10000UL,
                    ufixed4_value % 10000UL,
                    pmbus_drv_get_vout_mode_format_name(vout_mode),
                    (int)exponent,
                    (unsigned int)raw_word);
            }
            else if (vout_format == PMBUS_VOUT_MODE_FORMAT_DIRECT)
            {
                if (pmbus_drv_direct_to_fixed4(snapshot->command, raw_word, &fixed4_value) != 0U)
                {
                    PMBUS_DEBUG_PRINT("value=");
                    pmbus_drv_print_fixed4_value(fixed4_value);
                    PMBUS_DEBUG_PRINT(" | mode=%s | raw=0x%04X",
                        pmbus_drv_get_vout_mode_format_name(vout_mode),
                        (unsigned int)raw_word);
                }
                else
                {
                    PMBUS_DEBUG_PRINT("value_raw=0x%04X | mode=%s | coefficients unavailable",
                        (unsigned int)raw_word,
                        pmbus_drv_get_vout_mode_format_name(vout_mode));
                }
            }
            else if (vout_format == PMBUS_VOUT_MODE_FORMAT_IEEE_HALF)
            {
                if (pmbus_drv_ieee_half_to_fixed4(raw_word, &fixed4_value) != 0U)
                {
                    PMBUS_DEBUG_PRINT("value=");
                    pmbus_drv_print_fixed4_value(fixed4_value);
                    PMBUS_DEBUG_PRINT(" | mode=%s | raw=0x%04X",
                        pmbus_drv_get_vout_mode_format_name(vout_mode),
                        (unsigned int)raw_word);
                }
                else
                {
                    PMBUS_DEBUG_PRINT("value_raw=0x%04X | mode=%s | special/invalid IEEE value",
                        (unsigned int)raw_word,
                        pmbus_drv_get_vout_mode_format_name(vout_mode));
                }
            }
            else
            {
                PMBUS_DEBUG_PRINT("value_raw=0x%04X | mode=%s | param=0x%02X | rel=%u",
                    (unsigned int)raw_word,
                    pmbus_drv_get_vout_mode_format_name(vout_mode),
                    (unsigned int)(vout_mode & PMBUS_VOUT_MODE_PARAMETER_MASK),
                    (unsigned int)(((vout_mode & PMBUS_VOUT_MODE_RELATIVE_MASK) != 0U) ? 1U : 0U));
            }
            break;

        case PMBUS_COMMAND_PMBUS_REVISION:
            PMBUS_DEBUG_PRINT("value=0x%02X | part1=1.%u | part2=1.%u",
                (unsigned int)snapshot->raw[0],
                (unsigned int)((snapshot->raw[0] >> 4) & 0x0FU),
                (unsigned int)(snapshot->raw[0] & 0x0FU));
            break;

        case PMBUS_COMMAND_MFR_ID:
        case PMBUS_COMMAND_MFR_MODEL:
        case PMBUS_COMMAND_MFR_REVISION:
        case PMBUS_COMMAND_MFR_SERIAL:
            if (snapshot->raw_len > 0U)
            {
                block_length = snapshot->raw[0];
                if ((uint8_t)(block_length + 1U) > snapshot->raw_len)
                {
                    block_length = (uint8_t)(snapshot->raw_len - 1U);
                }

                PMBUS_DEBUG_PRINT("value=\"");
                for (index = 0U; index < block_length; index++)
                {
                    PMBUS_DEBUG_PRINT("%c", snapshot->raw[index + 1U]);
                }
                PMBUS_DEBUG_PRINT("\" | raw=");

                print_length = snapshot->raw_len;
                if (snapshot->pec_present != 0U)
                {
                    print_length = (uint8_t)(print_length - 1U);
                }

                for (index = 0U; index < print_length; index++)
                {
                    PMBUS_DEBUG_PRINT("%02X", (unsigned int)snapshot->raw[index]);
                    if ((index + 1U) < print_length)
                    {
                        PMBUS_DEBUG_PRINT(" ");
                    }
                }
            }
            break;

        default:
            PMBUS_DEBUG_PRINT("raw=");
            for (index = 0U; index < snapshot->raw_len; index++)
            {
                PMBUS_DEBUG_PRINT("%02X", (unsigned int)snapshot->raw[index]);
                if ((index + 1U) < snapshot->raw_len)
                {
                    PMBUS_DEBUG_PRINT(" ");
                }
            }
            break;
    }

    crc_valid = pmbus_drv_compute_tx_pec(snapshot, &crc);
    if ((snapshot->pec_present != 0U) && (snapshot->raw_len > 0U) && (crc_valid != 0U))
    {
        pec_value = snapshot->raw[snapshot->raw_len - 1U];
        if (pec_value == crc)
        {
            PMBUS_DEBUG_PRINT(" | PEC OK | PEC(tx=0x%02X, calc=0x%02X)", (unsigned int)pec_value, (unsigned int)crc);
        }
        else
        {
            PMBUS_DEBUG_PRINT(" | PEC FAIL | PEC(tx=0x%02X, calc=0x%02X)", (unsigned int)pec_value, (unsigned int)crc);
        }
    }

    PMBUS_DEBUG_PRINT("\r\n");
#else
    snapshot = snapshot;
#endif
}

static void pmbus_drv_reset_tx(void)
{
    g_tx_length = 0U;
    g_tx_index = 0U;
    g_last_read_used_pec = 0U;
}

static void pmbus_drv_reset_rx(void)
{
    g_rx_length = 0U;
}

static void pmbus_drv_set_active_address(uint8_t address_7bit)
{
    g_current_slave_address_7bit = address_7bit;
    pmbus_io_i2c_slave_open(PMBUS_ADDRESS_7BIT_TO_WRITE(address_7bit));
}

static void pmbus_drv_configure_alias_addresses(void)
{
#if PMBUS_ENABLE_ARA_ALIAS
    if ((pmbus_app_is_alert_asserted() != 0U) && (g_ara_alias_inhibit == 0U))
    {
        pmbus_io_i2c_slave_set_alias(PMBUS_I2C_ALIAS_SLOT_ARA, PMBUS_ALERT_RESPONSE_ADDRESS_7BIT, Enable);
    }
    else
    {
        pmbus_io_i2c_slave_set_alias(PMBUS_I2C_ALIAS_SLOT_ARA, PMBUS_ALERT_RESPONSE_ADDRESS_7BIT, Disable);
    }
#endif

#if PMBUS_ENABLE_ARP
    pmbus_io_i2c_slave_set_alias(PMBUS_I2C_ALIAS_SLOT_ARP, PMBUS_ARP_DEFAULT_ADDRESS_7BIT, Enable);
#endif

#if PMBUS_ENABLE_ZONE_ALIAS
    pmbus_io_i2c_slave_set_alias(PMBUS_I2C_ALIAS_SLOT_ZONE_READ, PMBUS_ZONE_READ_ADDRESS_7BIT, Enable);
    pmbus_io_i2c_slave_set_alias(PMBUS_I2C_ALIAS_SLOT_ZONE_WRITE, PMBUS_ZONE_WRITE_ADDRESS_7BIT, Enable);
#endif
}

static void pmbus_drv_restore_normal_address(void)
{
    g_ara_alias_active = 0U;
    pmbus_drv_set_active_address(pmbus_app_get_slave_address_7bit());
    pmbus_drv_configure_alias_addresses();
}

static void pmbus_drv_update_request_target(void)
{
    uint8_t address_7bit;

    address_7bit = pmbus_io_i2c_get_received_address();
    if (address_7bit == 0U)
    {
        address_7bit = pmbus_app_get_slave_address_7bit();
    }

    g_request_address_7bit = address_7bit;
    g_request_target = PMBUS_REQUEST_TARGET_NORMAL;

    if (address_7bit == PMBUS_ALERT_RESPONSE_ADDRESS_7BIT)
    {
        g_request_target = PMBUS_REQUEST_TARGET_ARA;
    }
    else if (address_7bit == PMBUS_ARP_DEFAULT_ADDRESS_7BIT)
    {
        g_request_target = PMBUS_REQUEST_TARGET_ARP;
    }
    else if (address_7bit == PMBUS_ZONE_READ_ADDRESS_7BIT)
    {
        g_request_target = PMBUS_REQUEST_TARGET_ZONE_READ;
    }
    else if (address_7bit == PMBUS_ZONE_WRITE_ADDRESS_7BIT)
    {
        g_request_target = PMBUS_REQUEST_TARGET_ZONE_WRITE;
    }
}

static void pmbus_drv_save_pending_request_context(void)
{
    g_pending_request_address_7bit = g_request_address_7bit;
    g_pending_request_target = g_request_target;
}

static void pmbus_drv_restore_pending_request_context(void)
{
    g_request_address_7bit = g_pending_request_address_7bit;
    g_request_target = g_pending_request_target;
}

static void pmbus_drv_prepare_ara_response(void)
{
    uint8_t response_address;
    uint8_t crc;

    response_address = PMBUS_ADDRESS_7BIT_TO_WRITE(pmbus_app_get_slave_address_7bit());
    crc = 0U;
    crc = pmbus_pec_update(crc, PMBUS_ADDRESS_7BIT_TO_READ(PMBUS_ALERT_RESPONSE_ADDRESS_7BIT));
    crc = pmbus_pec_update(crc, response_address);

    /*
        SMBus ARA returns the alerting device address. Provide a second byte
        containing PEC so hosts may read either one byte without PEC or two
        bytes with PEC before NACK/STOP releases the alias.
    */
    g_tx_buffer[0] = response_address;
    g_tx_buffer[1] = crc;
    g_tx_length = 2U;
    g_tx_index = 0U;
}

static void pmbus_drv_build_arp_udid(void)
{
    uint8_t index;

    for (index = 0U; index < PMBUS_ARP_UDID_LENGTH; index++)
    {
        g_arp_udid[index] = 0x00U;
    }

    /*
        Product UDID fields must be bound to production data before release.
        The final byte carries the current PMBus slave write address so ARP
        validation can confirm address ownership.
    */
    g_arp_udid[0] = 0x01U;
    g_arp_udid[1] = 0x13U;
    g_arp_udid[14] = (uint8_t)(g_arp_address_resolved & 0x01U);
    g_arp_udid[15] = pmbus_app_get_address_valid();
    g_arp_udid[16] = PMBUS_ADDRESS_7BIT_TO_WRITE(pmbus_app_get_slave_address_7bit());
}

static void pmbus_drv_prepare_arp_response(void)
{
    uint8_t index;
    uint8_t command_length;

    pmbus_drv_build_arp_udid();
    g_tx_buffer[0] = PMBUS_ARP_UDID_LENGTH;
    for (index = 0U; index < PMBUS_ARP_UDID_LENGTH; index++)
    {
        g_tx_buffer[(uint8_t)(index + 1U)] = g_arp_udid[index];
    }

    g_tx_length = (uint8_t)(PMBUS_ARP_UDID_LENGTH + 1U);
    g_tx_index = 0U;
    command_length = g_rx_length;
    if (command_length > 0U)
    {
        g_last_read_used_pec = pmbus_drv_should_append_read_pec(1U);
        pmbus_drv_append_tx_pec(command_length);
    }

    pmbus_drv_queue_event(PMBUS_DEBUG_EVENT_ARP, g_arp_last_command, g_tx_length);
}

static void pmbus_drv_process_arp_frame(uint8_t repeated_start)
{
    uint8_t command;
    uint8_t assigned_address_7bit;

    if (g_rx_length == 0U)
    {
        pmbus_drv_reset_tx();
        return;
    }

    command = g_rx_buffer[0];
    g_arp_last_command = command;

    if (repeated_start != 0U)
    {
        if ((command == PMBUS_ARP_COMMAND_GET_UDID) ||
            (command == PMBUS_ARP_COMMAND_DIRECTED_GET_UDID))
        {
            pmbus_drv_prepare_arp_response();
            return;
        }

        g_tx_buffer[0] = 0x00U;
        g_tx_length = 1U;
        g_tx_index = 0U;
        return;
    }

    switch (command)
    {
        case PMBUS_ARP_COMMAND_RESET_DEVICE:
            g_arp_prepared = 0U;
            g_arp_address_resolved = 0U;
            pmbus_drv_queue_event(PMBUS_DEBUG_EVENT_ARP, command, 0U);
            break;

        case PMBUS_ARP_COMMAND_PREPARE_TO_ARP:
            g_arp_prepared = 1U;
            g_arp_address_resolved = 0U;
            pmbus_drv_queue_event(PMBUS_DEBUG_EVENT_ARP, command, 1U);
            break;

        case PMBUS_ARP_COMMAND_ASSIGN_ADDRESS:
            if (g_rx_length >= 2U)
            {
                assigned_address_7bit = (uint8_t)((g_rx_buffer[(uint8_t)(g_rx_length - 1U)] >> 1) & 0x7FU);
                if ((assigned_address_7bit >= 0x08U) && (assigned_address_7bit <= 0x77U))
                {
                    g_pending_slave_address_7bit = assigned_address_7bit;
                    g_address_change_pending = 1U;
                    g_arp_address_resolved = 1U;
                    pmbus_drv_queue_event(PMBUS_DEBUG_EVENT_ARP, command, assigned_address_7bit);
                }
            }
            break;

        default:
            pmbus_app_set_status_cml(PMBUS_STATUS_CML_INVALID_OR_UNSUPPORTED_COMMAND_RECEIVED);
            pmbus_drv_queue_event(PMBUS_DEBUG_EVENT_ARP, command, 0xFFU);
            break;
    }
}

static void pmbus_drv_prepare_zone_read_response(void)
{
    uint16_t zone_config;
    uint16_t zone_active;

    zone_config = pmbus_app_get_zone_config();
    zone_active = pmbus_app_get_zone_active();

    /*
        Portable Zone Read alias payload:
        count, ZONE_CONFIG LSB/MSB, ZONE_ACTIVE LSB/MSB, STATUS_BYTE.
        Product-specific zone group behavior can extend this block later.
    */
    g_tx_buffer[0] = 5U;
    g_tx_buffer[1] = (uint8_t)(zone_config & 0x00FFU);
    g_tx_buffer[2] = (uint8_t)((zone_config >> 8) & 0x00FFU);
    g_tx_buffer[3] = (uint8_t)(zone_active & 0x00FFU);
    g_tx_buffer[4] = (uint8_t)((zone_active >> 8) & 0x00FFU);
    g_tx_buffer[5] = pmbus_app_get_status_byte();
    g_tx_length = 6U;
    g_tx_index = 0U;
    g_last_read_used_pec = pmbus_drv_should_append_read_pec(1U);
    pmbus_drv_append_read_only_tx_pec(PMBUS_ZONE_READ_ADDRESS_7BIT);
    pmbus_drv_queue_event(PMBUS_DEBUG_EVENT_ZONE, PMBUS_ZONE_READ_ADDRESS_7BIT, g_tx_length);
}

static void pmbus_drv_process_zone_write_frame(uint8_t repeated_start)
{
    uint16_t value;

    if (repeated_start != 0U)
    {
        pmbus_drv_prepare_zone_read_response();
        return;
    }

    if (g_rx_length >= 3U)
    {
        value = (uint16_t)g_rx_buffer[1];
        value = (uint16_t)(value | ((uint16_t)g_rx_buffer[2] << 8));

        if (g_rx_buffer[0] == PMBUS_COMMAND_ZONE_CONFIG)
        {
            pmbus_app_set_zone_config(value);
            pmbus_drv_queue_event(PMBUS_DEBUG_EVENT_ZONE, PMBUS_COMMAND_ZONE_CONFIG, (uint8_t)(value & 0x00FFU));
        }
        else if (g_rx_buffer[0] == PMBUS_COMMAND_ZONE_ACTIVE)
        {
            pmbus_app_set_zone_active(value);
            pmbus_drv_queue_event(PMBUS_DEBUG_EVENT_ZONE, PMBUS_COMMAND_ZONE_ACTIVE, (uint8_t)(value & 0x00FFU));
        }
        else
        {
            pmbus_app_set_status_cml(PMBUS_STATUS_CML_INVALID_OR_UNSUPPORTED_COMMAND_RECEIVED);
            pmbus_drv_queue_event(PMBUS_DEBUG_EVENT_ZONE, g_rx_buffer[0], 0xFFU);
        }
    }
    else
    {
        pmbus_app_set_status_cml(PMBUS_STATUS_CML_INVALID_OR_UNSUPPORTED_DATA_RECEIVED);
        pmbus_drv_queue_event(PMBUS_DEBUG_EVENT_ZONE, 0x00U, 0xFEU);
    }
}

static void pmbus_drv_update_ara_alias_state(void)
{
#if PMBUS_ENABLE_ARA_ALIAS
    if ((g_recover_pending != 0U) || (g_recover_state != PMBUS_RECOVER_STATE_IDLE))
    {
        if (g_ara_alias_active != 0U)
        {
#if PMBUS_I2C_ALIAS_SLOT_ARA == PMBUS_I2C_ALIAS_SLOT_DISABLED
            pmbus_drv_restore_normal_address();
#else
            g_ara_alias_active = 0U;
            pmbus_drv_configure_alias_addresses();
#endif
            pmbus_drv_queue_event(PMBUS_DEBUG_EVENT_ARA_ALIAS, g_current_slave_address_7bit, 0U);
        }
        return;
    }

    if (pmbus_app_is_alert_asserted() != 0U)
    {
        if ((g_ara_alias_active == 0U) && (g_ara_alias_inhibit == 0U))
        {
            g_ara_alias_active = 1U;
#if PMBUS_I2C_ALIAS_SLOT_ARA == PMBUS_I2C_ALIAS_SLOT_DISABLED
            pmbus_drv_set_active_address(PMBUS_ALERT_RESPONSE_ADDRESS_7BIT);
#else
            pmbus_drv_configure_alias_addresses();
#endif
            pmbus_drv_queue_event(PMBUS_DEBUG_EVENT_ARA_ALIAS, PMBUS_ALERT_RESPONSE_ADDRESS_7BIT, 1U);
        }
    }
    else
    {
        g_ara_alias_inhibit = 0U;
        if (g_ara_alias_active != 0U)
        {
#if PMBUS_I2C_ALIAS_SLOT_ARA == PMBUS_I2C_ALIAS_SLOT_DISABLED
            pmbus_drv_restore_normal_address();
#else
            g_ara_alias_active = 0U;
            pmbus_drv_configure_alias_addresses();
#endif
            pmbus_drv_queue_event(PMBUS_DEBUG_EVENT_ARA_ALIAS, g_current_slave_address_7bit, 0U);
        }
    }
#endif
}

static uint8_t pmbus_drv_bus_lines_released(void)
{
    if ((pmbus_io_read_scl() != 0U) && (pmbus_io_read_sda() != 0U))
    {
        return 1U;
    }

    return 0U;
}

static uint8_t pmbus_drv_bus_clear(void)
{
#if PMBUS_ENABLE_SLAVE_RECOVER
    uint8_t pulse_count;

    pmbus_io_drive_sda_high();

    if (pmbus_drv_bus_lines_released() != 0U)
    {
        return 1U;
    }

    if (pmbus_io_read_sda() == 0U)
    {
        for (pulse_count = 0U; pulse_count < PMBUS_I2C_BUS_CLEAR_PULSES; pulse_count++)
        {
            pmbus_io_drive_scl_high();
            pmbus_io_drive_scl_low();

            if (pmbus_drv_bus_lines_released() != 0U)
            {
                break;
            }
        }
    }

    pmbus_io_drive_scl_high();
    pmbus_io_drive_sda_high();
    return pmbus_drv_bus_lines_released();
#else
    return 1U;
#endif
}

static void pmbus_drv_set_recover_pending(uint8_t reason)
{
    g_recover_reason = reason;
    g_recover_pending = 1U;
    if (g_recover_state == PMBUS_RECOVER_STATE_IDLE)
    {
        g_recover_state = PMBUS_RECOVER_STATE_PENDING;
        g_recover_attempt_count = 0U;
        g_recover_backoff_count = 0U;
    }
}

static void pmbus_drv_reset_clock_low_monitor(void)
{
    g_software_scl_low_ms = 0U;
    g_software_scl_low_state = 0U;
}

static void pmbus_drv_check_clock_low_timeout_1ms(void)
{
#if PMBUS_ENABLE_SLAVE_RECOVER
    if (pmbus_io_read_scl() != 0U)
    {
        pmbus_drv_reset_clock_low_monitor();
        return;
    }

    if (g_software_scl_low_state == 0U)
    {
        g_software_scl_low_ms = 1U;
        g_software_scl_low_state = 1U;
    }
    else if (g_software_scl_low_state == 1U)
    {
        if (g_software_scl_low_ms < 0xFFFFU)
        {
            g_software_scl_low_ms++;
        }

        if (g_software_scl_low_ms >= PMBUS_I2C_CLOCK_LOW_TIMEOUT_MS)
        {
            g_software_scl_low_state = 2U;
            g_timeout_fault_count = (uint8_t)(g_timeout_fault_count + 1U);
            pmbus_app_set_status_cml(PMBUS_STATUS_CML_OTHER_COMMUNICATION_FAULT);
            pmbus_drv_queue_event(PMBUS_DEBUG_EVENT_CLOCK_LOW_TIMEOUT,
                                  (uint8_t)(g_software_scl_low_ms & 0x00FFU),
                                  (uint8_t)((g_software_scl_low_ms >> 8) & 0x00FFU));

            if (g_timeout_fault_count >= PMBUS_I2C_TIMEOUT_RECOVER_THRESHOLD)
            {
                pmbus_drv_set_recover_pending(PMBUS_RECOVER_REASON_TIMEOUT);
                g_timeout_fault_count = 0U;
            }
        }
    }
    else
    {
        /* Already latched. Wait for SCL high or background recovery. */
    }
#else
    pmbus_drv_reset_clock_low_monitor();
#endif
}

static uint8_t pmbus_drv_recover_bus(void)
{
#if PMBUS_ENABLE_SLAVE_RECOVER
    uint8_t attempt_count;
    uint8_t bus_released;

    pmbus_app_set_busy_state(1U);
    pmbus_io_i2c_interrupt(Disable);
    pmbus_io_i2c_timeout(Disable);
    pmbus_io_i2c_disable();
    pmbus_io_drive_sda_high();
    pmbus_io_drive_scl_high();
    pmbus_drv_reset_rx();
    pmbus_drv_reset_tx();
    g_write_frame_pending = 0U;
    g_write_frame_pending_tick = 0UL;
    pmbus_drv_reset_clock_low_monitor();
    g_request_address_7bit = pmbus_app_get_slave_address_7bit();
    g_request_target = PMBUS_REQUEST_TARGET_NORMAL;
    g_pending_request_address_7bit = g_request_address_7bit;
    g_pending_request_target = PMBUS_REQUEST_TARGET_NORMAL;
    bus_released = 0U;
    g_tx_finish_bus_error_guard = 0U;

    for (attempt_count = 0U; attempt_count < PMBUS_I2C_BUS_CLEAR_RETRY_COUNT; attempt_count++)
    {
        if (pmbus_drv_bus_clear() != 0U)
        {
            bus_released = 1U;
            break;
        }
    }

    if (bus_released != 0U)
    {
#if PMBUS_I2C_ALIAS_SLOT_ARA == PMBUS_I2C_ALIAS_SLOT_DISABLED
        if (g_ara_alias_active != 0U)
        {
            pmbus_drv_set_active_address(PMBUS_ALERT_RESPONSE_ADDRESS_7BIT);
        }
        else
        {
            pmbus_drv_set_active_address(pmbus_app_get_slave_address_7bit());
        }
#else
        pmbus_drv_set_active_address(pmbus_app_get_slave_address_7bit());
        pmbus_drv_configure_alias_addresses();
#endif

        pmbus_io_i2c_clear_timeout_flag();
        /*
            Keep the hardware timeout counter disabled while the slave is idle.
            It is enabled only during active read-response transmit states
            (SLA+R / DATA_TX_ACK) so an idle bus does not self-trigger
            repeated timeout recovery with status 0xF8.
        */
        pmbus_io_i2c_timeout(Disable);
        pmbus_io_i2c_interrupt(Enable);
    }

    pmbus_app_set_busy_state(0U);
    return bus_released;
#else
    return 1U;
#endif
}

static void pmbus_drv_append_tx_pec(uint8_t command_length)
{
#if PMBUS_ENABLE_PEC
    uint8_t crc;
    uint8_t index;
    uint8_t device_write_address;
    uint8_t device_read_address;

    if (g_last_read_used_pec == 0U)
    {
        return;
    }

    if (g_tx_length >= PMBUS_TX_BUFFER_SIZE)
    {
        return;
    }

    device_write_address = PMBUS_ADDRESS_7BIT_TO_WRITE(g_request_address_7bit);
    device_read_address = PMBUS_ADDRESS_7BIT_TO_READ(g_request_address_7bit);

    crc = 0U;
    crc = pmbus_pec_update(crc, device_write_address);

    for (index = 0U; index < command_length; index++)
    {
        crc = pmbus_pec_update(crc, g_rx_buffer[index]);
    }

    crc = pmbus_pec_update(crc, device_read_address);

    for (index = 0U; index < g_tx_length; index++)
    {
        crc = pmbus_pec_update(crc, g_tx_buffer[index]);
    }

    g_tx_buffer[g_tx_length] = crc;
    g_tx_length = (uint8_t)(g_tx_length + 1U);
#else
    command_length = command_length;
#endif
}

static void pmbus_drv_append_read_only_tx_pec(uint8_t address_7bit)
{
#if PMBUS_ENABLE_PEC
    uint8_t crc;
    uint8_t index;

    if (g_last_read_used_pec == 0U)
    {
        return;
    }

    if (g_tx_length >= PMBUS_TX_BUFFER_SIZE)
    {
        return;
    }

    crc = 0U;
    crc = pmbus_pec_update(crc, PMBUS_ADDRESS_7BIT_TO_READ(address_7bit));
    for (index = 0U; index < g_tx_length; index++)
    {
        crc = pmbus_pec_update(crc, g_tx_buffer[index]);
    }

    g_tx_buffer[g_tx_length] = crc;
    g_tx_length = (uint8_t)(g_tx_length + 1U);
#else
    address_7bit = address_7bit;
#endif
}

static uint8_t pmbus_drv_should_append_read_pec(uint8_t repeated_start)
{
#if PMBUS_ENABLE_PEC
    if (repeated_start != 0U)
    {
        return 1U;
    }
#else
    repeated_start = repeated_start;
#endif

    return 0U;
}

static void pmbus_drv_load_next_tx_byte(void)
{
    uint8_t next_byte;

    if (g_tx_index < g_tx_length)
    {
        next_byte = g_tx_buffer[g_tx_index];
        g_tx_index = (uint8_t)(g_tx_index + 1U);
    }
    else
    {
        next_byte = 0x00U;
    }

    pmbus_io_i2c_write_data(next_byte);
}

static uint8_t pmbus_drv_compute_write_pec(uint8_t frame_length_without_pec)
{
    uint8_t crc;
    uint8_t index;
    uint8_t device_write_address;

    device_write_address = PMBUS_ADDRESS_7BIT_TO_WRITE(g_request_address_7bit);
    crc = 0U;
    crc = pmbus_pec_update(crc, device_write_address);

    for (index = 0U; index < frame_length_without_pec; index++)
    {
        crc = pmbus_pec_update(crc, g_rx_buffer[index]);
    }

    return crc;
}

static pmbus_frame_class_t pmbus_drv_classify_current_frame(void)
{
    pmbus_dispatch_protocol_t protocol_no_pec;
    pmbus_dispatch_protocol_t protocol_with_pec;
    pmbus_dispatch_protocol_t read_protocol_no_pec;
    pmbus_dispatch_protocol_t read_protocol_with_pec;
    uint8_t command;
    uint8_t data_len;
    uint8_t candidate_length;
    uint8_t has_write_path;
    uint8_t has_read_path;

    if (g_rx_length == 0U)
    {
        return PMBUS_FRAME_CLASS_NONE;
    }

    command = g_rx_buffer[0];
    data_len = (uint8_t)(g_rx_length - 1U);
    has_write_path = 0U;
    has_read_path = 0U;

    protocol_no_pec = pmbus_dispatch_detect_protocol(command, data_len, &g_rx_buffer[1], 0U);
    if (protocol_no_pec != PMBUS_PROTOCOL_UNKNOWN)
    {
        has_write_path = 1U;
    }

    read_protocol_no_pec = pmbus_dispatch_detect_protocol(command, data_len, &g_rx_buffer[1], 1U);
    if (read_protocol_no_pec != PMBUS_PROTOCOL_UNKNOWN)
    {
        has_read_path = 1U;
    }

    if (data_len > 0U)
    {
        candidate_length = (uint8_t)(data_len - 1U);

        protocol_with_pec = pmbus_dispatch_detect_protocol(command, candidate_length, &g_rx_buffer[1], 0U);
        if (protocol_with_pec != PMBUS_PROTOCOL_UNKNOWN)
        {
            has_write_path = 1U;
        }

        read_protocol_with_pec = pmbus_dispatch_detect_protocol(command, candidate_length, &g_rx_buffer[1], 1U);
        if (read_protocol_with_pec != PMBUS_PROTOCOL_UNKNOWN)
        {
            has_read_path = 1U;
        }
    }

    if ((has_write_path != 0U) && (has_read_path != 0U))
    {
        return PMBUS_FRAME_CLASS_AMBIGUOUS;
    }

    if (has_read_path != 0U)
    {
        return PMBUS_FRAME_CLASS_READ_ONLY;
    }

    if (has_write_path != 0U)
    {
        return PMBUS_FRAME_CLASS_WRITE_ONLY;
    }

    return PMBUS_FRAME_CLASS_NONE;
}

static void pmbus_drv_prepare_default_read(void)
{
    pmbus_dispatch_transaction_t *transaction;
    uint8_t tx_length;

    transaction = &g_dispatch_transaction;
    tx_length = 0U;

    if (g_request_target == PMBUS_REQUEST_TARGET_ARA)
    {
        pmbus_drv_prepare_ara_response();
        return;
    }

    if (g_request_target == PMBUS_REQUEST_TARGET_ARP)
    {
        pmbus_drv_prepare_arp_response();
        return;
    }

    if (g_request_target == PMBUS_REQUEST_TARGET_ZONE_READ)
    {
        pmbus_drv_prepare_zone_read_response();
        return;
    }

    if (g_last_command_valid != 0U)
    {
        transaction->command = g_last_command;
        transaction->data_len = 0U;
        transaction->repeated_start = 1U;
        transaction->pec_present = 0U;
        transaction->pec_valid = 1U;
        transaction->protocol = pmbus_dispatch_detect_protocol(transaction->command, 0U, transaction->payload, 1U);

        if (transaction->protocol != PMBUS_PROTOCOL_UNKNOWN)
        {
            (void)pmbus_dispatch_execute(transaction, g_tx_buffer, &tx_length);
        }
        else
        {
            pmbus_dispatch_prepare_error_response(transaction->command, g_tx_buffer, &tx_length);
        }
    }
    else
    {
        g_tx_buffer[0] = 0x00U;
        tx_length = 1U;
    }

    g_tx_length = tx_length;
    g_tx_index = 0U;
}

static void pmbus_drv_process_frame(uint8_t repeated_start)
{
    pmbus_dispatch_transaction_t *transaction;
    pmbus_dispatch_protocol_t protocol_no_pec;
    pmbus_dispatch_protocol_t protocol_with_pec;
    uint8_t last_byte;
    uint8_t computed_pec;
    uint8_t candidate_length;
    uint8_t copy_index;
    uint8_t used_pec;
    uint8_t valid_pec;
    uint8_t tx_length;

    transaction = &g_dispatch_transaction;

    if (g_request_target == PMBUS_REQUEST_TARGET_ARA)
    {
        pmbus_drv_prepare_ara_response();
        g_write_frame_pending = 0U;
        g_write_frame_pending_tick = 0UL;
        return;
    }

    if (g_request_target == PMBUS_REQUEST_TARGET_ARP)
    {
        pmbus_drv_process_arp_frame(repeated_start);
        g_write_frame_pending = 0U;
        g_write_frame_pending_tick = 0UL;
        return;
    }

    if (g_request_target == PMBUS_REQUEST_TARGET_ZONE_READ)
    {
        pmbus_drv_prepare_zone_read_response();
        g_write_frame_pending = 0U;
        g_write_frame_pending_tick = 0UL;
        return;
    }

    if (g_request_target == PMBUS_REQUEST_TARGET_ZONE_WRITE)
    {
        pmbus_drv_process_zone_write_frame(repeated_start);
        g_write_frame_pending = 0U;
        g_write_frame_pending_tick = 0UL;
        return;
    }

    if (g_rx_length == 0U)
    {
        pmbus_drv_reset_tx();
        g_write_frame_pending = 0U;
        g_write_frame_pending_tick = 0UL;
        return;
    }

    pmbus_app_set_busy_state(1U);
    transaction->command = g_rx_buffer[0];
    transaction->repeated_start = repeated_start;
    transaction->pec_present = 0U;
    transaction->pec_valid = 1U;
    transaction->data_len = (uint8_t)(g_rx_length - 1U);

    protocol_no_pec = pmbus_dispatch_detect_protocol(transaction->command, transaction->data_len, &g_rx_buffer[1], repeated_start);
    protocol_with_pec = PMBUS_PROTOCOL_UNKNOWN;
    used_pec = 0U;
    valid_pec = 1U;

#if PMBUS_ENABLE_PEC
    if (transaction->data_len > 0U)
    {
        candidate_length = (uint8_t)(transaction->data_len - 1U);
        protocol_with_pec = pmbus_dispatch_detect_protocol(transaction->command, candidate_length, &g_rx_buffer[1], repeated_start);

        if (protocol_with_pec != PMBUS_PROTOCOL_UNKNOWN)
        {
            last_byte = g_rx_buffer[g_rx_length - 1U];
            computed_pec = pmbus_drv_compute_write_pec((uint8_t)(g_rx_length - 1U));

            if (last_byte == computed_pec)
            {
                transaction->data_len = candidate_length;
                used_pec = 1U;
                valid_pec = 1U;
            }
            else
            {
                if ((protocol_no_pec == PMBUS_PROTOCOL_UNKNOWN) ||
                    ((repeated_start == 0U) && (PMBUS_PEC_POLICY == PMBUS_PEC_POLICY_REQUIRED)))
                {
                    transaction->data_len = candidate_length;
                    used_pec = 1U;
                    valid_pec = 0U;
                }
            }
        }
    }

    if ((repeated_start == 0U) &&
        (PMBUS_PEC_POLICY == PMBUS_PEC_POLICY_REQUIRED) &&
        (used_pec == 0U))
    {
        valid_pec = 0U;
    }
#else
    last_byte = 0U;
    computed_pec = 0U;
    candidate_length = 0U;
#endif

    transaction->pec_present = used_pec;
    transaction->pec_valid = valid_pec;
    transaction->protocol = pmbus_dispatch_detect_protocol(transaction->command, transaction->data_len, &g_rx_buffer[1], repeated_start);

    if (transaction->data_len > (PMBUS_MAX_BLOCK_SIZE + 1U))
    {
        pmbus_app_set_status_cml(PMBUS_STATUS_CML_INVALID_OR_UNSUPPORTED_DATA_RECEIVED);
        pmbus_dispatch_prepare_error_response(transaction->command, g_tx_buffer, &tx_length);
        g_tx_length = tx_length;
        g_tx_index = 0U;
        pmbus_drv_queue_event(PMBUS_DEBUG_EVENT_RX_OVERFLOW, transaction->command, transaction->data_len);
        g_write_frame_pending = 0U;
        g_write_frame_pending_tick = 0UL;
        pmbus_app_set_busy_state(0U);
        return;
    }

    for (copy_index = 0U; copy_index < transaction->data_len; copy_index++)
    {
        transaction->payload[copy_index] = g_rx_buffer[copy_index + 1U];
    }

    pmbus_drv_capture_debug_frame(g_rx_length, transaction);

    g_last_command = transaction->command;
    g_last_command_valid = 1U;
    g_last_read_used_pec = 0U;
    tx_length = 0U;

    if (valid_pec == 0U)
    {
        pmbus_app_set_status_cml(PMBUS_STATUS_CML_PACKET_ERROR_CHECK_FAILED);
        pmbus_dispatch_prepare_error_response(transaction->command, g_tx_buffer, &tx_length);
        g_tx_length = tx_length;
        g_tx_index = 0U;
        pmbus_drv_queue_event(PMBUS_DEBUG_EVENT_PEC_ERROR, transaction->command, g_rx_length);
        g_write_frame_pending = 0U;
        g_write_frame_pending_tick = 0UL;
        pmbus_app_set_busy_state(0U);
        return;
    }

    if (transaction->protocol == PMBUS_PROTOCOL_UNKNOWN)
    {
        if (pmbus_dispatch_is_known_command(transaction->command) != 0U)
        {
            pmbus_app_set_status_cml(PMBUS_STATUS_CML_INVALID_OR_UNSUPPORTED_DATA_RECEIVED);
        }
        else
        {
            pmbus_app_set_status_cml(PMBUS_STATUS_CML_INVALID_OR_UNSUPPORTED_COMMAND_RECEIVED);
        }
        pmbus_dispatch_prepare_error_response(transaction->command, g_tx_buffer, &tx_length);
        g_tx_length = tx_length;
        g_tx_index = 0U;
        pmbus_drv_queue_event(PMBUS_DEBUG_EVENT_UNSUPPORTED, transaction->command, g_rx_length);
        g_write_frame_pending = 0U;
        pmbus_app_set_busy_state(0U);
        return;
    }

    {
        uint8_t dispatch_result;
        uint8_t semantic_tx_length;

        dispatch_result = pmbus_dispatch_execute(transaction, g_tx_buffer, &tx_length);
        semantic_tx_length = tx_length;

        if (dispatch_result != 0U)
        {
            if (repeated_start != 0U)
            {
                pmbus_semantics_record_read_response(transaction->command,
                    (uint8_t)transaction->protocol,
                    g_tx_buffer,
                    semantic_tx_length,
                    0U);
            }
            else
            {
                pmbus_semantics_record_write(transaction->command,
                    (uint8_t)transaction->protocol,
                    transaction->payload,
                    transaction->data_len);
            }
        }
    }
    g_tx_length = tx_length;
    g_tx_index = 0U;

    if ((repeated_start != 0U) && (pmbus_drv_should_append_read_pec(repeated_start) != 0U))
    {
        g_last_read_used_pec = 1U;
        pmbus_drv_append_tx_pec((uint8_t)(g_rx_length - ((used_pec != 0U) ? 1U : 0U)));
    }

    if (repeated_start != 0U)
    {
        pmbus_drv_capture_debug_tx(transaction->command,
            (uint8_t)transaction->protocol,
            (uint8_t)(g_rx_length - ((used_pec != 0U) ? 1U : 0U)),
            g_last_read_used_pec);
    }
    else
    {
        pmbus_drv_queue_event(PMBUS_DEBUG_EVENT_WRITE_DONE, transaction->command, transaction->data_len);
    }

    g_write_frame_pending = 0U;
    g_write_frame_pending_tick = 0UL;
    pmbus_app_set_busy_state(0U);
}

void pmbus_drv_init(void)
{
    uint8_t index;

    pmbus_io_init_i2c_pins();
    pmbus_io_init_alert_pin();
    pmbus_pec_init();

    pmbus_app_init();
    pmbus_app_set_busy_state(1U);
    pmbus_app_release_alert();

    if (pmbus_app_get_address_valid() == 0U)
    {
        pmbus_app_set_status_cml(PMBUS_STATUS_CML_OTHER_MEMORY_OR_LOGIC_FAULT);
        pmbus_app_set_mfr_fault_state(1U);
    }

    g_current_slave_address_7bit = pmbus_app_get_slave_address_7bit();
    g_request_address_7bit = g_current_slave_address_7bit;
    g_request_target = PMBUS_REQUEST_TARGET_NORMAL;
    g_pending_request_address_7bit = g_current_slave_address_7bit;
    g_pending_request_target = PMBUS_REQUEST_TARGET_NORMAL;
    g_ara_alias_active = 0U;
    g_ara_alias_inhibit = 0U;
    g_arp_prepared = 0U;
    g_arp_address_resolved = 1U;
    g_arp_last_command = 0U;
    g_address_change_pending = 0U;
    g_pending_slave_address_7bit = g_current_slave_address_7bit;
    g_tx_finish_bus_error_guard = 0U;
    g_recover_pending = 0U;
    g_recover_reason = PMBUS_RECOVER_REASON_NONE;
    g_recover_state = PMBUS_RECOVER_STATE_IDLE;
    g_recover_attempt_count = 0U;
    g_recover_backoff_count = 0U;
    g_timeout_fault_count = 0U;
    pmbus_drv_reset_clock_low_monitor();
    g_software_scl_low_monitor_enabled = 0U;
    g_bus_error_fault_count = 0U;
    g_rx_overflow_fault_count = 0U;
    g_recover_count = 0U;
    g_recover_fail_count = 0U;

    pmbus_drv_reset_rx();
    pmbus_drv_reset_tx();
    g_write_frame_pending = 0U;
    g_write_frame_pending_tick = 0UL;
    g_last_command = 0U;
    g_last_command_valid = 0U;
    g_debug_head = 0U;
    g_debug_tail = 0U;
    g_debug_frame_head = 0U;
    g_debug_frame_tail = 0U;
    g_debug_tx_head = 0U;
    g_debug_tx_tail = 0U;
    pmbus_app_set_comm_recovery_state(PMBUS_RECOVER_REASON_NONE, 0U, 0U, 1U);

    for (index = 0U; index < PMBUS_RX_BUFFER_SIZE; index++)
    {
        g_rx_buffer[index] = 0x00U;
    }

    for (index = 0U; index < PMBUS_TX_BUFFER_SIZE; index++)
    {
        g_tx_buffer[index] = 0x00U;
    }

    pmbus_drv_set_active_address(g_current_slave_address_7bit);
    pmbus_drv_configure_alias_addresses();
    /*
        Do not arm the timeout counter at startup. The ISR enables it only
        for active slave-transmit phases that actually need stuck-bus
        detection.
    */
    pmbus_io_i2c_timeout(Disable);
    pmbus_io_i2c_clear_timeout_flag();
    pmbus_io_i2c_interrupt(Enable);
    pmbus_app_set_busy_state(0U);
    g_software_scl_low_monitor_enabled = 1U;
    pmbus_io_enable_global_interrupt();
}

void pmbus_drv_timer_1ms(void)
{
    if (g_software_scl_low_monitor_enabled == 0U)
    {
        return;
    }

    /*
        Match the generic SMBus slave timer path: guard shared timeout state
        with the I2C NVIC IRQ only. Do not toggle the I2C peripheral INTEN.
    */
    pmbus_io_i2c_irq_guard(Disable);
    pmbus_drv_check_clock_low_timeout_1ms();
    pmbus_io_i2c_irq_guard(Enable);
}

void pmbus_drv_background_task(void)
{
    pmbus_debug_event_t event;
    uint8_t recover_reason;
    uint8_t recover_success;

    pmbus_app_background_task();
    pmbus_semantics_background_task();
    pmbus_drv_update_ara_alias_state();

    if (g_address_change_pending != 0U)
    {
        pmbus_app_set_busy_state(1U);
        pmbus_io_i2c_interrupt(Disable);
        pmbus_app_set_slave_address_7bit(g_pending_slave_address_7bit);
        pmbus_drv_set_active_address(g_pending_slave_address_7bit);
        pmbus_drv_configure_alias_addresses();
        pmbus_io_i2c_clear_timeout_flag();
        pmbus_io_i2c_timeout(Disable);
        pmbus_io_i2c_interrupt(Enable);
        g_request_address_7bit = g_pending_slave_address_7bit;
        g_current_slave_address_7bit = g_pending_slave_address_7bit;
        g_address_change_pending = 0U;
        pmbus_app_set_busy_state(0U);
    }

    if (g_recover_pending != 0U)
    {
        if (g_recover_state == PMBUS_RECOVER_STATE_BACKOFF)
        {
            if (g_recover_backoff_count > 0U)
            {
                g_recover_backoff_count = (uint8_t)(g_recover_backoff_count - 1U);
            }

            if (g_recover_backoff_count == 0U)
            {
                g_recover_state = PMBUS_RECOVER_STATE_PENDING;
            }
        }
        else if (g_recover_state == PMBUS_RECOVER_STATE_STUCK_BUS)
        {
            if (g_recover_backoff_count > 0U)
            {
                g_recover_backoff_count = (uint8_t)(g_recover_backoff_count - 1U);
            }

            if (g_recover_backoff_count == 0U)
            {
                g_recover_state = PMBUS_RECOVER_STATE_PENDING;
            }
        }

        if (g_recover_state == PMBUS_RECOVER_STATE_PENDING)
        {
            recover_reason = g_recover_reason;
            recover_success = pmbus_drv_recover_bus();

            if (recover_success != 0U)
            {
                g_recover_count = (uint8_t)(g_recover_count + 1U);
                pmbus_app_set_comm_recovery_state(recover_reason, g_recover_count, g_recover_fail_count, 1U);
                g_recover_pending = 0U;
                g_recover_reason = PMBUS_RECOVER_REASON_NONE;
                g_recover_state = PMBUS_RECOVER_STATE_IDLE;
                g_recover_attempt_count = 0U;
                g_recover_backoff_count = 0U;
                pmbus_drv_queue_event(PMBUS_DEBUG_EVENT_RECOVER, g_current_slave_address_7bit, recover_reason);
            }
            else
            {
                g_recover_fail_count = (uint8_t)(g_recover_fail_count + 1U);
                pmbus_app_set_status_cml(PMBUS_STATUS_CML_OTHER_COMMUNICATION_FAULT);
                pmbus_app_set_mfr_fault_state(1U);
                pmbus_app_set_comm_recovery_state(recover_reason, g_recover_count, g_recover_fail_count, 0U);
                pmbus_drv_queue_event(PMBUS_DEBUG_EVENT_RECOVER_FAIL, g_current_slave_address_7bit, recover_reason);
                g_recover_attempt_count = (uint8_t)(g_recover_attempt_count + 1U);

                if (g_recover_attempt_count >= PMBUS_I2C_RECOVER_MAX_ATTEMPTS)
                {
                    g_recover_state = PMBUS_RECOVER_STATE_STUCK_BUS;
                    g_recover_backoff_count = PMBUS_I2C_STUCK_BUS_RETRY_CYCLES;
                    g_recover_attempt_count = 0U;
                }
                else
                {
                    g_recover_state = PMBUS_RECOVER_STATE_BACKOFF;
                    g_recover_backoff_count = PMBUS_I2C_RECOVER_BACKOFF_CYCLES;
                }
            }
        }
    }

#if PMBUS_DEBUG_ENABLE
    while (g_debug_tail != g_debug_head)
    {
        event = g_debug_queue[g_debug_tail];
        g_debug_tail = (uint8_t)(g_debug_tail + 1U);
        if (g_debug_tail >= PMBUS_DEBUG_QUEUE_SIZE)
        {
            g_debug_tail = 0U;
        }

        switch (event.event_id)
        {
            case PMBUS_DEBUG_EVENT_STATUS:
                #if PMBUS_DEBUG_PRINT_STATUS
                PMBUS_DEBUG_PRINT("I2C status 0x%02X\r\n", (unsigned int)event.value0);
                #endif
                break;

            case PMBUS_DEBUG_EVENT_RX_FRAME:
                #if PMBUS_DEBUG_PRINT_RX_FRAME
                {
                    pmbus_debug_frame_snapshot_t *snapshot;
                    uint8_t next_frame_tail;
                    uint8_t frame_index;

                    frame_index = event.value0;
                    if (frame_index < PMBUS_DEBUG_FRAME_QUEUE_SIZE)
                    {
                        snapshot = &g_debug_frame_queue[frame_index];
                        pmbus_drv_print_debug_frame(snapshot);
                        next_frame_tail = (uint8_t)(frame_index + 1U);
                        if (next_frame_tail >= PMBUS_DEBUG_FRAME_QUEUE_SIZE)
                        {
                            next_frame_tail = 0U;
                        }
                        g_debug_frame_tail = next_frame_tail;
                    }
                }
                #endif
                break;

            case PMBUS_DEBUG_EVENT_RX_OVERFLOW:
                PMBUS_DEBUG_PRINT("PMBus RX overflow, status 0x%02X\r\n", (unsigned int)event.value0);
                break;

            case PMBUS_DEBUG_EVENT_TX_READY:
                #if PMBUS_DEBUG_PRINT_TX_READY
                {
                    pmbus_debug_tx_snapshot_t *snapshot;
                    uint8_t next_tx_tail;
                    uint8_t tx_index;

                    tx_index = event.value0;
                    if (tx_index < PMBUS_DEBUG_TX_QUEUE_SIZE)
                    {
                        snapshot = &g_debug_tx_queue[tx_index];
                        #if PMBUS_DEBUG_PRINT_TX_DECODE
                        pmbus_drv_print_debug_tx(snapshot);
                        #else
                        PMBUS_DEBUG_PRINT("PMBus TX ready cmd=0x%02X len=%u\r\n",
                            (unsigned int)snapshot->command,
                            (unsigned int)snapshot->raw_len);
                        #endif
                        next_tx_tail = (uint8_t)(tx_index + 1U);
                        if (next_tx_tail >= PMBUS_DEBUG_TX_QUEUE_SIZE)
                        {
                            next_tx_tail = 0U;
                        }
                        g_debug_tx_tail = next_tx_tail;
                    }
                }
                #endif
                break;

            case PMBUS_DEBUG_EVENT_UNSUPPORTED:
                PMBUS_DEBUG_PRINT("PMBus unsupported cmd=0x%02X frame=%u\r\n", (unsigned int)event.value0, (unsigned int)event.value1);
                break;

            case PMBUS_DEBUG_EVENT_PEC_ERROR:
                PMBUS_DEBUG_PRINT("PMBus PEC error cmd=0x%02X frame=%u\r\n", (unsigned int)event.value0, (unsigned int)event.value1);
                break;

            case PMBUS_DEBUG_EVENT_WRITE_DONE:
                #if PMBUS_DEBUG_PRINT_WRITE_DONE
                PMBUS_DEBUG_PRINT("PMBus write done cmd=0x%02X len=%u\r\n", (unsigned int)event.value0, (unsigned int)event.value1);
                #endif
                break;

            case PMBUS_DEBUG_EVENT_RECOVER:
                PMBUS_DEBUG_PRINT("PMBus slave recover addr7=0x%02X reason=%u\r\n",
                    (unsigned int)event.value0,
                    (unsigned int)event.value1);
                break;

            case PMBUS_DEBUG_EVENT_RECOVER_FAIL:
                PMBUS_DEBUG_PRINT("PMBus slave recover fail addr7=0x%02X reason=%u\r\n",
                    (unsigned int)event.value0,
                    (unsigned int)event.value1);
                break;

            case PMBUS_DEBUG_EVENT_ARA_ALIAS:
                PMBUS_DEBUG_PRINT("PMBus ARA alias addr7=0x%02X state=%u\r\n", (unsigned int)event.value0, (unsigned int)event.value1);
                break;

            case PMBUS_DEBUG_EVENT_ARP:
                PMBUS_DEBUG_PRINT("PMBus ARP cmd=0x%02X value=0x%02X\r\n", (unsigned int)event.value0, (unsigned int)event.value1);
                break;

            case PMBUS_DEBUG_EVENT_ZONE:
                PMBUS_DEBUG_PRINT("PMBus Zone event=0x%02X value=0x%02X\r\n", (unsigned int)event.value0, (unsigned int)event.value1);
                break;

            case PMBUS_DEBUG_EVENT_CLOCK_LOW_TIMEOUT:
                PMBUS_DEBUG_PRINT("PMBus software SCL-low timeout ms=%u\r\n",
                    (unsigned int)((uint16_t)event.value0 | ((uint16_t)event.value1 << 8)));
                break;

            default:
                break;
        }
    }
#endif
}

PMBUS_PORT_I2C_ISR_PROTOTYPE
{
    uint8_t status;
    uint8_t saved_state;
    uint8_t timeout_seen;

    saved_state = pmbus_io_isr_enter();

    status = pmbus_io_i2c_get_status();
    timeout_seen = pmbus_io_i2c_timeout_flag();
    if (timeout_seen != 0U)
    {
        pmbus_io_i2c_clear_timeout_flag();
        pmbus_io_i2c_disable_timeout_counter();
    }

    pmbus_drv_queue_event(PMBUS_DEBUG_EVENT_STATUS, status, 0U);

    switch (status)
    {
            case PMBUS_I2C_STATUS_SLA_W_ACK:
                g_tx_finish_bus_error_guard = 0U;
                if (g_write_frame_pending != 0U)
                {
                    pmbus_drv_restore_pending_request_context();
                    pmbus_drv_process_frame(0U);
                    pmbus_drv_reset_rx();
                    g_write_frame_pending = 0U;
                    g_write_frame_pending_tick = 0UL;
                }
                pmbus_drv_update_request_target();
                pmbus_drv_reset_rx();
                pmbus_drv_reset_tx();
                g_write_frame_pending_tick = 0UL;
                pmbus_io_i2c_set_ack();
                break;

            case PMBUS_I2C_STATUS_DATA_RX_ACK:
                g_tx_finish_bus_error_guard = 0U;
                if (g_rx_length < PMBUS_RX_BUFFER_SIZE)
                {
                    g_rx_buffer[g_rx_length] = pmbus_io_i2c_read_data();
                    g_rx_length = (uint8_t)(g_rx_length + 1U);
                    pmbus_io_i2c_set_ack();
                }
                else
                {
                    pmbus_app_set_status_cml(PMBUS_STATUS_CML_OTHER_COMMUNICATION_FAULT);
                    g_rx_overflow_fault_count = (uint8_t)(g_rx_overflow_fault_count + 1U);
                    pmbus_drv_queue_event(PMBUS_DEBUG_EVENT_RX_OVERFLOW, status, g_rx_length);
                    if (g_rx_overflow_fault_count != 0U)
                    {
                        pmbus_drv_set_recover_pending(PMBUS_RECOVER_REASON_RX_OVERFLOW);
                        g_rx_overflow_fault_count = 0U;
                    }
                    pmbus_io_i2c_clear_ack();
                }
                break;

            case PMBUS_I2C_STATUS_DATA_RX_NACK:
                g_tx_finish_bus_error_guard = 0U;
                pmbus_app_set_status_cml(PMBUS_STATUS_CML_OTHER_COMMUNICATION_FAULT);
                pmbus_drv_reset_rx();
                g_write_frame_pending = 0U;
                g_write_frame_pending_tick = 0UL;
                pmbus_io_i2c_set_ack();
                break;

            case PMBUS_I2C_STATUS_STOP_RESTART:
                g_tx_finish_bus_error_guard = 0U;
                if (g_rx_length > 0U)
                {
                    pmbus_frame_class_t frame_class;

                    if (g_request_target == PMBUS_REQUEST_TARGET_ARP)
                    {
                        if ((g_rx_buffer[0] == PMBUS_ARP_COMMAND_GET_UDID) ||
                            (g_rx_buffer[0] == PMBUS_ARP_COMMAND_DIRECTED_GET_UDID))
                        {
                            pmbus_drv_save_pending_request_context();
                            g_write_frame_pending = 1U;
                            g_write_frame_pending_tick = get_tick();
                        }
                        else
                        {
                            pmbus_drv_process_arp_frame(0U);
                            pmbus_drv_reset_rx();
                            g_write_frame_pending = 0U;
                            g_write_frame_pending_tick = 0UL;
                        }
                    }
                    else if (g_request_target == PMBUS_REQUEST_TARGET_ZONE_WRITE)
                    {
                        pmbus_drv_process_zone_write_frame(0U);
                        pmbus_drv_reset_rx();
                        g_write_frame_pending = 0U;
                        g_write_frame_pending_tick = 0UL;
                    }
                    else
                    {
                        frame_class = pmbus_drv_classify_current_frame();
                        if (frame_class == PMBUS_FRAME_CLASS_WRITE_ONLY)
                        {
                            pmbus_drv_process_frame(0U);
                            pmbus_drv_reset_rx();
                            g_write_frame_pending = 0U;
                            g_write_frame_pending_tick = 0UL;
                        }
                        else
                        {
                            /*
                                A command-only frame for a read-capable command is not
                                sufficient to prove a legal read transaction yet. Keep
                                it pending and only commit to the read path when the
                                master really issues SLA+R. If the next event is a new
                                SLA+W instead, the pending frame will be processed as a
                                write-side illegal transaction and STATUS_CML can latch
                                INVALID_OR_UNSUPPORTED_DATA as required by the PMBus
                                validation checklist.
                            */
                            pmbus_drv_save_pending_request_context();
                            g_write_frame_pending = 1U;
                            g_write_frame_pending_tick = get_tick();
                        }
                    }
                }
                pmbus_io_i2c_set_ack();
                pmbus_io_i2c_disable_timeout_counter();
                break;

            case PMBUS_I2C_STATUS_SLA_R_ACK:
                g_tx_finish_bus_error_guard = 0U;
                pmbus_drv_update_request_target();
                if (g_request_target == PMBUS_REQUEST_TARGET_ARA)
                {
                    pmbus_drv_prepare_ara_response();
                    pmbus_drv_load_next_tx_byte();
                }
                else if (g_request_target == PMBUS_REQUEST_TARGET_ARP)
                {
                    if (g_write_frame_pending != 0U)
                    {
                        pmbus_drv_restore_pending_request_context();
                    }
                    g_write_frame_pending = 0U;
                    g_write_frame_pending_tick = 0UL;
                    pmbus_drv_process_frame(1U);
                    pmbus_drv_reset_rx();
                    pmbus_drv_load_next_tx_byte();
                }
                else if (g_request_target == PMBUS_REQUEST_TARGET_ZONE_READ)
                {
                    pmbus_drv_prepare_zone_read_response();
                    pmbus_drv_load_next_tx_byte();
                }
                else if ((g_write_frame_pending != 0U) || (g_rx_length > 0U))
                {
                    if (g_write_frame_pending != 0U)
                    {
                        pmbus_drv_restore_pending_request_context();
                    }
                    g_write_frame_pending = 0U;
                    g_write_frame_pending_tick = 0UL;
                    pmbus_drv_process_frame(1U);
                    pmbus_drv_reset_rx();
                    pmbus_drv_load_next_tx_byte();
                }
                else
                {
                    pmbus_drv_prepare_default_read();
                    pmbus_drv_load_next_tx_byte();
                }
                pmbus_io_i2c_set_ack();
                pmbus_io_i2c_disable_timeout_counter();
                break;

            case PMBUS_I2C_STATUS_DATA_TX_ACK:
                g_tx_finish_bus_error_guard = 0U;
                pmbus_drv_load_next_tx_byte();
                if ((g_request_target == PMBUS_REQUEST_TARGET_ARA) && (g_tx_index >= g_tx_length))
                {
                    pmbus_io_i2c_clear_ack();
                }
                else
                {
                    pmbus_io_i2c_set_ack();
                }
                pmbus_io_i2c_disable_timeout_counter();
                break;

            case PMBUS_I2C_STATUS_DATA_TX_NACK:
            case PMBUS_I2C_STATUS_LAST_TX_ACK:
                pmbus_drv_reset_tx();
                pmbus_drv_reset_rx();
                g_tx_finish_bus_error_guard = 1U;
                g_write_frame_pending = 0U;
                g_write_frame_pending_tick = 0UL;
                pmbus_io_i2c_set_ack();
                pmbus_io_i2c_disable_timeout_counter();
                if (g_request_target == PMBUS_REQUEST_TARGET_ARA)
                {
                    pmbus_app_release_alert();
                    g_ara_alias_inhibit = 1U;
#if PMBUS_I2C_ALIAS_SLOT_ARA == PMBUS_I2C_ALIAS_SLOT_DISABLED
                    pmbus_drv_restore_normal_address();
#else
                    g_ara_alias_active = 0U;
                    pmbus_drv_configure_alias_addresses();
#endif
                    pmbus_drv_queue_event(PMBUS_DEBUG_EVENT_ARA_ALIAS, g_current_slave_address_7bit, 0U);
                }
                g_request_target = PMBUS_REQUEST_TARGET_NORMAL;
                g_request_address_7bit = pmbus_app_get_slave_address_7bit();
                break;

            case PMBUS_I2C_STATUS_BUS_ERROR:
                if (g_tx_finish_bus_error_guard != 0U)
                {
                    /*
                        Some M031 I2C STOP/NACK endings report one 0x00 status
                        immediately after a normal slave-transmit completion.
                        Treat that single post-TX report as cleanup; a genuinely
                        stuck bus is still recovered by the SCL-low monitor.
                    */
                    pmbus_drv_reset_rx();
                    pmbus_drv_reset_tx();
                    g_tx_finish_bus_error_guard = 0U;
                    g_write_frame_pending = 0U;
                    g_write_frame_pending_tick = 0UL;
                    pmbus_io_i2c_disable_timeout_counter();
                    pmbus_io_i2c_clear_timeout_flag();
                    pmbus_io_i2c_bus_error_reset();
                    break;
                }

                g_tx_finish_bus_error_guard = 0U;
                pmbus_drv_reset_rx();
                pmbus_drv_reset_tx();
                g_write_frame_pending = 0U;
                g_write_frame_pending_tick = 0UL;
                pmbus_io_i2c_disable_timeout_counter();
                pmbus_io_i2c_clear_timeout_flag();
                pmbus_io_i2c_bus_error_reset();

                if (pmbus_drv_bus_lines_released() == 0U)
                {
                    pmbus_app_set_status_cml(PMBUS_STATUS_CML_OTHER_COMMUNICATION_FAULT);
                    g_bus_error_fault_count = (uint8_t)(g_bus_error_fault_count + 1U);
                    if (g_bus_error_fault_count >= PMBUS_I2C_BUS_ERROR_RECOVER_THRESHOLD)
                    {
                        pmbus_drv_set_recover_pending(PMBUS_RECOVER_REASON_BUS_ERROR);
                        g_bus_error_fault_count = 0U;
                    }
                }
                else
                {
                    g_bus_error_fault_count = 0U;
                }
                break;

            default:
                pmbus_io_i2c_set_ack();
                break;
    }

    pmbus_io_i2c_si_check();
    pmbus_io_isr_exit(saved_state);
}
