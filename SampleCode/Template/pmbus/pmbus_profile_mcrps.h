#ifndef PMBUS_PROFILE_MCRPS_H
#define PMBUS_PROFILE_MCRPS_H

static const char *pmbus_profile_mcrps_get_command_name(uint8_t command, char *buffer, uint8_t buffer_size)
{
    switch (command)
    {
    case 0xB0U: return "USER_DATA_00";
    case 0xB1U: return "USER_DATA_01";
    case 0xB2U: return "USER_DATA_02";
    case 0xB3U: return "MFR_EFFICIENCY_DATA";
    case 0xB4U: return "USER_DATA_04";
    case 0xB8U: return "USER_DATA_08";
    case 0xC0U: return "MFR_MAX_TEMP_1";
    case 0xC1U: return "MFR_MAX_TEMP_2";
    case 0xC2U: return "MFR_MAX_TEMP_3";
    case 0xD0U: return "MFR_COLD_REDUNDANCY_CONFIG";
    case 0xD1U: return "MFR_READ_CONFIG_FILE_SIZE";
    case 0xD2U: return "MFR_READ_CONFIG_BLOCK_SIZE";
    case 0xD3U: return "MFR_READ_CONFIG_FILE";
    case 0xD4U: return "MFR_HW_COMPATIBILITY";
    case 0xD5U: return "MFR_FWUPLOAD_CAPABILITY";
    case 0xD6U: return "MFR_FWUPLOAD_MODE";
    case 0xD7U: return "MFR_FWUPLOAD";
    case 0xD8U: return "MFR_FWUPLOAD_STATUS";
    case 0xD9U: return "MFR_FW_REVISION";
    case 0xDAU: return "MFR_SPDM";
    case 0xDBU: return "MFR_FRU_PROTECTION";
    case 0xDCU: return "MFR_BLACKBOX";
    case 0xDDU: return "MFR_REAL_TIME_BLACK_BOX";
    case 0xDEU: return "MFR_SYSTEM_BLACK_BOX";
    case 0xDFU: return "MFR_BLACK_BOX_CONFIG";
    case 0xE0U: return "MFR_CLEAR_BLACK_BOX";
    case 0xE1U: return "MFR_LINE_STATUS";
    case 0xE2U: return "MFR_SYSTEM_LED_CNTL";
    case 0xE3U: return "MFR_FWUPLOAD_BLOCK_SIZE";
    case 0xE4U: return "MFR_EN_STATUS_SIMULATION_CMD";
    case 0xE9U: return "MFR_PEAK_CURRENT_RECORD";
    case 0xEBU: return "MFR_COMPONENT_ID";
    case 0xECU: return "MFR_TOT_POUT_MAX";
    case 0xEDU: return "MFR_VOUT_MARGINING";
    case 0xEEU: return "MFR_OCWPL1_SETTING";
    case 0xF0U: return "MFR_PWOK_WARNING_TIME";
    case 0xF1U: return "MFR_MAX_IOUT_CAPABILITY";
    case 0xF2U: return "MFR_FPC_MAIN_MIN_OFF_TIME";
    case 0xF3U: return "MFR_FPC_12VSB_MIN_OFF_TIME";
    case 0xFEU: return "MFR_SPECIFIC_COMMAND_EXT";
    case 0xFFU: return "PMBUS_COMMAND_EXT";
    default:
        return pmbus_profile_base_get_command_name(command, buffer, buffer_size);
    }
}

#endif
