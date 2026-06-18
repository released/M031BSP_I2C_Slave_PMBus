# PMBus Validation Checklist

This checklist is derived from `PMBUS_SUPPORT_MATRIX.md`.

Compliance target:
- Validate this M032 PMBus slave against:
  - `PMBus-Specification-Rev-1-3-1-Part-II-20150313.pdf`
  - `PMBus_rev_1.2_part_1_september_2010.pdf`
- A PASS means the observed bus protocol, payload, PEC, status/error response, and side effect match the documented behavior.
- If a command is currently a placeholder/shadow, PASS only covers host-visible communication behavior; product-complete CRPS behavior remains open until the real source/control path is connected.
- Keep this checklist synchronized with the Pico GUI `Full` checklist and `PMBUS_SUPPORT_MATRIX.md`.
- Keep command existence, protocol, placeholder status, and policy gaps synchronized with:
  - Pico HID Test Tool repository: `docs/PMBUS_COMMAND_CONTRACT.csv`
  - Pico HID Test Tool repository: `docs/PMBUS_COMMAND_CONTRACT.md`

Use assumptions:
- 7-bit slave address from `PAGE` device is `0x5A` unless A0/A1 strap changes it.
- `Write address byte = 0xB4`, `Read address byte = 0xB5`.
- PEC behavior follows `PMBUS_PEC_POLICY`; optional mode accepts host PEC when present and always generates slave read PEC.
- Logic analyzer should decode I2C/SMBus with repeated START enabled.

## 1. Bus Basics

- Verify `SCL` and `SDA` idle high.
- Verify slave ACK on:
  - `SLA+W`
  - `SLA+R`
- Verify repeated START path:
  - `S B4 A <CMD> A Sr B5 A ...`
- Verify no debug `printf` occurs inside the ISR path.

## 2. Byte Read / Write Commands

- `PAGE (0x00)`
  - Write Byte, then Read Byte
  - Expected: readback matches last written value
- `OPERATION (0x01)`
  - Write Byte `0x80`, then Read Byte
  - Expected: readback `0x80`, `STATUS_BYTE.OFF` cleared
  - Write Byte `0x00`
  - Expected: readback `0x00`, `STATUS_BYTE.OFF` set
- `ON_OFF_CONFIG (0x02)`
  - Write Byte, then Read Byte
  - Expected: readback matches last written value
- `PHASE (0x04)`
  - Write Byte, then Read Byte
  - Expected: readback matches last written value
- `WRITE_PROTECT (0x10)`
  - Read default byte
  - Expected default: `0x00`
  - Write non-zero byte, then attempt one writable command such as `ON_OFF_CONFIG`
  - Expected: writable command is rejected via `STATUS_CML.INVALID_OR_UNSUPPORTED_DATA_RECEIVED`
  - Write `WRITE_PROTECT = 0x00`
  - Expected: writable commands operate normally again
- `FAN_CONFIG_1_2 (0x3A)`
  - Write Byte, then Read Byte
  - Expected: readback matches last written value
- `FAN_CONFIG_3_4 (0x3D)`
  - Write Byte, then Read Byte
  - Expected: readback matches last written value
  - Note: this is currently a portable config shadow; real fan 3/4 hardware binding is product-specific
- `VOUT_MODE (0x20)`
  - Read Byte
  - Expected default `0x17`
- `POWER_MODE (0x34)`
  - Write Byte, then Read Byte
  - Expected: readback matches last written value
  - Note: this is currently a portable config shadow; real CRPS power-mode behavior remains platform policy
- Standard fault response byte shadows:
  - `VOUT_OV_FAULT_RESPONSE (0x41)`
  - `VOUT_UV_FAULT_RESPONSE (0x45)`
  - `IOUT_OC_FAULT_RESPONSE (0x47)`
  - `IOUT_OC_LV_FAULT_RESPONSE (0x49)`
  - `IOUT_UC_FAULT_RESPONSE (0x4C)`
  - `OT_FAULT_RESPONSE (0x50)`
  - `UT_FAULT_RESPONSE (0x54)`
  - `VIN_OV_FAULT_RESPONSE (0x56)`
  - `VIN_UV_FAULT_RESPONSE (0x5A)`
  - `IIN_OC_FAULT_RESPONSE (0x5C)`
  - `TON_MAX_FAULT_RESPONSE (0x63)`
  - `POUT_OP_FAULT_RESPONSE (0x69)`
  - For each command, Write Byte one non-default value and Read Byte it back
  - Expected: readback matches last written value
  - Note: these are currently response-policy shadows; platform-level shutdown/retry behavior is product policy and must be validated separately
