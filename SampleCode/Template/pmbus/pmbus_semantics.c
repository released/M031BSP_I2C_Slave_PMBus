#include <stdio.h>
#include "pmbus_types.h"
#include "board_config.h"
#include "pmbus_cfg_user.h"
#include "pmbus_protocol.h"
#include "pmbus_profile_names.h"
#include "pmbus_io.h"
#include "pmbus_semantics.h"

#define PMBUS_SEMANTIC_DIR_WRITE              0U
#define PMBUS_SEMANTIC_DIR_READ               1U
#define PMBUS_SEMANTIC_DATA_MAX               4U

typedef struct
{
    uint8_t direction;
    uint8_t command;
    uint8_t protocol;
    uint8_t length;
    uint8_t pec_present;
    uint8_t data[PMBUS_SEMANTIC_DATA_MAX];
} pmbus_semantic_event_t;

typedef struct
{
    uint8_t mask;
    const char *name;
    const char *todo;
} pmbus_semantic_bit8_t;

static volatile uint8_t g_semantic_head;
static volatile uint8_t g_semantic_tail;
static volatile uint8_t g_semantic_drop_count;
static pmbus_semantic_event_t g_semantic_queue[PMBUS_SEMANTIC_QUEUE_SIZE];

static uint8_t pmbus_semantics_should_record(uint8_t command);
static void pmbus_semantics_record(uint8_t direction,
                                   uint8_t command,
                                   uint8_t protocol,
                                   const uint8_t *data,
                                   uint8_t data_len,
                                   uint8_t pec_present);
static const char *pmbus_semantics_command_name(uint8_t command);
static uint16_t pmbus_semantics_get_u16(const pmbus_semantic_event_t *event);
static void pmbus_semantics_print_event(const pmbus_semantic_event_t *event);
static void pmbus_semantics_print_byte_bits(const char *title,
                                            uint8_t value,
                                            const pmbus_semantic_bit8_t *bits,
                                            uint8_t count);
static uint8_t pmbus_semantics_mask_shift(uint8_t mask);
static void pmbus_semantics_print_status_byte(uint8_t value);
static void pmbus_semantics_print_status_word(uint16_t value);
static void pmbus_semantics_print_status_vout(uint8_t value);
static void pmbus_semantics_print_status_iout(uint8_t value);
static void pmbus_semantics_print_status_input(uint8_t value);
static void pmbus_semantics_print_status_temperature(uint8_t value);
static void pmbus_semantics_print_status_cml(uint8_t value);
static void pmbus_semantics_print_status_other(uint8_t value);
static void pmbus_semantics_print_status_fans(uint8_t value);
static void pmbus_semantics_print_operation(uint8_t value);
static void pmbus_semantics_print_on_off_config(uint8_t value);
static void pmbus_semantics_print_vout_mode(uint8_t value);
static void pmbus_semantics_print_fault_response(uint8_t value);
static void pmbus_semantics_print_fan_config(uint8_t value);
static void pmbus_semantics_print_profile_bits(uint8_t command, uint16_t value, uint8_t width);

static const char *pmbus_semantics_command_name(uint8_t command)
{
    static char command_name_buffer[28];
    const char *name;

    name = pmbus_profile_get_command_name(command, command_name_buffer, (uint8_t)sizeof(command_name_buffer));
    if (name != (const char *)0)
    {
        return name;
    }

    switch (command)
    {
        case PMBUS_COMMAND_OPERATION: return "OPERATION";
        case PMBUS_COMMAND_ON_OFF_CONFIG: return "ON_OFF_CONFIG";
        case PMBUS_COMMAND_WRITE_PROTECT: return "WRITE_PROTECT";
        case PMBUS_COMMAND_CAPABILITY: return "CAPABILITY";
        case PMBUS_COMMAND_SMBALERT_MASK: return "SMBALERT_MASK";
        case PMBUS_COMMAND_VOUT_MODE: return "VOUT_MODE";
        case PMBUS_COMMAND_POWER_MODE: return "POWER_MODE";
        case PMBUS_COMMAND_FAN_CONFIG_1_2: return "FAN_CONFIG_1_2";
        case PMBUS_COMMAND_FAN_CONFIG_3_4: return "FAN_CONFIG_3_4";
        case PMBUS_COMMAND_STATUS_BYTE: return "STATUS_BYTE";
        case PMBUS_COMMAND_STATUS_WORD: return "STATUS_WORD";
        case PMBUS_COMMAND_STATUS_VOUT: return "STATUS_VOUT";
        case PMBUS_COMMAND_STATUS_IOUT: return "STATUS_IOUT";
        case PMBUS_COMMAND_STATUS_INPUT: return "STATUS_INPUT";
        case PMBUS_COMMAND_STATUS_TEMPERATURE: return "STATUS_TEMPERATURE";
        case PMBUS_COMMAND_STATUS_CML: return "STATUS_CML";
        case PMBUS_COMMAND_STATUS_OTHER: return "STATUS_OTHER";
        case PMBUS_COMMAND_STATUS_MFR_SPECIFIC: return "STATUS_MFR_SPECIFIC";
        case PMBUS_COMMAND_STATUS_FANS_1_2: return "STATUS_FANS_1_2";
        case PMBUS_COMMAND_STATUS_FANS_3_4: return "STATUS_FANS_3_4";
        default:
            break;
    }

    sprintf(command_name_buffer, "CMD_0x%02X", (unsigned int)command);
    return command_name_buffer;
}

