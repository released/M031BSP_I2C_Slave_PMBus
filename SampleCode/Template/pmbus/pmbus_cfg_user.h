#ifndef PMBUS_CFG_USER_H
#define PMBUS_CFG_USER_H

/*
    All PMBus addresses in this file are 7-bit addresses. Convert to the
    on-wire 8-bit address only at the I2C transaction boundary.
*/
#define PMBUS_ADDRESS_7BIT_BASE               0x58U
#define PMBUS_ALERT_RESPONSE_ADDRESS_7BIT     0x0CU
#define PMBUS_ARP_DEFAULT_ADDRESS_7BIT        0x61U
#define PMBUS_ZONE_READ_ADDRESS_7BIT          0x28U
#define PMBUS_ZONE_WRITE_ADDRESS_7BIT         0x37U

/*
    Slot strap address table. Platform code reads A1/A0 and maps:
    00 -> 0x58, 01 -> 0x59, 10 -> 0x5A, 11 -> 0x5B.
    The fallback is used when the strap source is disabled or invalid.
*/
#define PMBUS_ADDRESS_STRAP_00_7BIT           0x58U
#define PMBUS_ADDRESS_STRAP_01_7BIT           0x59U
#define PMBUS_ADDRESS_STRAP_10_7BIT           0x5AU
#define PMBUS_ADDRESS_STRAP_11_7BIT           0x5BU
#define PMBUS_ADDRESS_INVALID_FALLBACK_7BIT   PMBUS_ADDRESS_STRAP_10_7BIT

#define PMBUS_ADDRESS_7BIT_TO_WRITE(addr7)    ((unsigned char)((addr7) << 1))
#define PMBUS_ADDRESS_7BIT_TO_READ(addr7)     ((unsigned char)(((addr7) << 1) | 0x01U))

/*
    Fixed transaction buffers. No dynamic allocation is used.
    TX size is block-size plus count/PEC overhead.
*/
#define PMBUS_RX_BUFFER_SIZE                  40U
#define PMBUS_TX_BUFFER_SIZE                  34U
#define PMBUS_MAX_BLOCK_SIZE                  32U
#define PMBUS_DEBUG_QUEUE_SIZE                16U

#define PMBUS_PROFILE_MINIMAL                 1U
#define PMBUS_PROFILE_FULL                    2U

#define PMBUS_COMMAND_PROFILE_BASE            1U
#define PMBUS_COMMAND_PROFILE_M_CRPS          2U
#define PMBUS_COMMAND_PROFILE_TI_UCD90XXX     3U

/*
    Command profile selection controls profile-specific command names and
    policy namespace interpretation. It is intentionally separate from
    PMBUS_PROFILE_MINIMAL/FULL, which controls feature size.
    - BASE: PMBus specification names for USER_DATA/MFR_SPECIFIC ranges.
    - M_CRPS: M-CRPS public profile names and CRPS overlay rows.
    - TI_UCD90XXX: TI UCD90xxx profile display names; target-specific
      device emulation remains product/application policy.
*/
#ifndef PMBUS_COMMAND_PROFILE
#define PMBUS_COMMAND_PROFILE                 PMBUS_COMMAND_PROFILE_BASE
#endif

/*
    Profile selection:
    - PMBUS_PROFILE_FULL keeps the full PoC command surface enabled.
    - PMBUS_PROFILE_MINIMAL keeps the common discovery, status,
      basic telemetry, and MFR identity commands for smaller flash targets.
    Individual PMBUS_ENABLE_CMD_* switches may still be overridden by the
    compiler command line or a platform config before this file is included.
*/
#ifndef PMBUS_PROFILE
#define PMBUS_PROFILE                         PMBUS_PROFILE_FULL
#endif