- `PMBUS_REVISION (0x98)`
  - Read Byte
  - Expected `0x33`

## 3. Word Read / Write Commands

- `ZONE_CONFIG (0x07)`
  - Write Word, then Read Word
  - Expected: readback matches last written value
- `ZONE_ACTIVE (0x08)`
  - Write Word, then Read Word
  - Expected: readback matches last written value
- `VOUT_COMMAND (0x21)`
  - Write Word, then Read Word
  - Expected: readback matches last written value
  - Verify `Process Call` also echoes updated word
- VOUT programming shadows:
  - `VOUT_TRIM (0x22)`
  - `VOUT_CAL_OFFSET (0x23)`
  - `VOUT_MAX (0x24)`
  - `VOUT_MARGIN_HIGH (0x25)`
  - `VOUT_MARGIN_LOW (0x26)`
  - `VOUT_TRANSITION_RATE (0x27)`
  - `VOUT_DROOP (0x28)`
  - `VOUT_SCALE_LOOP (0x29)`
  - `VOUT_SCALE_MONITOR (0x2A)`
  - `VOUT_MIN (0x2B)`
  - For each command, Write Word one non-default value and Read Word it back
  - Expected: readback matches last written value
  - Note: these are currently config shadows; real voltage-loop behavior remains platform policy
- Power-stage and calibration shadows:
  - `POUT_MAX (0x31)`
  - `MAX_DUTY (0x32)`
  - `FREQUENCY_SWITCH (0x33)`
  - `VIN_ON (0x35)`
  - `VIN_OFF (0x36)`
  - `INTERLEAVE (0x37)`
  - `IOUT_CAL_GAIN (0x38)`
  - `IOUT_CAL_OFFSET (0x39)`
  - For each command, Write Word one non-default value and Read Word it back
  - Expected: readback matches last written value
  - Note: these are portable shadows; real CRPS control-loop, startup, interleave, and calibration binding remains a TODO
- `VOUT_OV_FAULT_LIMIT (0x40)`
  - Write lower threshold than current VOUT
  - Expected: `STATUS_VOUT.VOUT_OV_FAULT` sets
- `VOUT_OV_WARN_LIMIT (0x42)`
  - Write lower threshold than current VOUT, but keep fault limit above VOUT
  - Expected: `STATUS_VOUT.VOUT_OV_WARNING` sets without `STATUS_BYTE.VOUT_OV_FAULT`
- `VOUT_UV_WARN_LIMIT (0x43)`
  - Write threshold above current VOUT, but fault limit below VOUT
  - Expected: `STATUS_VOUT.VOUT_UV_WARNING` sets
- `VOUT_UV_FAULT_LIMIT (0x44)`
  - Write threshold above current VOUT
  - Expected: `STATUS_VOUT.VOUT_UV_FAULT` sets
- `IOUT_OC_FAULT_LIMIT (0x46)`
  - Write lower threshold than current IOUT
  - Expected: `STATUS_IOUT.IOUT_OC_FAULT` sets, `STATUS_BYTE.IOUT_OC_FAULT` sets
- `IOUT_OC_WARN_LIMIT (0x4A)`
  - Write lower threshold than current IOUT, but keep fault limit above IOUT
  - Expected: `STATUS_IOUT.IOUT_OC_WARNING` sets without `STATUS_BYTE.IOUT_OC_FAULT`
- `IOUT_OC_LV_FAULT_LIMIT (0x48)`
  - Write Word, then Read Word
  - Expected: readback matches last written value
  - Note: this is currently a threshold shadow; real low-voltage OC protection binding remains platform policy
- `IOUT_UC_FAULT_LIMIT (0x4B)`
  - Write Word, then Read Word
  - Expected: readback matches last written value
  - Note: this is currently a threshold shadow; real under-current protection binding remains platform policy
- `OT_FAULT_LIMIT (0x4F)`
  - Write lower threshold than hottest temperature
  - Expected: `STATUS_TEMPERATURE.OT_FAULT` sets