static uint8_t pmbus_semantics_should_record(uint8_t command)
{
    switch (command)
    {
        case PMBUS_COMMAND_OPERATION:
        case PMBUS_COMMAND_ON_OFF_CONFIG:
        case PMBUS_COMMAND_WRITE_PROTECT:
        case PMBUS_COMMAND_CAPABILITY:
        case PMBUS_COMMAND_SMBALERT_MASK:
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
        case PMBUS_COMMAND_STATUS_WORD:
        case PMBUS_COMMAND_STATUS_VOUT:
        case PMBUS_COMMAND_STATUS_IOUT:
        case PMBUS_COMMAND_STATUS_INPUT:
        case PMBUS_COMMAND_STATUS_TEMPERATURE:
        case PMBUS_COMMAND_STATUS_CML:
        case PMBUS_COMMAND_STATUS_OTHER:
        case PMBUS_COMMAND_STATUS_MFR_SPECIFIC:
        case PMBUS_COMMAND_STATUS_FANS_1_2:
        case PMBUS_COMMAND_STATUS_FANS_3_4:
            return 1U;

        default:
            break;
    }

#if (PMBUS_COMMAND_PROFILE == PMBUS_COMMAND_PROFILE_M_CRPS)
    if ((command == 0xD0U) ||
        (command == 0xD6U) ||
        (command == 0xD8U) ||
        (command == 0xDBU) ||
        (command == 0xDFU) ||
        (command == 0xE1U) ||
        (command == 0xE2U) ||
        (command == 0xE4U) ||
        (command == 0xEDU))
    {
        return 1U;
    }
#elif (PMBUS_COMMAND_PROFILE == PMBUS_COMMAND_PROFILE_TI_UCD90XXX)
    if ((command >= 0xD2U) && (command <= 0xFCU))
    {
        return 1U;
    }
#endif

    return 0U;
}

static void pmbus_semantics_record(uint8_t direction,
                                   uint8_t command,
                                   uint8_t protocol,
                                   const uint8_t *data,
                                   uint8_t data_len,
                                   uint8_t pec_present)
{
#if PMBUS_DEBUG_ENABLE && PMBUS_DEBUG_PRINT_SEMANTICS
    uint8_t index;
    uint8_t copy_len;
    uint8_t irq_state;
    uint8_t next_head;

    if (pmbus_semantics_should_record(command) == 0U)
    {
        return;
    }

    copy_len = data_len;
    if (copy_len > PMBUS_SEMANTIC_DATA_MAX)
    {
        copy_len = PMBUS_SEMANTIC_DATA_MAX;
    }

    irq_state = pmbus_io_irq_save_disable();
    next_head = (uint8_t)(g_semantic_head + 1U);
    if (next_head >= PMBUS_SEMANTIC_QUEUE_SIZE)
    {
        next_head = 0U;
    }

    if (next_head != g_semantic_tail)
    {
        g_semantic_queue[g_semantic_head].direction = direction;
        g_semantic_queue[g_semantic_head].command = command;
        g_semantic_queue[g_semantic_head].protocol = protocol;
        g_semantic_queue[g_semantic_head].length = data_len;
        g_semantic_queue[g_semantic_head].pec_present = pec_present;

        for (index = 0U; index < PMBUS_SEMANTIC_DATA_MAX; index++)
        {
            if ((data != (const uint8_t *)0) && (index < copy_len))
            {
                g_semantic_queue[g_semantic_head].data[index] = data[index];
            }
            else
            {
                g_semantic_queue[g_semantic_head].data[index] = 0U;
            }
        }

        g_semantic_head = next_head;
    }
    else
    {
        if (g_semantic_drop_count < 255U)
        {
            g_semantic_drop_count++;
        }
    }
    pmbus_io_irq_restore(irq_state);
#else
    (void)direction;
    (void)command;
    (void)protocol;
    (void)data;
    (void)data_len;
    (void)pec_present;
#endif
}

