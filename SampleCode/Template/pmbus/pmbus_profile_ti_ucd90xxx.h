#ifndef PMBUS_PROFILE_TI_UCD90XXX_H
#define PMBUS_PROFILE_TI_UCD90XXX_H

static const char *pmbus_profile_ti_ucd90xxx_get_command_name(uint8_t command, char *buffer, uint8_t buffer_size)
{
    switch (command)
    {
    case 0xB5U: return "BLACK_BOX_FAULT_INFO";
    case 0xB6U: return "BLACK_BOX_FAULT_RAILS_WARNING";
    case 0xB7U: return "BLACK_BOX_LOG_RAILS_VALUE";
    case 0xB9U: return "RAIL_STATE";
    case 0xD0U: return "SEQ_TIMEOUT";
    case 0xD1U: return "VOUT_CAL_MONITOR";
    case 0xD2U: return "SYSTEM_RESET_CONFIG";
    case 0xD3U: return "SYSTEM_WATCHDOG_CONFIG";
    case 0xD4U: return "SYSTEM_WATCHDOG_RESET";
    case 0xD5U: return "MONITOR_CONFIG";
    case 0xD6U: return "NUM_PAGES";
    case 0xD7U: return "RUN_TIME_CLOCK";
    case 0xD8U: return "RUN_TIME_CLOCK_TRIM";
    case 0xD9U: return "ROM_MODE";
    case 0xDAU: return "USER_RAM_00";
    case 0xDBU: return "SOFT_RESET";
    case 0xDCU: return "RESET_COUNT";
    case 0xDDU: return "PIN_SELECTED_RAIL_STATES";
    case 0xDEU: return "RESEQUENCE";
    case 0xDFU: return "CONSTANTS";
    case 0xE0U: return "PWM_SELECT";
    case 0xE1U: return "PWM_CONFIG";
    case 0xE2U: return "PARM_INFO";
    case 0xE3U: return "PARM_VALUE";
    case 0xE4U: return "TEMPERATURE_CAL_GAIN";
    case 0xE5U: return "TEMPERATURE_CAL_OFFSET";
    case 0xE7U: return "FAN_CONFIG_INDEX";
    case 0xE8U: return "FAN_CONFIG";
    case 0xE9U: return "FAULT_RESPONSES";
    case 0xEAU: return "LOGGED_FAULTS";
    case 0xEBU: return "LOGGED_FAULT_DETAIL_INDEX";
    case 0xECU: return "LOGGED_FAULT_DETAIL";
    case 0xEDU: return "LOGGED_PAGE_PEAKS";
    case 0xEEU: return "LOGGED_COMMON_PEAKS";
    case 0xEFU: return "LOG_FAULT_DETAIL_ENABLES";
    case 0xF0U: return "EXECUTE_FLASH";
    case 0xF1U: return "SECURITY";
    case 0xF2U: return "SECURITY_BIT_MASK";
    case 0xF3U: return "MFR_STATUS";
    case 0xF4U: return "GPI_FAULT_RESPONSES";
    case 0xF5U: return "MARGIN_CONFIG";
    case 0xF6U: return "SEQ_CONFIG";
    case 0xF7U: return "GPO_CONFIG_INDEX";
    case 0xF8U: return "GPO_CONFIG";
    case 0xF9U: return "GPI_CONFIG";
    case 0xFAU: return "GPIO_SELECT";
    case 0xFBU: return "GPIO_CONFIG";
    case 0xFCU: return "MISC_CONFIG";
    case 0xFDU: return "DEVICE_ID";
    default:
        return pmbus_profile_base_get_command_name(command, buffer, buffer_size);
    }
}

#endif