- `OT_WARN_LIMIT (0x51)`
  - Write lower threshold than hottest temperature, but fault limit above hottest temperature
  - Expected: `STATUS_TEMPERATURE.OT_WARNING` sets
- `UT_WARN_LIMIT (0x52)`
  - Write Word, then Read Word
  - Expected: readback matches last written value
- `UT_FAULT_LIMIT (0x53)`
  - Write Word, then Read Word
  - Expected: readback matches last written value
  - Note: under-temperature thresholds are currently portable shadows; real thermal policy binding remains platform policy
- `VIN_OV_FAULT_LIMIT (0x55)`
  - Write lower threshold than current VIN
  - Expected: `STATUS_INPUT.VIN_OV_FAULT` sets
- `VIN_OV_WARN_LIMIT (0x57)`
  - Write lower threshold than current VIN, but fault limit above VIN
  - Expected: `STATUS_INPUT.VIN_OV_WARNING` sets
- `VIN_UV_WARN_LIMIT (0x58)`
  - Write threshold above current VIN, but fault limit below VIN
  - Expected: `STATUS_INPUT.VIN_UV_WARNING` sets
- `VIN_UV_FAULT_LIMIT (0x59)`
  - Write threshold above current VIN
  - Expected: `STATUS_INPUT.VIN_UV_FAULT` sets, `STATUS_BYTE.VIN_UV_FAULT` sets
- `IIN_OC_FAULT_LIMIT (0x5B)`
  - Write lower threshold than current IIN
  - Expected: `STATUS_INPUT.IIN_OC_FAULT` sets
- `IIN_OC_WARN_LIMIT (0x5D)`
  - Write lower threshold than current IIN, but fault limit above IIN
  - Expected: `STATUS_INPUT.IIN_OC_WARNING` sets
- Power-good, timing, and power-limit shadows:
  - `POWER_GOOD_ON (0x5E)`
  - `POWER_GOOD_OFF (0x5F)`
  - `TON_DELAY (0x60)`
  - `TON_RISE (0x61)`
  - `TON_MAX_FAULT_LIMIT (0x62)`
  - `TOFF_DELAY (0x64)`
  - `TOFF_FALL (0x65)`
  - `TOFF_MAX_WARN_LIMIT (0x66)`
  - `POUT_OP_FAULT_LIMIT (0x68)`
  - `POUT_OP_WARN_LIMIT (0x6A)`
  - `PIN_OP_WARN_LIMIT (0x6B)`
  - For each command, Write Word one non-default value and Read Word it back
  - Expected: readback matches last written value
  - Note: these are portable shadows; real sequencer, power-good, and power-limit behavior remains platform policy
- `FAN_COMMAND_1 (0x3B)`
  - Write target above current fan 1 RPM
  - Expected: `STATUS_FANS_1_2.FAN_1_WARNING` or `FAN_1_FAULT` by target-relative policy
- `FAN_COMMAND_2 (0x3C)`
  - Write target above current fan 2 RPM
  - Expected: `STATUS_FANS_1_2.FAN_2_WARNING` or `FAN_2_FAULT` by target-relative policy
- `FAN_COMMAND_3 (0x3E)`
  - Write Word, then Read Word
  - Expected: readback matches last written value
  - Note: this is currently a portable config shadow; real fan 3 control is product-specific
- `FAN_COMMAND_4 (0x3F)`
  - Write Word, then Read Word
  - Expected: readback matches last written value
  - Note: this is currently a portable config shadow; real fan 4 control is product-specific

## 4. Standard Status Readback

- Read `STATUS_BYTE (0x78)`
  - Verify summary bits follow sub-status policy
- Read `STATUS_WORD (0x79)`
  - Verify high-byte summaries follow active sub-status registers
  - Verify low byte contains `STATUS_BYTE`
- Read:
  - `STATUS_VOUT (0x7A)`
  - `STATUS_IOUT (0x7B)`
  - `STATUS_INPUT (0x7C)`
  - `STATUS_TEMPERATURE (0x7D)`
  - `STATUS_CML (0x7E)`
  - `STATUS_OTHER (0x7F)`
  - `STATUS_MFR_SPECIFIC (0x80)`
  - `STATUS_FANS_1_2 (0x81)`

## 5. Telemetry Readback