void pmbus_semantics_record_write(unsigned char command,
                                  unsigned char protocol,
                                  const unsigned char *payload,
                                  unsigned char payload_len)
{
    pmbus_semantics_record(PMBUS_SEMANTIC_DIR_WRITE,
        (uint8_t)command,
        (uint8_t)protocol,
        (const uint8_t *)payload,
        (uint8_t)payload_len,
        0U);
}

void pmbus_semantics_record_read_response(unsigned char command,
                                          unsigned char protocol,
                                          const unsigned char *data,
                                          unsigned char data_len,
                                          unsigned char pec_present)
{
    pmbus_semantics_record(PMBUS_SEMANTIC_DIR_READ,
        (uint8_t)command,
        (uint8_t)protocol,
        (const uint8_t *)data,
        (uint8_t)data_len,
        (uint8_t)pec_present);
}

static uint16_t pmbus_semantics_get_u16(const pmbus_semantic_event_t *event)
{
    uint16_t value;

    value = (uint16_t)event->data[0];
    value = (uint16_t)(value | ((uint16_t)event->data[1] << 8));
    return value;
}

static void pmbus_semantics_print_byte_bits(const char *title,
                                            uint8_t value,
                                            const pmbus_semantic_bit8_t *bits,
                                            uint8_t count)
{
    uint8_t index;
    uint8_t field_value;
    uint8_t shift;
    uint16_t mask16;

    PMBUS_DEBUG_PRINT("  %s=0x%02X\r\n", title, (unsigned int)value);
    for (index = 0U; index < count; index++)
    {
        mask16 = (uint16_t)bits[index].mask;
        if ((mask16 != 0U) && ((mask16 & (uint16_t)(mask16 - 1U)) == 0U))
        {
            field_value = (uint8_t)((value & bits[index].mask) != 0U);
        }
        else
        {
            shift = pmbus_semantics_mask_shift(bits[index].mask);
            field_value = (uint8_t)((value & bits[index].mask) >> shift);
        }

        PMBUS_DEBUG_PRINT("    %-30s=%u -> %s\r\n",
            bits[index].name,
            (unsigned int)field_value,
            bits[index].todo);
    }
}

static uint8_t pmbus_semantics_mask_shift(uint8_t mask)
{
    uint8_t shift;

    shift = 0U;
    while (((mask & 0x01U) == 0U) && (shift < 8U))
    {
        mask = (uint8_t)(mask >> 1);
        shift++;
    }

    return shift;
}

static void pmbus_semantics_print_operation(uint8_t value)
{
    uint8_t margin_mode;

    margin_mode = (uint8_t)((value >> 4) & 0x07U);
    PMBUS_DEBUG_PRINT("  OPERATION=0x%02X\r\n", (unsigned int)value);
    PMBUS_DEBUG_PRINT("    bit7 ON_OFF=%u -> TODO: bind to product output enable / soft-off policy\r\n",
        (unsigned int)((value & 0x80U) != 0U));
    PMBUS_DEBUG_PRINT("    bits[6:4] MARGIN_MODE=0x%X -> TODO: bind to margin high/low/nominal rail policy\r\n",
        (unsigned int)margin_mode);
    PMBUS_DEBUG_PRINT("    bits[3:0] lower policy=0x%X -> TODO: validate product-specific OPERATION subfunction use\r\n",
        (unsigned int)(value & 0x0FU));
}

static void pmbus_semantics_print_on_off_config(uint8_t value)
{
    static const pmbus_semantic_bit8_t bits[] =
    {
        { 0x10U, "bit4 power-up behavior", "TODO: bind input-power auto-start policy" },
        { 0x08U, "bit3 CONTROL turn-on", "TODO: bind CONTROL pin turn-on behavior" },
        { 0x04U, "bit2 CONTROL polarity", "TODO: bind CONTROL pin active polarity" },
        { 0x02U, "bit1 OPERATION control", "TODO: bind OPERATION command enable policy" },
        { 0x01U, "bit0 immediate off", "TODO: bind shutdown delay / immediate-off policy" }
    };

    pmbus_semantics_print_byte_bits("ON_OFF_CONFIG", value, bits, (uint8_t)(sizeof(bits) / sizeof(bits[0])));
}