/*
    Profile default switches. These only provide defaults for the
    PMBUS_ENABLE_CMD_* switches below; each command group can still be
    overridden explicitly before including this file.
*/
#if (PMBUS_PROFILE == PMBUS_PROFILE_MINIMAL)
#define PMBUS_PROFILE_DEFAULT_CMD_CORE        1U
#define PMBUS_PROFILE_DEFAULT_CMD_STATUS      1U
#define PMBUS_PROFILE_DEFAULT_CMD_TELEMETRY   1U
#define PMBUS_PROFILE_DEFAULT_CMD_LIMITS      0U
#define PMBUS_PROFILE_DEFAULT_CMD_FAN         0U
#define PMBUS_PROFILE_DEFAULT_CMD_ENERGY      0U
#define PMBUS_PROFILE_DEFAULT_CMD_MFR_BASIC   1U
#define PMBUS_PROFILE_DEFAULT_CMD_MFR_EXT     0U
#define PMBUS_PROFILE_DEFAULT_CMD_PAGE_PLUS   0U
#define PMBUS_PROFILE_DEFAULT_CMD_COEFFICIENTS 1U
#define PMBUS_PROFILE_DEFAULT_CMD_ZONE        0U
#define PMBUS_PROFILE_DEFAULT_CMD_POLICY      0U
#define PMBUS_PROFILE_DEFAULT_CMD_FWUPLOAD    0U
#define PMBUS_PROFILE_DEFAULT_ARP             0U
#define PMBUS_PROFILE_DEFAULT_ZONE_ALIAS      0U
#else
#define PMBUS_PROFILE_DEFAULT_CMD_CORE        1U
#define PMBUS_PROFILE_DEFAULT_CMD_STATUS      1U
#define PMBUS_PROFILE_DEFAULT_CMD_TELEMETRY   1U
#define PMBUS_PROFILE_DEFAULT_CMD_LIMITS      1U
#define PMBUS_PROFILE_DEFAULT_CMD_FAN         1U
#define PMBUS_PROFILE_DEFAULT_CMD_ENERGY      1U
#define PMBUS_PROFILE_DEFAULT_CMD_MFR_BASIC   1U
#define PMBUS_PROFILE_DEFAULT_CMD_MFR_EXT     1U
#define PMBUS_PROFILE_DEFAULT_CMD_PAGE_PLUS   1U
#define PMBUS_PROFILE_DEFAULT_CMD_COEFFICIENTS 1U
#define PMBUS_PROFILE_DEFAULT_CMD_ZONE        1U
#define PMBUS_PROFILE_DEFAULT_CMD_POLICY      1U
#define PMBUS_PROFILE_DEFAULT_CMD_FWUPLOAD    1U
#define PMBUS_PROFILE_DEFAULT_ARP             1U
#define PMBUS_PROFILE_DEFAULT_ZONE_ALIAS      1U
#endif

/*
    Command group switches. Use 1U to compile the group in, 0U to remove it
    from the descriptor table, dispatcher, and related application storage.
    - CORE: PAGE, OPERATION, ON_OFF_CONFIG, CLEAR_FAULTS, CAPABILITY,
      PMBUS_REVISION, VOUT_MODE, VOUT_COMMAND, and basic store/restore/query.
    - STATUS: STATUS_BYTE/WORD and grouped status registers.
    - TELEMETRY: READ_VIN/IIN/VOUT/IOUT/temperature/power/frequency data.
    - LIMITS: voltage/current/input/temperature/power limit and response
      commands.
    - FAN: fan config, fan command, and fan speed read commands.
    - ENERGY: READ_EIN/EOUT and kWh accumulator commands.
    - MFR_BASIC: required identity strings such as MFR_ID/MODEL/REVISION/SERIAL.
    - MFR_EXT: optional manufacturer data, blackbox, cold redundancy, and
      extended identity/capability commands.
    - PAGE_PLUS: PAGE_PLUS_READ/WRITE support.
    - COEFFICIENTS: Direct-format coefficient support.
    - ZONE: PMBus Zone config/active commands.
    - POLICY: USER_DATA and SMBALERT_MASK policy table support.
    - FWUPLOAD: placeholder manufacturer firmware upload command set.
*/
#ifndef PMBUS_ENABLE_CMD_CORE
#define PMBUS_ENABLE_CMD_CORE                 PMBUS_PROFILE_DEFAULT_CMD_CORE
#endif

#ifndef PMBUS_ENABLE_CMD_STATUS
#define PMBUS_ENABLE_CMD_STATUS               PMBUS_PROFILE_DEFAULT_CMD_STATUS
#endif

#ifndef PMBUS_ENABLE_CMD_TELEMETRY
#define PMBUS_ENABLE_CMD_TELEMETRY            PMBUS_PROFILE_DEFAULT_CMD_TELEMETRY
#endif

#ifndef PMBUS_ENABLE_CMD_LIMITS
#define PMBUS_ENABLE_CMD_LIMITS               PMBUS_PROFILE_DEFAULT_CMD_LIMITS
#endif

