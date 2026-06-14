#ifndef PMBUS_DISPATCH_H
#define PMBUS_DISPATCH_H
#include "board_config.h"

#define PMBUS_COMMAND_PAGE                         0x00U
#define PMBUS_COMMAND_OPERATION                    0x01U
#define PMBUS_COMMAND_ON_OFF_CONFIG                0x02U
#define PMBUS_COMMAND_CLEAR_FAULTS                 0x03U
#define PMBUS_COMMAND_PHASE                        0x04U
#define PMBUS_COMMAND_PAGE_PLUS_WRITE              0x05U
#define PMBUS_COMMAND_PAGE_PLUS_READ               0x06U
#define PMBUS_COMMAND_ZONE_CONFIG                  0x07U
#define PMBUS_COMMAND_ZONE_ACTIVE                  0x08U
#define PMBUS_COMMAND_WRITE_PROTECT                0x10U
#define PMBUS_COMMAND_STORE_DEFAULT_ALL            0x11U
#define PMBUS_COMMAND_RESTORE_DEFAULT_ALL          0x12U
#define PMBUS_COMMAND_STORE_DEFAULT_CODE           0x13U
#define PMBUS_COMMAND_RESTORE_DEFAULT_CODE         0x14U
#define PMBUS_COMMAND_STORE_USER_ALL               0x15U
#define PMBUS_COMMAND_RESTORE_USER_ALL             0x16U
#define PMBUS_COMMAND_STORE_USER_CODE              0x17U
#define PMBUS_COMMAND_RESTORE_USER_CODE            0x18U
#define PMBUS_COMMAND_CAPABILITY                   0x19U
#define PMBUS_COMMAND_QUERY                        0x1AU
#define PMBUS_COMMAND_SMBALERT_MASK                0x1BU
#define PMBUS_COMMAND_VOUT_MODE                    0x20U
#define PMBUS_COMMAND_VOUT_COMMAND                 0x21U
#define PMBUS_COMMAND_VOUT_TRIM                    0x22U
#define PMBUS_COMMAND_VOUT_CAL_OFFSET              0x23U
#define PMBUS_COMMAND_VOUT_MAX                     0x24U
#define PMBUS_COMMAND_VOUT_MARGIN_HIGH             0x25U
#define PMBUS_COMMAND_VOUT_MARGIN_LOW              0x26U
#define PMBUS_COMMAND_VOUT_TRANSITION_RATE         0x27U
#define PMBUS_COMMAND_VOUT_DROOP                   0x28U
#define PMBUS_COMMAND_VOUT_SCALE_LOOP              0x29U
#define PMBUS_COMMAND_VOUT_SCALE_MONITOR           0x2AU
#define PMBUS_COMMAND_VOUT_MIN                     0x2BU
#define PMBUS_COMMAND_COEFFICIENTS                 0x30U
#define PMBUS_COMMAND_POUT_MAX                     0x31U
#define PMBUS_COMMAND_MAX_DUTY                     0x32U
#define PMBUS_COMMAND_FREQUENCY_SWITCH             0x33U
#define PMBUS_COMMAND_POWER_MODE                   0x34U
#define PMBUS_COMMAND_VIN_ON                       0x35U
#define PMBUS_COMMAND_VIN_OFF                      0x36U
#define PMBUS_COMMAND_INTERLEAVE                   0x37U
#define PMBUS_COMMAND_IOUT_CAL_GAIN                0x38U
#define PMBUS_COMMAND_IOUT_CAL_OFFSET              0x39U
#define PMBUS_COMMAND_FAN_CONFIG_1_2               0x3AU
#define PMBUS_COMMAND_FAN_COMMAND_1                0x3BU
#define PMBUS_COMMAND_FAN_COMMAND_2                0x3CU
#define PMBUS_COMMAND_FAN_CONFIG_3_4               0x3DU
#define PMBUS_COMMAND_FAN_COMMAND_3                0x3EU
#define PMBUS_COMMAND_FAN_COMMAND_4                0x3FU
#define PMBUS_COMMAND_VOUT_OV_FAULT_LIMIT          0x40U
#define PMBUS_COMMAND_VOUT_OV_FAULT_RESPONSE       0x41U
#define PMBUS_COMMAND_VOUT_OV_WARN_LIMIT           0x42U
#define PMBUS_COMMAND_VOUT_UV_WARN_LIMIT           0x43U
#define PMBUS_COMMAND_VOUT_UV_FAULT_LIMIT          0x44U
#define PMBUS_COMMAND_VOUT_UV_FAULT_RESPONSE       0x45U
#define PMBUS_COMMAND_IOUT_OC_FAULT_LIMIT          0x46U
#define PMBUS_COMMAND_IOUT_OC_FAULT_RESPONSE       0x47U
#define PMBUS_COMMAND_IOUT_OC_LV_FAULT_LIMIT       0x48U
#define PMBUS_COMMAND_IOUT_OC_LV_FAULT_RESPONSE    0x49U
#define PMBUS_COMMAND_IOUT_OC_WARN_LIMIT           0x4AU
#define PMBUS_COMMAND_IOUT_UC_FAULT_LIMIT          0x4BU
#define PMBUS_COMMAND_IOUT_UC_FAULT_RESPONSE       0x4CU
#define PMBUS_COMMAND_OT_FAULT_LIMIT               0x4FU
#define PMBUS_COMMAND_OT_FAULT_RESPONSE            0x50U
#define PMBUS_COMMAND_OT_WARN_LIMIT                0x51U
#define PMBUS_COMMAND_UT_WARN_LIMIT                0x52U
#define PMBUS_COMMAND_UT_FAULT_LIMIT               0x53U
#define PMBUS_COMMAND_UT_FAULT_RESPONSE            0x54U
#define PMBUS_COMMAND_VIN_OV_FAULT_LIMIT           0x55U
#define PMBUS_COMMAND_VIN_OV_FAULT_RESPONSE        0x56U
#define PMBUS_COMMAND_VIN_OV_WARN_LIMIT            0x57U
#define PMBUS_COMMAND_VIN_UV_WARN_LIMIT            0x58U
#define PMBUS_COMMAND_VIN_UV_FAULT_LIMIT           0x59U
#define PMBUS_COMMAND_VIN_UV_FAULT_RESPONSE        0x5AU
#define PMBUS_COMMAND_IIN_OC_FAULT_LIMIT           0x5BU
#define PMBUS_COMMAND_IIN_OC_FAULT_RESPONSE        0x5CU
#define PMBUS_COMMAND_IIN_OC_WARN_LIMIT            0x5DU
#define PMBUS_COMMAND_POWER_GOOD_ON                0x5EU
#define PMBUS_COMMAND_POWER_GOOD_OFF               0x5FU
#define PMBUS_COMMAND_TON_DELAY                    0x60U
#define PMBUS_COMMAND_TON_RISE                     0x61U
#define PMBUS_COMMAND_TON_MAX_FAULT_LIMIT          0x62U
#define PMBUS_COMMAND_TON_MAX_FAULT_RESPONSE       0x63U
#define PMBUS_COMMAND_TOFF_DELAY                   0x64U
#define PMBUS_COMMAND_TOFF_FALL                    0x65U
#define PMBUS_COMMAND_TOFF_MAX_WARN_LIMIT          0x66U
#define PMBUS_COMMAND_POUT_OP_FAULT_LIMIT          0x68U
#define PMBUS_COMMAND_POUT_OP_FAULT_RESPONSE       0x69U
#define PMBUS_COMMAND_POUT_OP_WARN_LIMIT           0x6AU
#define PMBUS_COMMAND_PIN_OP_WARN_LIMIT            0x6BU
#define PMBUS_COMMAND_STATUS_BYTE                  0x78U
#define PMBUS_COMMAND_STATUS_WORD                  0x79U
#define PMBUS_COMMAND_STATUS_VOUT                  0x7AU
#define PMBUS_COMMAND_STATUS_IOUT                  0x7BU
#define PMBUS_COMMAND_STATUS_INPUT                 0x7CU
#define PMBUS_COMMAND_STATUS_TEMPERATURE           0x7DU
#define PMBUS_COMMAND_STATUS_CML                   0x7EU
#define PMBUS_COMMAND_STATUS_OTHER                 0x7FU
#define PMBUS_COMMAND_STATUS_MFR_SPECIFIC          0x80U
#define PMBUS_COMMAND_STATUS_FANS_1_2              0x81U
#define PMBUS_COMMAND_STATUS_FANS_3_4              0x82U
#define PMBUS_COMMAND_READ_KWH_IN                  0x83U
#define PMBUS_COMMAND_READ_KWH_OUT                 0x84U
#define PMBUS_COMMAND_READ_KWH_CONFIG              0x85U
#define PMBUS_COMMAND_READ_EIN                     0x86U
#define PMBUS_COMMAND_READ_EOUT                    0x87U
#define PMBUS_COMMAND_READ_VIN                     0x88U
#define PMBUS_COMMAND_READ_IIN                     0x89U
#define PMBUS_COMMAND_READ_VCAP                    0x8AU
#define PMBUS_COMMAND_READ_VOUT                    0x8BU
#define PMBUS_COMMAND_READ_IOUT                    0x8CU
#define PMBUS_COMMAND_READ_TEMPERATURE_1           0x8DU
#define PMBUS_COMMAND_READ_TEMPERATURE_2           0x8EU
#define PMBUS_COMMAND_READ_TEMPERATURE_3           0x8FU
#define PMBUS_COMMAND_READ_FAN_SPEED_1             0x90U
#define PMBUS_COMMAND_READ_FAN_SPEED_2             0x91U
#define PMBUS_COMMAND_READ_FAN_SPEED_3             0x92U
#define PMBUS_COMMAND_READ_FAN_SPEED_4             0x93U
#define PMBUS_COMMAND_READ_DUTY_CYCLE              0x94U
#define PMBUS_COMMAND_READ_FREQUENCY               0x95U
#define PMBUS_COMMAND_READ_POUT                    0x96U
#define PMBUS_COMMAND_READ_PIN                     0x97U
#define PMBUS_COMMAND_PMBUS_REVISION               0x98U
#define PMBUS_COMMAND_MFR_ID                       0x99U
#define PMBUS_COMMAND_MFR_MODEL                    0x9AU
#define PMBUS_COMMAND_MFR_REVISION                 0x9BU
#define PMBUS_COMMAND_MFR_LOCATION                 0x9CU
#define PMBUS_COMMAND_MFR_DATE                     0x9DU
#define PMBUS_COMMAND_MFR_SERIAL                   0x9EU
#define PMBUS_COMMAND_APP_PROFILE_SUPPORT          0x9FU
#define PMBUS_COMMAND_MFR_VIN_MIN                  0xA0U
#define PMBUS_COMMAND_MFR_VIN_MAX                  0xA1U
#define PMBUS_COMMAND_MFR_IIN_MAX                  0xA2U
#define PMBUS_COMMAND_MFR_PIN_MAX                  0xA3U
#define PMBUS_COMMAND_MFR_VOUT_MIN                 0xA4U
#define PMBUS_COMMAND_MFR_VOUT_MAX                 0xA5U
#define PMBUS_COMMAND_MFR_IOUT_MAX                 0xA6U
#define PMBUS_COMMAND_MFR_POUT_MAX                 0xA7U
#define PMBUS_COMMAND_MFR_TAMBIENT_MAX             0xA8U
#define PMBUS_COMMAND_MFR_TAMBIENT_MIN             0xA9U
#define PMBUS_COMMAND_MFR_EFFICIENCY_LL            0xAAU
#define PMBUS_COMMAND_MFR_EFFICIENCY_HL            0xABU
#define PMBUS_COMMAND_MFR_PIN_ACCURACY             0xACU
#define PMBUS_COMMAND_IC_DEVICE_ID                 0xADU
#define PMBUS_COMMAND_IC_DEVICE_REV                0xAEU
#define PMBUS_COMMAND_USER_DATA_00                 0xB0U
#define PMBUS_COMMAND_USER_DATA_01                 0xB1U
#define PMBUS_COMMAND_USER_DATA_02                 0xB2U
#define PMBUS_COMMAND_USER_DATA_03                 0xB3U
#define PMBUS_COMMAND_USER_DATA_04                 0xB4U
#define PMBUS_COMMAND_USER_DATA_05                 0xB5U
#define PMBUS_COMMAND_USER_DATA_06                 0xB6U
#define PMBUS_COMMAND_USER_DATA_07                 0xB7U
#define PMBUS_COMMAND_USER_DATA_08                 0xB8U
#define PMBUS_COMMAND_USER_DATA_09                 0xB9U
#define PMBUS_COMMAND_USER_DATA_10                 0xBAU
#define PMBUS_COMMAND_USER_DATA_11                 0xBBU
#define PMBUS_COMMAND_USER_DATA_12                 0xBCU
#define PMBUS_COMMAND_USER_DATA_13                 0xBDU
#define PMBUS_COMMAND_USER_DATA_14                 0xBEU
#define PMBUS_COMMAND_USER_DATA_15                 0xBFU
#define PMBUS_COMMAND_MFR_MAX_TEMP_1               0xC0U
#define PMBUS_COMMAND_MFR_MAX_TEMP_2               0xC1U
#define PMBUS_COMMAND_MFR_MAX_TEMP_3               0xC2U
#define PMBUS_COMMAND_MFR_SPECIFIC_C4              0xC4U
#define PMBUS_COMMAND_MFR_SPECIFIC_C5              0xC5U
#define PMBUS_COMMAND_MFR_SPECIFIC_C6              0xC6U
#define PMBUS_COMMAND_MFR_SPECIFIC_C7              0xC7U
#define PMBUS_COMMAND_MFR_SPECIFIC_C8              0xC8U
#define PMBUS_COMMAND_MFR_SPECIFIC_C9              0xC9U
#define PMBUS_COMMAND_MFR_SPECIFIC_CA              0xCAU
#define PMBUS_COMMAND_MFR_SPECIFIC_CB              0xCBU
#define PMBUS_COMMAND_MFR_SPECIFIC_CC              0xCCU
#define PMBUS_COMMAND_MFR_SPECIFIC_CD              0xCDU
#define PMBUS_COMMAND_MFR_SPECIFIC_CE              0xCEU
#define PMBUS_COMMAND_MFR_SPECIFIC_CF              0xCFU
#define PMBUS_COMMAND_MFR_COLD_REDUNDANCY_CONFIG   0xD0U
#define PMBUS_COMMAND_MFR_SPECIFIC_D1              0xD1U
#define PMBUS_COMMAND_MFR_SPECIFIC_D2              0xD2U
#define PMBUS_COMMAND_MFR_SPECIFIC_D3              0xD3U
#define PMBUS_COMMAND_MFR_SPECIFIC_D4              0xD4U
#define PMBUS_COMMAND_MFR_SPECIFIC_D5              0xD5U
#define PMBUS_COMMAND_MFR_FWUPLOAD_MODE            0xD6U
#define PMBUS_COMMAND_MFR_FWUPLOAD                 0xD7U
#define PMBUS_COMMAND_MFR_FWUPLOAD_STATUS          0xD8U
#define PMBUS_COMMAND_MFR_SPECIFIC_D9              0xD9U
#define PMBUS_COMMAND_MFR_SPECIFIC_DA              0xDAU
#define PMBUS_COMMAND_MFR_SPECIFIC_DB              0xDBU
#define PMBUS_COMMAND_MFR_BLACKBOX                 0xDCU
#define PMBUS_COMMAND_MFR_SPECIFIC_DD              0xDDU
#define PMBUS_COMMAND_MFR_SPECIFIC_DE              0xDEU
#define PMBUS_COMMAND_MFR_SPECIFIC_DF              0xDFU
#define PMBUS_COMMAND_MFR_SPECIFIC_E0              0xE0U
#define PMBUS_COMMAND_MFR_SPECIFIC_E1              0xE1U
#define PMBUS_COMMAND_MFR_SPECIFIC_E2              0xE2U
#define PMBUS_COMMAND_MFR_SPECIFIC_E3              0xE3U
#define PMBUS_COMMAND_MFR_SPECIFIC_E4              0xE4U
#define PMBUS_COMMAND_MFR_SPECIFIC_E5              0xE5U
#define PMBUS_COMMAND_MFR_SPECIFIC_E6              0xE6U
#define PMBUS_COMMAND_MFR_SPECIFIC_E7              0xE7U
#define PMBUS_COMMAND_MFR_SPECIFIC_E8              0xE8U
#define PMBUS_COMMAND_MFR_SPECIFIC_E9              0xE9U
#define PMBUS_COMMAND_MFR_SPECIFIC_EA              0xEAU
#define PMBUS_COMMAND_MFR_SPECIFIC_EB              0xEBU
#define PMBUS_COMMAND_MFR_SPECIFIC_EC              0xECU
#define PMBUS_COMMAND_MFR_SPECIFIC_ED              0xEDU
#define PMBUS_COMMAND_MFR_SPECIFIC_EE              0xEEU
#define PMBUS_COMMAND_MFR_SPECIFIC_EF              0xEFU
#define PMBUS_COMMAND_MFR_SPECIFIC_F0              0xF0U
#define PMBUS_COMMAND_MFR_SPECIFIC_F1              0xF1U
#define PMBUS_COMMAND_MFR_SPECIFIC_F2              0xF2U
#define PMBUS_COMMAND_MFR_SPECIFIC_F3              0xF3U
#define PMBUS_COMMAND_MFR_SPECIFIC_F4              0xF4U
#define PMBUS_COMMAND_MFR_SPECIFIC_F5              0xF5U
#define PMBUS_COMMAND_MFR_SPECIFIC_F6              0xF6U
#define PMBUS_COMMAND_MFR_SPECIFIC_F7              0xF7U
#define PMBUS_COMMAND_MFR_SPECIFIC_F8              0xF8U
#define PMBUS_COMMAND_MFR_SPECIFIC_F9              0xF9U
#define PMBUS_COMMAND_MFR_SPECIFIC_FA              0xFAU
#define PMBUS_COMMAND_MFR_SPECIFIC_FB              0xFBU
#define PMBUS_COMMAND_MFR_SPECIFIC_FC              0xFCU
#define PMBUS_COMMAND_MFR_SPECIFIC_FD              0xFDU
#define PMBUS_COMMAND_MFR_SPECIFIC_COMMAND_EXT     0xFEU
#define PMBUS_COMMAND_PMBUS_COMMAND_EXT            0xFFU