static void pmbus_semantics_print_vout_mode(uint8_t value)
{
    uint8_t format;
    uint8_t parameter;
    const char *format_name;

    format = (uint8_t)(value & PMBUS_VOUT_MODE_FORMAT_MASK);
    parameter = (uint8_t)(value & PMBUS_VOUT_MODE_PARAMETER_MASK);
    format_name = "UNKNOWN";

    if (format == PMBUS_VOUT_MODE_FORMAT_ULINEAR16)
    {
        format_name = "ULINEAR16";
    }
    else if (format == PMBUS_VOUT_MODE_FORMAT_VID)
    {
        format_name = "VID";
    }
    else if (format == PMBUS_VOUT_MODE_FORMAT_DIRECT)
    {
        format_name = "DIRECT";
    }
    else if (format == PMBUS_VOUT_MODE_FORMAT_IEEE_HALF)
    {
        format_name = "IEEE_HALF";
    }

    PMBUS_DEBUG_PRINT("  VOUT_MODE=0x%02X\r\n", (unsigned int)value);
    PMBUS_DEBUG_PRINT("    bit7 RELATIVE=%u -> TODO: bind relative voltage policy if product uses it\r\n",
        (unsigned int)((value & PMBUS_VOUT_MODE_RELATIVE_MASK) != 0U));
    PMBUS_DEBUG_PRINT("    bits[6:5] FORMAT=%s -> VOUT_* words must use this format\r\n", format_name);
    PMBUS_DEBUG_PRINT("    bits[4:0] PARAMETER=0x%02X -> exponent / VID table / format parameter\r\n",
        (unsigned int)parameter);
}

static void pmbus_semantics_print_fault_response(uint8_t value)
{
    PMBUS_DEBUG_PRINT("  FAULT_RESPONSE=0x%02X\r\n", (unsigned int)value);
    PMBUS_DEBUG_PRINT("    bits[7:6] response action=0x%X -> TODO: bind shutdown/retry/latch action\r\n",
        (unsigned int)((value >> 6) & 0x03U));
    PMBUS_DEBUG_PRINT("    bits[5:3] retry setting=0x%X -> TODO: bind retry count/timing policy\r\n",
        (unsigned int)((value >> 3) & 0x07U));
    PMBUS_DEBUG_PRINT("    bits[2:0] delay setting=0x%X -> TODO: bind fault response delay policy\r\n",
        (unsigned int)(value & 0x07U));
}

static void pmbus_semantics_print_fan_config(uint8_t value)
{
    static const pmbus_semantic_bit8_t bits[] =
    {
        { 0x80U, "bit7 fan installed", "TODO: bind fan presence / SKU policy" },
        { 0x40U, "bit6 commanded mode", "TODO: bind RPM/PWM command interpretation" },
        { 0x30U, "bits[5:4] tach pulse", "TODO: bind tach pulse-per-rev setting" },
        { 0x08U, "bit3 direction", "TODO: bind fan direction/reversal policy" },
        { 0x07U, "bits[2:0] mode", "TODO: bind fan mode policy" }
    };

    pmbus_semantics_print_byte_bits("FAN_CONFIG", value, bits, (uint8_t)(sizeof(bits) / sizeof(bits[0])));
}

static void pmbus_semantics_print_status_byte(uint8_t value)
{
    static const pmbus_semantic_bit8_t bits[] =
    {
        { PMBUS_STATUS_BYTE_BUSY, "bit7 BUSY", "status source hook" },
        { PMBUS_STATUS_BYTE_OFF, "bit6 OFF", "status source hook" },
        { PMBUS_STATUS_BYTE_VOUT_OV_FAULT, "bit5 VOUT_OV_FAULT", "status source hook" },
        { PMBUS_STATUS_BYTE_IOUT_OC_FAULT, "bit4 IOUT_OC_FAULT", "status source hook" },
        { PMBUS_STATUS_BYTE_VIN_UV_FAULT, "bit3 VIN_UV_FAULT", "status source hook" },
        { PMBUS_STATUS_BYTE_TEMPERATURE, "bit2 TEMPERATURE", "status source hook" },
        { PMBUS_STATUS_BYTE_CML, "bit1 CML", "status source hook" },
        { PMBUS_STATUS_BYTE_NONE_OF_THE_ABOVE, "bit0 NONE_OF_THE_ABOVE", "status source hook" }
    };

    pmbus_semantics_print_byte_bits("STATUS_BYTE", value, bits, (uint8_t)(sizeof(bits) / sizeof(bits[0])));
}