#ifndef PMBUS_ENABLE_CMD_FAN
#define PMBUS_ENABLE_CMD_FAN                  PMBUS_PROFILE_DEFAULT_CMD_FAN
#endif

#ifndef PMBUS_ENABLE_CMD_ENERGY
#define PMBUS_ENABLE_CMD_ENERGY               PMBUS_PROFILE_DEFAULT_CMD_ENERGY
#endif

#ifndef PMBUS_ENABLE_CMD_MFR_BASIC
#define PMBUS_ENABLE_CMD_MFR_BASIC            PMBUS_PROFILE_DEFAULT_CMD_MFR_BASIC
#endif

#ifndef PMBUS_ENABLE_CMD_MFR_EXT
#define PMBUS_ENABLE_CMD_MFR_EXT              PMBUS_PROFILE_DEFAULT_CMD_MFR_EXT
#endif

#ifndef PMBUS_ENABLE_CMD_PAGE_PLUS
#define PMBUS_ENABLE_CMD_PAGE_PLUS            PMBUS_PROFILE_DEFAULT_CMD_PAGE_PLUS
#endif

#ifndef PMBUS_ENABLE_CMD_COEFFICIENTS
#define PMBUS_ENABLE_CMD_COEFFICIENTS         PMBUS_PROFILE_DEFAULT_CMD_COEFFICIENTS
#endif

#ifndef PMBUS_ENABLE_CMD_ZONE
#define PMBUS_ENABLE_CMD_ZONE                 PMBUS_PROFILE_DEFAULT_CMD_ZONE
#endif

#ifndef PMBUS_ENABLE_CMD_POLICY
#define PMBUS_ENABLE_CMD_POLICY               PMBUS_PROFILE_DEFAULT_CMD_POLICY
#endif

#ifndef PMBUS_ENABLE_CMD_FWUPLOAD
#define PMBUS_ENABLE_CMD_FWUPLOAD             PMBUS_PROFILE_DEFAULT_CMD_FWUPLOAD
#endif

#ifndef PMBUS_ENABLE_CMD_CRPS
#if (PMBUS_COMMAND_PROFILE == PMBUS_COMMAND_PROFILE_M_CRPS)
#define PMBUS_ENABLE_CMD_CRPS                 PMBUS_ENABLE_CMD_MFR_EXT
#else
#define PMBUS_ENABLE_CMD_CRPS                 0U
#endif
#endif

#define PMBUS_PEC_POLICY_DISABLED             0U
#define PMBUS_PEC_POLICY_OPTIONAL             1U
#define PMBUS_PEC_POLICY_REQUIRED             2U
/*
    PEC policy selection:
    - PMBUS_PEC_POLICY_OPTIONAL: PEC is enabled, but the host is not forced
      to send PEC. Incoming PEC is validated when present, and read responses
      include PEC.
    - PMBUS_PEC_POLICY_REQUIRED: PEC is enabled and write-side transactions
      without valid PEC are rejected. Use this for strict production profiles.
    - PMBUS_PEC_POLICY_DISABLED: PEC is disabled. Use only for explicit
      bring-up tests.
*/
#ifndef PMBUS_PEC_POLICY
#define PMBUS_PEC_POLICY                      PMBUS_PEC_POLICY_OPTIONAL
#endif
#define PMBUS_ENABLE_PEC                      ((PMBUS_PEC_POLICY) != PMBUS_PEC_POLICY_DISABLED)

#define PMBUS_PEC_BACKEND_SOFTWARE            0U
#define PMBUS_PEC_BACKEND_HW_CRC              1U
/*
    PEC CRC backend selection:
    - PMBUS_PEC_BACKEND_SOFTWARE uses the portable bitwise CRC-8 code.
    - PMBUS_PEC_BACKEND_HW_CRC uses the platform hardware CRC peripheral.
      It is the default for M031 and does not require linking StdDriver crc.c.
*/
#ifndef PMBUS_PEC_BACKEND
#define PMBUS_PEC_BACKEND                     PMBUS_PEC_BACKEND_HW_CRC
#endif