typedef enum
{
    PMBUS_PROTOCOL_UNKNOWN = 0,
    PMBUS_PROTOCOL_SEND_BYTE,
    PMBUS_PROTOCOL_WRITE_BYTE,
    PMBUS_PROTOCOL_WRITE_WORD,
    PMBUS_PROTOCOL_READ_BYTE,
    PMBUS_PROTOCOL_READ_WORD,
    PMBUS_PROTOCOL_READ_DWORD,
    PMBUS_PROTOCOL_BLOCK_WRITE,
    PMBUS_PROTOCOL_BLOCK_READ,
    PMBUS_PROTOCOL_PROCESS_CALL,
    PMBUS_PROTOCOL_BLOCK_WRITE_READ_PROCESS_CALL
} pmbus_dispatch_protocol_t;

typedef struct
{
    unsigned char command;
    unsigned char payload[PMBUS_MAX_BLOCK_SIZE + 1U];
    unsigned char data_len;
    unsigned char repeated_start;
    unsigned char pec_present;
    unsigned char pec_valid;
    pmbus_dispatch_protocol_t protocol;
} pmbus_dispatch_transaction_t;

pmbus_dispatch_protocol_t pmbus_dispatch_detect_protocol(unsigned char command, unsigned char data_len, unsigned char *payload, unsigned char repeated_start);
unsigned char pmbus_dispatch_is_known_command(unsigned char command);
unsigned char pmbus_dispatch_execute(pmbus_dispatch_transaction_t *transaction, unsigned char *tx_buffer, unsigned char *tx_length);
void pmbus_dispatch_prepare_error_response(unsigned char command, unsigned char *tx_buffer, unsigned char *tx_length);

#endif