static void pmbus_semantics_print_status_word(uint16_t value)
{
    PMBUS_DEBUG_PRINT("  STATUS_WORD=0x%04X\r\n", (unsigned int)value);
    PMBUS_DEBUG_PRINT("    bit15 VOUT=%u, bit14 IOUT/POUT=%u, bit13 INPUT=%u, bit12 MFR=%u\r\n",
        (unsigned int)((value & PMBUS_STATUS_WORD_VOUT) != 0U),
        (unsigned int)((value & PMBUS_STATUS_WORD_IOUT_POUT) != 0U),
        (unsigned int)((value & PMBUS_STATUS_WORD_INPUT) != 0U),
        (unsigned int)((value & PMBUS_STATUS_WORD_MFR_SPECIFIC) != 0U));
    PMBUS_DEBUG_PRINT("    bit11 PG=%u, bit10 FANS=%u, bit9 OTHER=%u, bit8 UNKNOWN=%u, bit1 CML=%u\r\n",
        (unsigned int)((value & PMBUS_STATUS_WORD_PG_STATUS) != 0U),
        (unsigned int)((value & PMBUS_STATUS_WORD_FANS) != 0U),
        (unsigned int)((value & PMBUS_STATUS_WORD_OTHER) != 0U),
        (unsigned int)((value & PMBUS_STATUS_WORD_UNKNOWN) != 0U),
        (unsigned int)((value & PMBUS_STATUS_WORD_LOW_BYTE_CML) != 0U));
    PMBUS_DEBUG_PRINT("    TODO: bind each asserted group bit to product fault source and ALERT# policy\r\n");
}

static void pmbus_semantics_print_status_vout(uint8_t value)
{
    static const pmbus_semantic_bit8_t bits[] =
    {
        { PMBUS_STATUS_VOUT_VOUT_OV_FAULT, "bit7 OV_FAULT", "TODO: bind output OV comparator / ADC" },
        { PMBUS_STATUS_VOUT_VOUT_OV_WARNING, "bit6 OV_WARNING", "TODO: bind output OV warning threshold" },
        { PMBUS_STATUS_VOUT_VOUT_UV_WARNING, "bit5 UV_WARNING", "TODO: bind output UV warning threshold" },
        { PMBUS_STATUS_VOUT_VOUT_UV_FAULT, "bit4 UV_FAULT", "TODO: bind output UV comparator / ADC" },
        { PMBUS_STATUS_VOUT_VOUT_MAX_MIN_WARNING, "bit3 MAX_MIN_WARNING", "TODO: bind margin/min/max policy" },
        { PMBUS_STATUS_VOUT_TON_MAX_FAULT, "bit2 TON_MAX_FAULT", "TODO: bind startup timer" },
        { PMBUS_STATUS_VOUT_TOFF_MAX_WARNING, "bit1 TOFF_MAX_WARNING", "TODO: bind shutdown timer" },
        { PMBUS_STATUS_VOUT_VOUT_TRACKING_ERROR, "bit0 TRACKING_ERROR", "TODO: bind rail tracking monitor" }
    };

    pmbus_semantics_print_byte_bits("STATUS_VOUT", value, bits, (uint8_t)(sizeof(bits) / sizeof(bits[0])));
}

static void pmbus_semantics_print_status_iout(uint8_t value)
{
    static const pmbus_semantic_bit8_t bits[] =
    {
        { PMBUS_STATUS_IOUT_IOUT_OC_FAULT, "bit7 OC_FAULT", "TODO: bind current fault source" },
        { PMBUS_STATUS_IOUT_IOUT_OC_LV_FAULT, "bit6 OC_LV_FAULT", "TODO: bind low-voltage current fault" },
        { PMBUS_STATUS_IOUT_IOUT_OC_WARNING, "bit5 OC_WARNING", "TODO: bind current warning source" },
        { PMBUS_STATUS_IOUT_IOUT_UC_FAULT, "bit4 UC_FAULT", "TODO: bind under-current fault source" },
        { PMBUS_STATUS_IOUT_CURRENT_SHARE_FAULT, "bit3 SHARE_FAULT", "TODO: bind current-share monitor" },
        { PMBUS_STATUS_IOUT_IN_POWER_LIMITING_MODE, "bit2 POWER_LIMIT_MODE", "TODO: bind power-limit state" },
        { PMBUS_STATUS_IOUT_POUT_OP_FAULT, "bit1 POUT_OP_FAULT", "TODO: bind overpower fault" },
        { PMBUS_STATUS_IOUT_POUT_OP_WARNING, "bit0 POUT_OP_WARNING", "TODO: bind overpower warning" }
    };

    pmbus_semantics_print_byte_bits("STATUS_IOUT", value, bits, (uint8_t)(sizeof(bits) / sizeof(bits[0])));
}