- Read Word:
  - `READ_VIN (0x88)`
  - `READ_IIN (0x89)`
  - `READ_VOUT (0x8B)`
  - `READ_IOUT (0x8C)`
  - `READ_TEMPERATURE_1/2/3 (0x8D/0x8E/0x8F)`
  - `READ_FAN_SPEED_1/2 (0x90/0x91)`
  - `READ_POUT (0x96)`
  - `READ_PIN (0x97)`
- Expected:
  - `READ_VOUT` uses ULINEAR16 with `VOUT_MODE`
  - current/power/temperature/fan use LINEAR11

## 6. Block Read Commands

- `READ_EIN (0x86)`
  - Verify first byte is block count `0x06`
  - Verify accumulator/sample count payload updates over time
- `READ_EOUT (0x87)`
  - Same checks as `READ_EIN`
- `MFR_ID (0x99)`
  - Verify ASCII block string
- `MFR_MODEL (0x9A)`
  - Verify ASCII block string
- `MFR_REVISION (0x9B)`
  - Verify ASCII block string
- `MFR_SERIAL (0x9E)`
  - Verify ASCII block string
- `MFR_BLACKBOX (0xDC)`
  - Verify 32-byte payload
  - Verify status/telemetry snapshot bytes update as expected

## 7. Block Write-Read Process Call Commands

- `QUERY (0x1A)`
  - Write block count `0x01` with one target command byte, then read back one query byte block
  - Recommended targets:
    - `0x98 PMBUS_REVISION`
    - `0x99 MFR_ID`
    - `0x10 WRITE_PROTECT`
    - one response byte command such as `0x41 VOUT_OV_FAULT_RESPONSE`
  - Expected:
    - response block count `0x01`
    - bit7 set for supported command
    - read/write bits reflect the command's legal transaction types
    - format bits match the expected data family
- `SMBALERT_MASK (0x1B)`
  - Write Word `0x1234`
  - Then execute Block Write-Read Process Call read path with block count `0x00`
  - Expected:
    - response block count `0x02`
    - payload `0x34 0x12`
  - Restore to default `0x0000`
- `COEFFICIENTS (0x30)`
  - Execute Block Write-Read Process Call with block count `0x01` and one target command byte
  - Recommended target: `0x88 READ_VIN`
  - Expected:
    - response block count `0x05`
    - five coefficient bytes are returned in DIRECT-format order
  - Note: current values are fixed placeholders; replace with real DIRECT coefficients only if product exposes DIRECT-format commands
- `PAGE_PLUS_WRITE (0x05)`
  - Execute Block Write with block count including page byte, target command byte, and target payload
  - Recommended target: `0x01 OPERATION`
  - Expected:
    - transaction ACKs
    - target command handler updates the same shadow as direct target write
  - Note: current page handling is portable shadow behavior. TODO: validate against real multi-page rail implementation.
- `PAGE_PLUS_READ (0x06)`
  - Execute Block Write-Read Process Call with block count including page byte and target command byte
  - Recommended targets:
    - `0x98 PMBUS_REVISION`
    - `0x99 MFR_ID`
    - `0x79 STATUS_WORD`
  - Expected:
    - response is returned as a PMBus block payload
    - first response byte is byte/word/dword payload length for non-block target commands
  - Note: current page handling is portable shadow behavior. TODO: validate against real multi-page rail implementation.
- `STORE/RESTORE (0x11..0x18)`
  - Verify `*_ALL` variants ACK as Send Byte
  - Verify `*_CODE` variants ACK as Write Byte
  - Expected:
    - command is accepted unless write-protect is active
    - no actual NVM persistence is performed
  - Note: current implementation records the request only. TODO: bind to product NVM with wear/error policy.

## 7A. P2 Read-Only Placeholder Commands

- `STATUS_FANS_3_4 (0x82)`
  - Read Byte
  - Expected fixed `0x00` until fan 3/4 hardware is added
- `READ_KWH_IN (0x83)` / `READ_KWH_OUT (0x84)`
  - Read 32
  - Expected fixed `0x00000000` until energy accumulator is bound
- `READ_KWH_CONFIG (0x85)`
  - Read Word
  - Expected fixed `0x0000` until energy counter configuration is defined
- `READ_VCAP (0x8A)`
  - Read Word
  - Expected placeholder currently mirrors VIN telemetry
- `READ_FAN_SPEED_3 (0x92)` / `READ_FAN_SPEED_4 (0x93)`
  - Read Word
  - Expected fixed `0x0000` until fan 3/4 hardware is added
