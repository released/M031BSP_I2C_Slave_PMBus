#ifndef PMBUS_PROFILE_NAMES_H
#define PMBUS_PROFILE_NAMES_H

#include <stdio.h>
#include "pmbus_cfg_user.h"
#include "pmbus_protocol.h"
#include "pmbus_profile_base.h"

#if (PMBUS_COMMAND_PROFILE == PMBUS_COMMAND_PROFILE_M_CRPS)
#include "pmbus_profile_mcrps.h"
#elif (PMBUS_COMMAND_PROFILE == PMBUS_COMMAND_PROFILE_TI_UCD90XXX)
#include "pmbus_profile_ti_ucd90xxx.h"
#endif

static const char *pmbus_profile_get_command_name(uint8_t command, char *buffer, uint8_t buffer_size)
{
#if (PMBUS_COMMAND_PROFILE == PMBUS_COMMAND_PROFILE_M_CRPS)
    return pmbus_profile_mcrps_get_command_name(command, buffer, buffer_size);
#elif (PMBUS_COMMAND_PROFILE == PMBUS_COMMAND_PROFILE_TI_UCD90XXX)
    return pmbus_profile_ti_ucd90xxx_get_command_name(command, buffer, buffer_size);
#else
    return pmbus_profile_base_get_command_name(command, buffer, buffer_size);
#endif
}

#endif