static void pmbus_semantics_print_status_input(uint8_t value)
{
    static const pmbus_semantic_bit8_t bits[] =
    {
        { PMBUS_STATUS_INPUT_VIN_OV_FAULT, "bit7 VIN_OV_FAULT", "TODO: bind input OV fault" },
        { PMBUS_STATUS_INPUT_VIN_OV_WARNING, "bit6 VIN_OV_WARNING", "TODO: bind input OV warning" },
        { PMBUS_STATUS_INPUT_VIN_UV_WARNING, "bit5 VIN_UV_WARNING", "TODO: bind input UV warning" },
        { PMBUS_STATUS_INPUT_VIN_UV_FAULT, "bit4 VIN_UV_FAULT", "TODO: bind input UV fault" },
        { PMBUS_STATUS_INPUT_UNIT_OFF_FOR_INSUFFICIENT_INPUT_VOLTAGE, "bit3 OFF_FOR_LOW_VIN", "TODO: bind input qualification" },
        { PMBUS_STATUS_INPUT_IIN_OC_FAULT, "bit2 IIN_OC_FAULT", "TODO: bind input current fault" },
        { PMBUS_STATUS_INPUT_IIN_OC_WARNING, "bit1 IIN_OC_WARNING", "TODO: bind input current warning" },
        { PMBUS_STATUS_INPUT_PIN_OP_WARNING, "bit0 PIN_OP_WARNING", "TODO: bind input power warning" }
    };

    pmbus_semantics_print_byte_bits("STATUS_INPUT", value, bits, (uint8_t)(sizeof(bits) / sizeof(bits[0])));
}

static void pmbus_semantics_print_status_temperature(uint8_t value)
{
    static const pmbus_semantic_bit8_t bits[] =
    {
        { PMBUS_STATUS_TEMPERATURE_OT_FAULT, "bit7 OT_FAULT", "TODO: bind over-temperature fault" },
        { PMBUS_STATUS_TEMPERATURE_OT_WARNING, "bit6 OT_WARNING", "TODO: bind over-temperature warning" },
        { PMBUS_STATUS_TEMPERATURE_UT_WARNING, "bit5 UT_WARNING", "TODO: bind under-temperature warning" },
        { PMBUS_STATUS_TEMPERATURE_UT_FAULT, "bit4 UT_FAULT", "TODO: bind under-temperature fault" }
    };

    pmbus_semantics_print_byte_bits("STATUS_TEMPERATURE", value, bits, (uint8_t)(sizeof(bits) / sizeof(bits[0])));
}

static void pmbus_semantics_print_status_cml(uint8_t value)
{
    static const pmbus_semantic_bit8_t bits[] =
    {
        { PMBUS_STATUS_CML_INVALID_OR_UNSUPPORTED_COMMAND_RECEIVED, "bit7 UNSUPPORTED_CMD", "protocol handler source" },
        { PMBUS_STATUS_CML_INVALID_OR_UNSUPPORTED_DATA_RECEIVED, "bit6 UNSUPPORTED_DATA", "protocol handler source" },
        { PMBUS_STATUS_CML_PACKET_ERROR_CHECK_FAILED, "bit5 PEC_FAILED", "PEC handler source" },
        { PMBUS_STATUS_CML_MEMORY_FAULT_DETECTED, "bit4 MEMORY_FAULT", "TODO: bind product memory/NVM fault" },
        { PMBUS_STATUS_CML_PROCESSOR_FAULT_DETECTED, "bit3 PROCESSOR_FAULT", "TODO: bind MCU self-test/watchdog fault" },
        { PMBUS_STATUS_CML_OTHER_COMMUNICATION_FAULT, "bit1 OTHER_COMM", "bus recovery source" },
        { PMBUS_STATUS_CML_OTHER_MEMORY_OR_LOGIC_FAULT, "bit0 OTHER_LOGIC", "TODO: bind product logic fault" }
    };

    pmbus_semantics_print_byte_bits("STATUS_CML", value, bits, (uint8_t)(sizeof(bits) / sizeof(bits[0])));
}