/*
    Debug output switches. Debug is printed from background context only, not
    from the I2C ISR. Disable individual items when UART bandwidth affects
    validation logs.
*/
#define PMBUS_DEBUG_ENABLE                    1U
#define PMBUS_DEBUG_PRINT_RX_FRAME            1U
#define PMBUS_DEBUG_PRINT_TX_READY            1U
#define PMBUS_DEBUG_PRINT_TX_DECODE           1U
#define PMBUS_DEBUG_PRINT_WRITE_DONE          1U
#define PMBUS_DEBUG_PRINT_SEMANTICS           0U
#define PMBUS_DEBUG_PRINT_STATUS              0U

#ifndef PMBUS_SEMANTIC_QUEUE_SIZE
#define PMBUS_SEMANTIC_QUEUE_SIZE             16U
#endif

#define PMBUS_SYSTEM_POLICY_CRPS_DEFAULT      1U
#define PMBUS_SYSTEM_POLICY_LAB_VALIDATION    2U
/*
    System policy selection:
    - CRPS_DEFAULT keeps optional SMBus/PMBus lab helpers disabled unless a
      final product requirement explicitly enables them.
    - LAB_VALIDATION keeps ARA/ARP helper paths available so the Pico PMBus
      tester can validate edge cases during bring-up.
*/
#ifndef PMBUS_SYSTEM_POLICY
#define PMBUS_SYSTEM_POLICY                   PMBUS_SYSTEM_POLICY_LAB_VALIDATION
#endif

#if (PMBUS_SYSTEM_POLICY == PMBUS_SYSTEM_POLICY_CRPS_DEFAULT)
#define PMBUS_POLICY_DEFAULT_ARA_ALIAS        0U
#define PMBUS_POLICY_DEFAULT_ARP              0U
#else
#define PMBUS_POLICY_DEFAULT_ARA_ALIAS        1U
#define PMBUS_POLICY_DEFAULT_ARP              PMBUS_PROFILE_DEFAULT_ARP
#endif

/*
    Runtime protocol service switches:
    - PMBUS_ENABLE_SLAVE_RECOVER enables I2C slave bus-clear/re-init recovery.
    - PMBUS_ENABLE_ARA_ALIAS enables the lab Alert Response Address workaround
      when the MCU cannot expose a true second hardware slave address.
    - PMBUS_ENABLE_ARP enables SMBus ARP/UDID command handling.
    - PMBUS_ENABLE_ZONE_ALIAS enables extra Zone read/write I2C alias address
      handling when supported by the platform I2C port.
*/
#define PMBUS_ENABLE_SLAVE_RECOVER            1U

#ifndef PMBUS_ENABLE_ARA_ALIAS
#define PMBUS_ENABLE_ARA_ALIAS                PMBUS_POLICY_DEFAULT_ARA_ALIAS
#endif

#ifndef PMBUS_ENABLE_ARP
#define PMBUS_ENABLE_ARP                      PMBUS_POLICY_DEFAULT_ARP
#endif

#ifndef PMBUS_ENABLE_ZONE_ALIAS
#define PMBUS_ENABLE_ZONE_ALIAS               PMBUS_PROFILE_DEFAULT_ZONE_ALIAS
#endif

/*
    Hardware alias slot assignment. Use DISABLED when the platform I2C slave
    cannot expose that alias. ARA may fall back to a software alias workaround.
*/
#define PMBUS_I2C_ALIAS_SLOT_DISABLED         0xFFU
#define PMBUS_I2C_ALIAS_SLOT_ARA              1U
#define PMBUS_I2C_ALIAS_SLOT_ARP              2U
#define PMBUS_I2C_ALIAS_SLOT_ZONE_READ        3U
#define PMBUS_I2C_ALIAS_SLOT_ZONE_WRITE       PMBUS_I2C_ALIAS_SLOT_DISABLED

/*
    Bus recovery thresholds. These values control how aggressively the slave
    releases a stuck bus and re-initializes the I2C peripheral.
*/
#define PMBUS_I2C_BUS_CLEAR_PULSES                   9U
#define PMBUS_I2C_BUS_CLEAR_RETRY_COUNT              3U
#define PMBUS_I2C_RECOVER_MAX_ATTEMPTS               3U
#define PMBUS_I2C_RECOVER_BACKOFF_CYCLES             2U
#define PMBUS_I2C_STUCK_BUS_RETRY_CYCLES             8U
#define PMBUS_I2C_CLOCK_LOW_TIMEOUT_MS               35U
#define PMBUS_I2C_TIMEOUT_RECOVER_THRESHOLD          1U
#define PMBUS_I2C_BUS_ERROR_RECOVER_THRESHOLD        1U

#endif
