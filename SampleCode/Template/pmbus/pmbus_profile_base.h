#ifndef PMBUS_PROFILE_BASE_H
#define PMBUS_PROFILE_BASE_H

static const char *pmbus_profile_base_get_command_name(uint8_t command, char *buffer, uint8_t buffer_size)
{
    if ((command >= PMBUS_COMMAND_USER_DATA_00) &&
        (command <= PMBUS_COMMAND_USER_DATA_15))
    {
        if ((buffer != (char *)0) && (buffer_size >= 13U))
        {
            sprintf(buffer, "USER_DATA_%02u", (unsigned int)(command - PMBUS_COMMAND_USER_DATA_00));
            return buffer;
        }
        return "USER_DATA";
    }

    if ((command >= 0xC0U) &&
        (command <= PMBUS_COMMAND_MFR_SPECIFIC_FD))
    {
        if ((buffer != (char *)0) && (buffer_size >= 17U))
        {
            sprintf(buffer, "MFR_SPECIFIC_%02X", (unsigned int)command);
            return buffer;
        }
        return "MFR_SPECIFIC";
    }

    if (command == PMBUS_COMMAND_MFR_SPECIFIC_COMMAND_EXT)
    {
        return "MFR_SPECIFIC_COMMAND_EXT";
    }

    if (command == PMBUS_COMMAND_PMBUS_COMMAND_EXT)
    {
        return "PMBUS_COMMAND_EXT";
    }

    return (const char *)0;
}

#endif