- `READ_DUTY_CYCLE (0x94)`
  - Read Word
  - Expected fixed `0x0000` until PWM/control-loop duty reporting is bound
- `READ_FREQUENCY (0x95)`
  - Read Word
  - Expected placeholder currently mirrors `FREQUENCY_SWITCH`
- `MFR_LOCATION (0x9C)` / `MFR_DATE (0x9D)` / `APP_PROFILE_SUPPORT (0x9F)`
  - Block Read
  - Expected placeholder blocks; replace with manufacturing and validated profile data
- `MFR_* rating commands (0xA0..0xAE)`
  - Read Word, Read Byte, or Block Read according to the support matrix
  - Expected placeholder values; replace with SKU and measurement-accuracy data
- `MFR_MAX_TEMP_1..3 (0xC0..0xC2)`
  - Read Word
  - Expected placeholder values mirroring current temperatures until peak-hold policy is implemented

## 8. Vendor Commands

- `USER_DATA_00..15 (0xB0..0xBF)`
  - For at least one command in the range, perform Block Read before writing
  - Expected default: block count `0x01`, payload equals the command byte
  - Perform Block Write with 1..16 bytes, then Block Read
  - Expected: readback matches the written bytes, truncated to 16 bytes if a longer block was attempted
  - Verify `QUERY` reports supported read/write and non-numeric/block-style format
  - Note: current CRPS policy is volatile shadow only; persistence requires product/NVM binding
- Unassigned `MFR_SPECIFIC_C4..FD`
  - Keep existing product-specific commands `D0/D6/D7/D8/DC` on their dedicated behavior paths
  - For at least one unassigned command, perform the same Block Write / Block Read / QUERY checks as `USER_DATA`
  - Expected: volatile 16-byte shadow behavior, reset on power cycle
- `MFR_SPECIFIC_COMMAND_EXT (0xFE)` / `PMBUS_COMMAND_EXT (0xFF)`
  - Verify `QUERY` reports the selector as supported
  - Execute an extended Block Write using selector + extended command + block count + data
  - Execute the matching extended Block Write-Read Process Call
  - Expected: volatile shadow readback for the same selector/extended-command pair
  - Note: extended subcommands are policy placeholders until a product-specific owner is assigned
- `MFR_COLD_REDUNDANCY_CONFIG (0xD0)`
  - Write Byte, then Read Byte
  - Expected: readback matches written value
- `MFR_FWUPLOAD_MODE (0xD6)`
  - Write Byte, then Read Byte
  - Expected: mode bit follows write unless blocked by bad image/unsupported state
- `MFR_FWUPLOAD (0xD7)`
  - Exercise valid block write sequence
  - Verify `MFR_FWUPLOAD_STATUS (0xD8)` transitions

## 9. PEC Validation

- With `PMBUS_PEC_POLICY = PMBUS_PEC_POLICY_OPTIONAL`
  - Verify `Read Byte/Word/Block Read` appends PEC
  - Verify `Write Byte/Word/Block Write` accepts no PEC
  - Verify `Write Byte/Word/Block Write` accepts valid PEC when present
  - Verify `Write Byte/Word/Block Write` rejects bad PEC when the frame shape indicates a PEC byte and sets `STATUS_CML.PACKET_ERROR_CHECK_FAILED`
- With `PMBUS_PEC_POLICY = PMBUS_PEC_POLICY_REQUIRED`
  - Verify `Write Byte/Word/Block Write` rejects missing PEC and sets `STATUS_CML.PACKET_ERROR_CHECK_FAILED`
  - Verify `Read Byte/Word/Block Read` still appends slave PEC
- With `PMBUS_PEC_POLICY = PMBUS_PEC_POLICY_DISABLED`
  - Verify the same transactions succeed without PEC byte
  - Verify extra PEC bytes are treated as normal payload and rejected if the protocol length no longer matches

## 10. Error Handling

- Unsupported command
  - Expected: `STATUS_CML.INVALID_OR_UNSUPPORTED_COMMAND_RECEIVED`
- Supported command with wrong transaction type or malformed payload
  - Expected: `STATUS_CML.INVALID_OR_UNSUPPORTED_DATA_RECEIVED`
- Bus timeout / stuck-bus recovery
  - Verify recover event, recover fail event, and related `STATUS_WORD.OTHER` behavior