static void pmbus_semantics_print_status_other(uint8_t value)
{
    static const pmbus_semantic_bit8_t bits[] =
    {
        { PMBUS_STATUS_OTHER_INPUT_A_FUSE_OR_BREAKER_FAULT, "bit5 INPUT_A_FUSE", "TODO: bind input A fuse/breaker" },
        { PMBUS_STATUS_OTHER_INPUT_B_FUSE_OR_BREAKER_FAULT, "bit4 INPUT_B_FUSE", "TODO: bind input B fuse/breaker" },
        { PMBUS_STATUS_OTHER_INPUT_A_ORING_DEVICE_FAULT, "bit3 INPUT_A_ORING", "TODO: bind input A ORing" },
        { PMBUS_STATUS_OTHER_INPUT_B_ORING_DEVICE_FAULT, "bit2 INPUT_B_ORING", "TODO: bind input B ORing" },
        { PMBUS_STATUS_OTHER_OUTPUT_ORING_DEVICE_FAULT, "bit1 OUTPUT_ORING", "TODO: bind output ORing" },
        { PMBUS_STATUS_OTHER_FIRST_TO_ASSERT_SMBALERT, "bit0 FIRST_ALERT", "TODO: bind ALERT# first-source capture" }
    };

    pmbus_semantics_print_byte_bits("STATUS_OTHER", value, bits, (uint8_t)(sizeof(bits) / sizeof(bits[0])));
}

static void pmbus_semantics_print_status_fans(uint8_t value)
{
    static const pmbus_semantic_bit8_t bits[] =
    {
        { PMBUS_STATUS_FANS_1_2_FAN_1_FAULT, "bit7 FAN1_FAULT", "TODO: bind FAN1 fault" },
        { PMBUS_STATUS_FANS_1_2_FAN_2_FAULT, "bit6 FAN2_FAULT", "TODO: bind FAN2 fault" },
        { PMBUS_STATUS_FANS_1_2_FAN_1_WARNING, "bit5 FAN1_WARNING", "TODO: bind FAN1 warning" },
        { PMBUS_STATUS_FANS_1_2_FAN_2_WARNING, "bit4 FAN2_WARNING", "TODO: bind FAN2 warning" },
        { PMBUS_STATUS_FANS_1_2_FAN_1_SPEED_OVERRIDDEN, "bit3 FAN1_OVERRIDE", "TODO: bind FAN1 override" },
        { PMBUS_STATUS_FANS_1_2_FAN_2_SPEED_OVERRIDDEN, "bit2 FAN2_OVERRIDE", "TODO: bind FAN2 override" },
        { PMBUS_STATUS_FANS_1_2_AIRFLOW_FAULT, "bit1 AIRFLOW_FAULT", "TODO: bind airflow fault" },
        { PMBUS_STATUS_FANS_1_2_AIRFLOW_WARNING, "bit0 AIRFLOW_WARNING", "TODO: bind airflow warning" }
    };

    pmbus_semantics_print_byte_bits("STATUS_FANS", value, bits, (uint8_t)(sizeof(bits) / sizeof(bits[0])));
}

static void pmbus_semantics_print_profile_bits(uint8_t command, uint16_t value, uint8_t width)
{
    uint8_t bit;
    uint16_t mask;
    const char *profile_name;

    profile_name = "PMBus Base";
#if (PMBUS_COMMAND_PROFILE == PMBUS_COMMAND_PROFILE_M_CRPS)
    profile_name = "M-CRPS";
#elif (PMBUS_COMMAND_PROFILE == PMBUS_COMMAND_PROFILE_TI_UCD90XXX)
    profile_name = "TI UCD90xxx";
#endif

    PMBUS_DEBUG_PRINT("  %s profile semantic hook cmd=0x%02X value=0x%04X\r\n",
        profile_name,
        (unsigned int)command,
        (unsigned int)value);

    for (bit = 0U; bit < width; bit++)
    {
        mask = (uint16_t)(1UL << bit);
        PMBUS_DEBUG_PRINT("    bit%u=%u -> TODO: bind profile-defined product action/status for %s\r\n",
            (unsigned int)bit,
            (unsigned int)((value & mask) != 0U),
            pmbus_semantics_command_name(command));
    }
}

