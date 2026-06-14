# M031BSP_I2C_Slave_PMBus

Nuvoton M031 PMBus/SMBus slave firmware validation.

Last updated: 2026/06/14

## Overview

The firmware focuses on a standards-aligned PMBus slave transport path:

- SMBus/PMBus slave addressing and repeated START handling
- PEC generation and validation using CRC-8 polynomial 0x07
- SMBALERT#/ARA support path
- PMBus command dispatch aligned to PMBus 1.3.1 Part II command summary
- Runtime debug logs for RX decode and TX payload traceability
- Host-visible placeholder/shadow values for commands that are not yet connected to real PSU control or telemetry sources

## Target Hardware

| Item | Value |
| --- | --- |
| MCU | Nuvoton M031 series |
| Project board | M031 EVB or compatible custom M031 board |
| PMBus role | SMBus/PMBus slave |
| Default slave address | 0x5A, 7-bit |
| PMBus bus speed | 400 kHz target |
| Toolchain | Keil uVision5 with ARM Compiler 6 |
| Debug UART | UART0, 115200 8N1 |

## Pin Map

| Signal | Pin | Direction | Notes |
| --- | --- | --- | --- |
| PMBUS_SCL | PB5 | Input/output | I2C0 SCL, open-drain, external pull-up required |
| PMBUS_SDA | PB4 | Input/output | I2C0 SDA, open-drain, external pull-up required |
| PMBUS_ALERT# | PB6 | Output | Active-low SMBALERT#, open-drain, external pull-up required |
| UART0_RXD | PB12 | Input | Debug UART RX |
| UART0_TXD | PB13 | Output | Debug UART TX |
| HEARTBEAT | PB14 | Output | 1 second heartbeat toggle |
| GPIO_SPARE | PB15 | Output | Initialized spare output |

## Repository Layout

```text
Library/                                   Nuvoton BSP and driver library
SampleCode/Template/main.c                 Main firmware entry point
SampleCode/Template/board_config.h         Board-level pin and PMBus defaults
SampleCode/Template/misc_config.*          Clock, UART, GPIO, timer setup
SampleCode/Template/pmbus_io.*             Platform glue for PMBus IO behavior
SampleCode/Template/pmbus/                 PMBus protocol, dispatch, and platform code
SampleCode/Template/Keil/Template.uvprojx  Keil project
SampleCode/Template/PMBUS_SUPPORT_MATRIX.md
SampleCode/Template/PMBUS_VALIDATION_CHECKLIST.md
```

## Build

Open the Keil project:

```text
SampleCode/Template/Keil/Template.uvprojx
```

Expected build outputs:

```text
SampleCode/Template/Keil/obj/template.axf
SampleCode/Template/Keil/obj/template.hex
SampleCode/Template/Keil/obj/template.bin
```

## Runtime Behavior

At startup, the firmware initializes system clock, GPIO, UART0, Timer1, SysTick, timer service, and PMBus slave service.

The PMBus bus-critical path is handled in the I2C/PMBus interrupt path. 

Background code is used for debug printing and non-critical housekeeping only. 

This is intentional: SLA+W, SLA+R, repeated START, STOP, PEC, and TX byte preparation must not depend on slow background logging.

The main loop dispatches:

- Timer service tasks
- PMBus platform background task
- PMBus driver background task
- UART console reset commands

UART console reset commands:

```text
x, X, z, Z -> SYS_ResetChip()
```

## PMBus / SMBus Support

The implementation is intended to align with these documents:

- `docs/PMBus-Specification-Rev-1-3-1-Part-II-20150313.pdf` in the Pico HID Test Tool workspace
- `docs/PMBus_rev_1.2_part_1_september_2010.pdf` in the Pico HID Test Tool workspace

Supported transaction formats include:

- Send Byte
- Receive Byte
- Write Byte
- Write Word
- Read Byte
- Read Word
- Read 32
- Block Write
- Block Read
- Process Call
- Block Write-Read Process Call
- Group Command
- PEC enable/disable behavior
- SMBALERT#/ARA flow

Command support and validation status are tracked in:

```text
SampleCode/Template/PMBUS_SUPPORT_MATRIX.md
SampleCode/Template/PMBUS_VALIDATION_CHECKLIST.md
```

## Important Product Note

Some PMBus commands currently return fixed values or volatile shadow values. 

These are useful for host-side protocol validation, 

but they are not final product behavior until connected to real product telemetry, control logic, fault sources, non-volatile storage, or an approved product policy.

Fixed-value or shadow-backed commands should keep source comments so future firmware work can trace where real product values must be connected.

## Typical Validation Setup

Hardware wiring with the Pico HID Test Tool as PMBus master:

| Pico signal | Pico pin | M031 signal | M031 pin |
| --- | --- | --- | --- |
| PMBus SDA | GP20 | PMBUS_SDA | PB4 |
| PMBus SCL | GP21 | PMBUS_SCL | PB5 |
| PMBus ALERT# | GP14 | PMBUS_ALERT# | PB6 |
| GND | GND | GND | GND |

Recommended validation flow:

1. Program the M031 firmware.
2. Open UART0 debug log at 115200 8N1.
3. Connect Pico HID Test Tool.
4. Open the PMBus tab.
5. Set address to `0x5A`.
6. Enable PEC.
7. Enable PMBus master.
8. Run `Scan`.
9. Run quick-test groups in order: `Basic`, `PEC`, `Error`, `Telemetry`, `Full`.
10. Confirm the tool log and MCU UART log match expected command, protocol, PEC, and payload behavior.

## Expected Validation Signals

A healthy scan should identify the device and read at least:

- `PMBUS_REVISION`
- `MFR_ID`
- `MFR_MODEL`

A healthy basic checklist should pass bus ACK, repeated START read, common write/readback, VOUT mode, and status reads.

A healthy PEC checklist should pass PEC-enabled read byte, read word, block read, and bad-PEC negative-path behavior.

A healthy telemetry checklist should decode fixed or shadow telemetry values and report PEC OK.

Manual checklist items remain manual when they require external board behavior, power-stage behavior, or logic-analyzer confirmation.

## Configuration Files

Primary configuration points:

```text
SampleCode/Template/board_config.h
SampleCode/Template/pmbus/pmbus_cfg_common.h
```

Common settings include:

```c
PMBUS_BOARD_SCL_PIN
PMBUS_BOARD_SDA_PIN
PMBUS_BOARD_ALERT_PIN
PMBUS_DEFAULT_ADDRESS_A0_LEVEL
PMBUS_DEFAULT_ADDRESS_A1_LEVEL
PMBUS_ENABLE_PEC
PMBUS_ENABLE_DEBUG_RX
PMBUS_ENABLE_DEBUG_TX
PMBUS_ENABLE_SLAVE_RECOVER
PMBUS_ENABLE_ARA_ALIAS
```

## Debug Logging

Debug logging is intentionally human-readable and command-aware. RX debug frames include command byte and command name when known. TX debug logs should print decoded values for telemetry commands so GUI logs can be correlated with MCU-side source values.

Do not print from timing-critical ISR paths. Queue or capture data in ISR and print from background processing.

![log 1](log_1.jpg)

![log 2](log_2.jpg)

![log 3](log_3.jpg)

![log 4](log_4.jpg)


## Known Constraints

- External pull-ups are required on PMBus SCL, SDA, and ALERT#.
- Debug logging can change timing if used excessively; keep bus-critical behavior in ISR.
- Fixed-value telemetry must be replaced by real ADC/control data before product release.
- Non-volatile behavior for STORE/RESTORE/USER_DATA/MFR policy must be finalized by product requirements.

## Related Validation Documents

```text
SampleCode/Template/PMBUS_SUPPORT_MATRIX.md
SampleCode/Template/PMBUS_VALIDATION_CHECKLIST.md
Pico HID Test Tool repository: docs/PMBUS_TABLE31_GAP_MATRIX.md
```

## Revision History

| Date | Change |
| --- | --- |
| 2026/06/14 | Initial GitHub README draft created from MCU README template. |