## 11. Transaction Examples

- `READ_VOUT 0x8B` without PEC
  - `S B4 A 8B A Sr B5 A <LSB> A <MSB> N P`
- `STATUS_WORD 0x79` with PEC
  - `S B4 A 79 A <PEC> A Sr B5 A <LSB> A <MSB> A <PEC> N P`
- `MFR_MODEL 0x9A` block read
  - `S B4 A 9A A Sr B5 A <COUNT> A <DATA...> N P`
- `OPERATION 0x01` write byte with PEC
  - `S B4 A 01 A <DATA> A <PEC> A P`
- `QUERY 0x1A` block write-read process call with PEC
  - `S B4 A 1A A 01 A <TARGET_CMD> A <PEC> A Sr B5 A 01 A <QUERY_BYTE> A <PEC> N P`
- `SMBALERT_MASK 0x1B` read helper with PEC
  - `S B4 A 1B A 00 A <PEC> A Sr B5 A 02 A <LSB> A <MSB> A <PEC> N P`
- `COEFFICIENTS 0x30` block write-read process call with PEC
  - `S B4 A 30 A 01 A <TARGET_CMD> A <PEC> A Sr B5 A 05 A <M_L> A <M_H> A <B_L> A <B_H> A <R> A <PEC> N P`
- `ARA 0x0C` when ALERT# asserted
  - no PEC host read: `S 19 A <ALERTING_WRITE_ADDR> N P`
  - PEC host read: `S 19 A <ALERTING_WRITE_ADDR> A <PEC> N P`
- `ARP 0x61 GET_UDID`
  - `S C2 A 03 A Sr C3 A 11 A <17-byte UDID> A <PEC> N P`
- `Zone Read 0x28`
  - `S 51 A 05 A <ZONE_CONFIG_L> A <ZONE_CONFIG_H> A <ZONE_ACTIVE_L> A <ZONE_ACTIVE_H> A <STATUS_BYTE> A <PEC> N P`

## 12. Host Communication-Format Coverage

When validating this slave with the Pico PMBus master tool, the intended host-side format coverage is:

- SMBus:
  - `Receive Byte`
  - `Read Byte`
  - `Read Word`
  - `Block Read`
  - `Send Byte`
  - `Write Byte`
  - `Write Word`
  - `Block Write`
  - `Process Call`
  - `Block Write-Read Process Call`
  - `PEC enable/disable`
- PMBus:
  - `Group Command`
  - `CONTROL` assert/deassert
  - `ALERT#` poll / optional ARA helper
  - ARA response compatibility:
    - no-PEC host read expects one byte: alerting slave write address
    - PEC host read expects two bytes: alerting slave write address + SMBus PEC over `ARA read address + response address`
    - first ARA byte must be valid immediately at `SLA+R ACK`; reading a stale previous TX byte is a failure
    - final ARA byte must drive the controller to a transmit-finished status; debug log should show alias state returning to normal
    - after ARA is served, the portable alias path must stay inhibited until ALERT# is actually deasserted so the host can read `STATUS_CML` and clear faults at the normal address
    - after master NACK/STOP, alias must release and normal slave address must respond again
  - ARP alias:
    - `0x61` should ACK when `PMBUS_ENABLE_ARP = 1`
    - `GET_UDID` returns count `0x11` plus 17 UDID bytes
    - `ASSIGN_ADDRESS` updates the normal PMBus slave address and reconfigures aliases
  - Zone alias:
    - `0x28` Zone Read should return count `0x05`, `ZONE_CONFIG`, `ZONE_ACTIVE`, and `STATUS_BYTE`
    - `0x37` Zone Write is disabled by default on M031 unless a free alias slot is assigned

Checklist interpretation:

- A passing Pico `Full` suite means the currently automatable transport checks passed.
- `Receive Byte`, generic `Block Write`, end-to-end `ALERT# -> ARA`, and bus-timing review may still remain manual depending on the target test setup.
- `Block Write-Read Process Call` coverage now includes explicit `QUERY (0x1A)` and `SMBALERT_MASK (0x1B)` validation for this slave.
- `QUERY (0x1A)` and `SMBALERT_MASK (0x1B)` are now explicit validation targets for this slave.
- `Group Command` should be interpreted as verified transport sequencing unless a dedicated application-level simultaneous-commit check is added for the command under test.