static void pmbus_semantics_print_event(const pmbus_semantic_event_t *event)
{
    uint8_t value8;
    uint16_t value16;
    const char *direction_name;

    value8 = event->data[0];
    value16 = pmbus_semantics_get_u16(event);
    direction_name = "READ";
    if (event->direction == PMBUS_SEMANTIC_DIR_WRITE)
    {
        direction_name = "WRITE";
    }

    PMBUS_DEBUG_PRINT("PMBus semantic %s cmd=0x%02X (%s) proto=%u len=%u\r\n",
        direction_name,
        (unsigned int)event->command,
        pmbus_semantics_command_name(event->command),
        (unsigned int)event->protocol,
        (unsigned int)event->length);

    switch (event->command)
    {
        case PMBUS_COMMAND_OPERATION:
            pmbus_semantics_print_operation(value8);
            break;

        case PMBUS_COMMAND_ON_OFF_CONFIG:
            pmbus_semantics_print_on_off_config(value8);
            break;

        case PMBUS_COMMAND_WRITE_PROTECT:
            PMBUS_DEBUG_PRINT("  WRITE_PROTECT=0x%02X -> TODO: bind write-protect policy gating\r\n", (unsigned int)value8);
            break;

        case PMBUS_COMMAND_CAPABILITY:
            PMBUS_DEBUG_PRINT("  CAPABILITY=0x%02X -> TODO: verify CAPABILITY bits against final product feature set\r\n", (unsigned int)value8);
            break;

        case PMBUS_COMMAND_SMBALERT_MASK:
            PMBUS_DEBUG_PRINT("  SMBALERT_MASK=0x%04X -> TODO: bind mask bits to ALERT# source enable policy\r\n", (unsigned int)value16);
            break;

        case PMBUS_COMMAND_VOUT_MODE:
            pmbus_semantics_print_vout_mode(value8);
            break;

        case PMBUS_COMMAND_POWER_MODE:
            PMBUS_DEBUG_PRINT("  POWER_MODE=0x%02X -> TODO: bind power mode bits to product power-state policy\r\n", (unsigned int)value8);
            break;

        case PMBUS_COMMAND_FAN_CONFIG_1_2:
        case PMBUS_COMMAND_FAN_CONFIG_3_4:
            pmbus_semantics_print_fan_config(value8);
            break;

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
            pmbus_semantics_print_fault_response(value8);
            break;

        case PMBUS_COMMAND_STATUS_BYTE:
            pmbus_semantics_print_status_byte(value8);
            break;

        case PMBUS_COMMAND_STATUS_WORD:
            pmbus_semantics_print_status_word(value16);
            break;

        case PMBUS_COMMAND_STATUS_VOUT:
            pmbus_semantics_print_status_vout(value8);
            break;

        case PMBUS_COMMAND_STATUS_IOUT:
            pmbus_semantics_print_status_iout(value8);
            break;

        case PMBUS_COMMAND_STATUS_INPUT:
            pmbus_semantics_print_status_input(value8);
            break;

        case PMBUS_COMMAND_STATUS_TEMPERATURE:
            pmbus_semantics_print_status_temperature(value8);
            break;

        case PMBUS_COMMAND_STATUS_CML:
            pmbus_semantics_print_status_cml(value8);
            break;

        case PMBUS_COMMAND_STATUS_OTHER:
            pmbus_semantics_print_status_other(value8);
            break;

        case PMBUS_COMMAND_STATUS_MFR_SPECIFIC:
            PMBUS_DEBUG_PRINT("  STATUS_MFR_SPECIFIC=0x%02X -> TODO: bind profile-specific manufacturer fault bits\r\n", (unsigned int)value8);
            break;

        case PMBUS_COMMAND_STATUS_FANS_1_2:
        case PMBUS_COMMAND_STATUS_FANS_3_4:
            pmbus_semantics_print_status_fans(value8);
            break;

        default:
            if (event->length >= 2U)
            {
                pmbus_semantics_print_profile_bits(event->command, value16, 16U);
            }
            else
            {
                pmbus_semantics_print_profile_bits(event->command, (uint16_t)value8, 8U);
            }
            break;
    }
}

void pmbus_semantics_background_task(void)
{
#if PMBUS_DEBUG_ENABLE && PMBUS_DEBUG_PRINT_SEMANTICS
    pmbus_semantic_event_t event;
    uint8_t irq_state;
    uint8_t has_event;
    uint8_t drop_count;

    do
    {
        has_event = 0U;
        drop_count = 0U;
        irq_state = pmbus_io_irq_save_disable();
        if (g_semantic_tail != g_semantic_head)
        {
            event = g_semantic_queue[g_semantic_tail];
            g_semantic_tail = (uint8_t)(g_semantic_tail + 1U);
            if (g_semantic_tail >= PMBUS_SEMANTIC_QUEUE_SIZE)
            {
                g_semantic_tail = 0U;
            }
            has_event = 1U;
        }
        else if (g_semantic_drop_count != 0U)
        {
            drop_count = g_semantic_drop_count;
            g_semantic_drop_count = 0U;
        }
        pmbus_io_irq_restore(irq_state);

        if (has_event != 0U)
        {
            pmbus_semantics_print_event(&event);
        }
        else if (drop_count != 0U)
        {
            PMBUS_DEBUG_PRINT("PMBus semantic log dropped %u event(s); increase PMBUS_SEMANTIC_QUEUE_SIZE or reduce checklist burst rate\r\n",
                (unsigned int)drop_count);
        }
    } while (has_event != 0U);
#endif
}
