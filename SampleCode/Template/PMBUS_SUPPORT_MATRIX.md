# PMBus Support Matrix

This matrix documents the current PMBus device support in the portable framework.

Compliance target:
- Treat this M032 PMBus slave as a PMBus command/transaction reference target for:
  - `PMBus-Specification-Rev-1-3-1-Part-II-20150313.pdf`
  - `PMBus_rev_1.2_part_1_september_2010.pdf`
- Profile-specific source documents:
  - Base profile: `PMBus-Specification-Rev-1-3-1-Part-II-20150313.pdf` plus `PMBus_rev_1.2_part_1_september_2010.pdf`
  - M-CRPS profile: `M-CRPS_Base_Specification_version_1p06p00_RC1-draft7_042026.pdf`
  - TI UCD90xxx profile: `UCD90xxx Sequencer and System Health Controller PMBus Command Reference.pdf` (first-stage command-name/profile support; full chip validation remains product-specific)
- Current scope is complete host-visible PMBus command/transaction behavior, including protocol, data format, status/error behavior, and PEC behavior.
- Host-visible placeholders and config shadows are intentional extension hooks. They are not final ADC/GPIO/NVM/control product behavior until users bind them to real product sources or policy.
- Keep this matrix synchronized with the Pico tool-side PMBus matrix and validation checklist whenever a command is added or its behavior changes.
- Command-level synchronization source:
  - Pico HID Test Tool repository: `docs/PMBUS_COMMAND_CONTRACT.csv`
  - Pico HID Test Tool repository: `docs/PMBUS_COMMAND_CONTRACT.md`
- Update the command contract first when adding/removing/changing PMBus command support, then update this matrix to match.

Assumptions:
- Addressing is documented in 7-bit form.
- `PEC` means SMBus PEC behavior follows `PMBUS_PEC_POLICY`:
  - `PMBUS_PEC_POLICY_DISABLED`: no PEC is generated or accepted as an extra transaction byte.
  - `PMBUS_PEC_POLICY_OPTIONAL`: host write PEC is accepted/validated when present; slave read PEC is generated.
  - `PMBUS_PEC_POLICY_REQUIRED`: write-side transactions require valid host PEC; read-side transactions still generate slave PEC.
- `Source Path` describes the internal data owner:
  - `Config shadow`: writable PMBus configuration shadow in `pmbus_app`
  - `Telemetry shadow`: encoded telemetry shadow refreshed in background
  - `Status shadow`: PMBus status registers refreshed in background
  - `Platform shadow`: platform-facing adapter state in `pmbus_platform`
  - `Vendor shadow`: vendor-specific shadow state in `pmbus_app`

## System policy defaults

`pmbus_cfg_user.h` separates production CRPS behavior from lab validation helpers:

- `PMBUS_SYSTEM_POLICY_CRPS_DEFAULT`
  - ARA alias defaults to disabled.
  - ARP/UDID defaults to disabled.
  - Use this for CRPS product builds unless the final product contract explicitly requires these SMBus helpers.
- `PMBUS_SYSTEM_POLICY_LAB_VALIDATION`
  - ARA alias may be enabled to validate `SMBALERT# -> ARA -> normal-address recovery`.
  - ARP/UDID framework may be enabled to validate address-resolution flows.
  - This is the current sample default to keep the Pico validation flow available.

Command profile policy:

- `PMBUS_COMMAND_PROFILE` selects profile-specific command display/debug names independently from `PMBUS_PROFILE_MINIMAL/FULL` and `PMBUS_ENABLE_CMD_CRPS`.
- Base profile keeps public PMBus names plus generic policy namespace names (`USER_DATA_00..15`, `MFR_SPECIFIC_C0..FD`), for example `0xE3 -> MFR_SPECIFIC_E3`.
- M-CRPS profile maps public M-CRPS Table 12-38 names, for example `0xE3 -> MFR_FWUPLOAD_BLOCK_SIZE`.
- TI UCD90xxx profile maps TI command display/debug names, for example `0xE3 -> PARM_VALUE`; full UCD90xxx chip behavior and validation remain a separate product-specific layer.

Bit/field semantic policy:

- `PMBUS_DEBUG_PRINT_SEMANTICS` enables background semantic logs after successful command dispatch.
- The semantic layer is not part of the I2C ISR path and must not change bus timing or command data.
- Standard bit-defined commands currently have explicit semantic hooks for `OPERATION`, `ON_OFF_CONFIG`, `VOUT_MODE`, `FAN_CONFIG_*`, standard `*_FAULT_RESPONSE` bytes, `STATUS_*`, and `SMBALERT_MASK`.
- Profile-specific commands use the selected profile command name in semantic logs. Dedicated bit handlers must be added when a profile document defines bit-level meaning for that command.
- A row may be treated as product-complete only after its semantic hook is bound to real product behavior or the product policy explicitly accepts the fixed placeholder value.
- Host-visible shadow/placeholder rows may still pass transport validation, but they must not be presented as final ADC/GPIO/NVM/control behavior.

Clock-low/stuck-bus handling is implemented through a portable software SCL-low monitor. `TMR1_IRQHandler()` samples SCL once per millisecond and requests slave recovery when SCL remains low for `PMBUS_I2C_CLOCK_LOW_TIMEOUT_MS` (default `35U`). Final timeout-duration compliance still requires external fault injection and logic-analyzer timing validation.

The ordinary I2C timeout flag is cleared at ISR entry but does not bypass active I2C status handling. `SLA_R_ACK`, `DATA_TX_ACK`, `DATA_RX_ACK`, and `STOP_RESTART` must still run before SI is cleared; otherwise a pending timeout flag can release SCL with stale DAT/RX state.

Normal slave-transmit NACK/STOP cleanup is not treated as a PMBus communication fault. A single post-TX `0x00` controller status after `DATA_TX_NACK` / `LAST_TX_ACK` is ignored so scan reads do not assert CML/ALERT#/ARA; sustained low SCL/SDA conditions still use the software timeout and bus-clear recovery path.

Command-only repeated-start reads use the normal SMBus combined-format flow. `STOP_RESTART` only saves the pending command context; `SLA_R_ACK` restores that context, dispatches/prepares the PMBus response, writes the first response byte to I2C DAT, and then advances the TX index. A read address byte such as `0xB5` for target `0x5A` is never valid PMBus response data and indicates a slave TX first-byte timing failure.

Hardware note for the current M032 target:

- The PMBus layer is implemented over the normal I2C slave controller plus software SMBus/PMBus logic.
- I2C instance and pin binding are selected by `PMBUS_PORT_PROFILE` in `board_config.h`; the PMBus command core, protocol parser, and dispatcher are expected to remain unchanged when moving from I2C0 to I2C1 or another board-specific port.
- Current port profiles:
  - `PMBUS_PORT_PROFILE_M031_I2C0_PB4_PB5`: default validated mapping, PB4 SDA / PB5 SCL.
  - `PMBUS_PORT_PROFILE_M031_I2C1_PA2_PA3`: alternate M031 mapping, PA2 SDA / PA3 SCL.
- Do not treat hardware SMBus Bus Management / PEC registers as available on this target:
  - `I2C_BUSCTL`
  - `I2C_BUSTCTL`
  - `I2C_BUSSTS`
  - `I2C_PKTSIZE`
  - `I2C_PKTCRC`
  - `I2C_BUSTOUT`
  - `I2C_CLKTOUT`
- PEC generation/checking, block count handling, ARA/ARP policy, and command semantics are software-owned.
- Any ordinary I2C timeout counter used by the port layer is only a recovery aid, not a claim of hardware SMBus controller compliance.
- Software SCL-low timeout is the portable SMBus clock-low timeout path for this M032 PMBus slave.

| Command | Code | Protocol | PEC | Source Path | Expected Response / Behavior |
|---|---:|---|---|---|---|
| `PAGE` | `0x00` | Read Byte, Write Byte | R/W | Config shadow | Current page byte |
| `OPERATION` | `0x01` | Read Byte, Write Byte | R/W | Config shadow | Operation byte; bit7 drives `STATUS_BYTE.OFF` summary |
| `ON_OFF_CONFIG` | `0x02` | Read Byte, Write Byte | R/W | Config shadow | On/off policy byte |
| `CLEAR_FAULTS` | `0x03` | Send Byte | W | Status shadow | Clears latched fault bits and CML source |
| `PHASE` | `0x04` | Read Byte, Write Byte | R/W | Config shadow | Current phase selector shadow |
| `PAGE_PLUS_WRITE` | `0x05` | Block Write | W | Config shadow | Page-qualified write wrapper; reuses the target command handler and stores last page/target/payload. TODO: bind to real multi-page rails before product use |
| `PAGE_PLUS_READ` | `0x06` | Block Write-Read Process Call | R | Config shadow | Page-qualified read wrapper; returns target byte/word/dword/block response as a block payload. TODO: bind to real multi-page rails before product use |
| `ZONE_CONFIG` | `0x07` | Read Word, Write Word | R/W | Config shadow | 16-bit zone configuration shadow |
| `ZONE_ACTIVE` | `0x08` | Read Word, Write Word | R/W | Config shadow | 16-bit active-zone state shadow |
| `WRITE_PROTECT` | `0x10` | Read Byte, Write Byte | R/W | Config shadow | Write-protect byte; non-zero locks writable command handlers except `WRITE_PROTECT` and `CLEAR_FAULTS` |
| `STORE_DEFAULT_ALL` | `0x11` | Send Byte | W | Config shadow | ACKs and records the store/restore request only. TODO: connect to product NVM with wear/error policy |
| `RESTORE_DEFAULT_ALL` | `0x12` | Send Byte | W | Config shadow | ACKs and records the store/restore request only. TODO: connect to product NVM with wear/error policy |
| `STORE_DEFAULT_CODE` | `0x13` | Write Byte | W | Config shadow | ACKs and records the requested code only. TODO: connect to product NVM with wear/error policy |
| `RESTORE_DEFAULT_CODE` | `0x14` | Write Byte | W | Config shadow | ACKs and records the requested code only. TODO: connect to product NVM with wear/error policy |
| `STORE_USER_ALL` | `0x15` | Send Byte | W | Config shadow | ACKs and records the store/restore request only. TODO: connect to product NVM with wear/error policy |
| `RESTORE_USER_ALL` | `0x16` | Send Byte | W | Config shadow | ACKs and records the store/restore request only. TODO: connect to product NVM with wear/error policy |
| `STORE_USER_CODE` | `0x17` | Write Byte | W | Config shadow | ACKs and records the requested code only. TODO: connect to product NVM with wear/error policy |
| `RESTORE_USER_CODE` | `0x18` | Write Byte | W | Config shadow | ACKs and records the requested code only. TODO: connect to product NVM with wear/error policy |
| `FAN_CONFIG_1_2` | `0x3A` | Read Byte, Write Byte | R/W | Config shadow | Fan control/config byte |
| `FAN_COMMAND_1` | `0x3B` | Read Word, Write Word | R/W | Config shadow + Telemetry policy | LINEAR11 fan 1 target; target-relative warning/fault heuristic |
| `FAN_COMMAND_2` | `0x3C` | Read Word, Write Word | R/W | Config shadow + Telemetry policy | LINEAR11 fan 2 target; target-relative warning/fault heuristic |
| `FAN_CONFIG_3_4` | `0x3D` | Read Byte, Write Byte | R/W | Config shadow | Fan 3/4 config byte shadow; currently not bound to real fan hardware |
| `FAN_COMMAND_3` | `0x3E` | Read Word, Write Word | R/W | Config shadow | Fan 3 command shadow; TODO bind to real fan/PWM path if product uses fan 3 |
| `FAN_COMMAND_4` | `0x3F` | Read Word, Write Word | R/W | Config shadow | Fan 4 command shadow; TODO bind to real fan/PWM path if product uses fan 4 |
| `CAPABILITY` | `0x19` | Read Byte | R | Config shadow | `0x80` when PEC enabled, `0x00` otherwise |
| `QUERY` | `0x1A` | Block Write-Read Process Call | R | Dispatch map | Returns a 1-byte PMBus query capability payload for the requested command byte |
| `SMBALERT_MASK` | `0x1B` | Write Word, Block Write-Read Process Call | R/W | Config shadow | 16-bit SMBALERT mask shadow; read path returns block payload `[count=2][LSB][MSB]` |
| `VOUT_MODE` | `0x20` | Read Byte, Write Byte | R/W | Config shadow | PMBus Table 2 VOUT_MODE byte: validates absolute/relative plus ULINEAR16/VID/Direct/IEEE selector semantics |
| `VOUT_COMMAND` | `0x21` | Read Word, Write Word, Process Call | R/W | Config shadow | VOUT_MODE-aware target voltage word; ULINEAR16/Direct/IEEE update the internal mV shadow; VID keeps right-justified raw code until a product VID table is assigned |
| `VOUT_TRIM` | `0x22` | Read Word, Write Word | R/W | Config shadow | VOUT_MODE-aware trim shadow; write path parses/validates active Table 2 format; ULINEAR16/Direct/IEEE are numeric, VID is product-table raw code |
| `VOUT_CAL_OFFSET` | `0x23` | Read Word, Write Word | R/W | Config shadow | VOUT_MODE-aware calibration offset shadow; write path parses/validates active Table 2 format; ULINEAR16/Direct/IEEE are numeric, VID is product-table raw code |
| `VOUT_MAX` | `0x24` | Read Word, Write Word | R/W | Config shadow | VOUT_MODE-aware maximum output-voltage shadow; write path parses/validates active Table 2 format; numeric formats can feed limit comparison |
| `VOUT_MARGIN_HIGH` | `0x25` | Read Word, Write Word | R/W | Config shadow | VOUT_MODE-aware high-margin voltage shadow; write path parses/validates active Table 2 format including relative-mode legality |
| `VOUT_MARGIN_LOW` | `0x26` | Read Word, Write Word | R/W | Config shadow | VOUT_MODE-aware low-margin voltage shadow; write path parses/validates active Table 2 format including relative-mode legality |
| `VOUT_TRANSITION_RATE` | `0x27` | Read Word, Write Word | R/W | Config shadow | LINEAR11 transition-rate shadow |
| `VOUT_DROOP` | `0x28` | Read Word, Write Word | R/W | Config shadow | LINEAR11 droop shadow |
| `VOUT_SCALE_LOOP` | `0x29` | Read Word, Write Word | R/W | Config shadow | LINEAR11 loop-scale shadow |
| `VOUT_SCALE_MONITOR` | `0x2A` | Read Word, Write Word | R/W | Config shadow | LINEAR11 monitor-scale shadow |
| `VOUT_MIN` | `0x2B` | Read Word, Write Word | R/W | Config shadow | VOUT_MODE-aware minimum output-voltage shadow; write path parses/validates active Table 2 format; numeric formats can feed limit comparison |
| `COEFFICIENTS` | `0x30` | Block Write-Read Process Call | R | Config shadow | Returns usable default DIRECT coefficients m=1 b=0 R=3 (raw word represents millivolts); TODO replace if product Direct scaling differs |
| `POUT_MAX` | `0x31` | Read Word, Write Word | R/W | Config shadow | LINEAR11 output-power max shadow; TODO bind to CRPS rating/control policy |
| `MAX_DUTY` | `0x32` | Read Word, Write Word | R/W | Config shadow | LINEAR11 duty-cycle max shadow; TODO bind to PWM/control-loop limit |
| `FREQUENCY_SWITCH` | `0x33` | Read Word, Write Word | R/W | Config shadow | LINEAR11 switching-frequency shadow; TODO bind to power-stage control |
| `POWER_MODE` | `0x34` | Read Byte, Write Byte | R/W | Config shadow | Power-mode byte shadow; TODO bind to actual power-mode state machine |
| `VIN_ON` | `0x35` | Read Word, Write Word | R/W | Config shadow | LINEAR11 VIN-on threshold shadow; TODO bind to startup policy |
| `VIN_OFF` | `0x36` | Read Word, Write Word | R/W | Config shadow | LINEAR11 VIN-off threshold shadow; TODO bind to shutdown policy |
| `INTERLEAVE` | `0x37` | Read Word, Write Word | R/W | Config shadow | 16-bit interleave shadow; TODO bind to phase/interleave implementation if used |
| `IOUT_CAL_GAIN` | `0x38` | Read Word, Write Word | R/W | Config shadow | LINEAR11 current-sense gain calibration shadow; TODO bind to calibration data |
| `IOUT_CAL_OFFSET` | `0x39` | Read Word, Write Word | R/W | Config shadow | LINEAR11 current-sense offset calibration shadow; TODO bind to calibration data |
| `VOUT_OV_FAULT_LIMIT` | `0x40` | Read Word, Write Word | R/W | Config shadow + Telemetry policy | VOUT_MODE-aware threshold shadow; ULINEAR16/Direct/IEEE can auto-set `STATUS_VOUT.VOUT_OV_FAULT`; VID threshold policy is product-table TODO |
| `VOUT_OV_FAULT_RESPONSE` | `0x41` | Read Byte, Write Byte | R/W | Config shadow | Response-policy byte shadow; product shutdown/retry behavior is platform policy |
| `VOUT_OV_WARN_LIMIT` | `0x42` | Read Word, Write Word | R/W | Config shadow + Telemetry policy | VOUT_MODE-aware threshold shadow; ULINEAR16/Direct/IEEE can auto-set `STATUS_VOUT.VOUT_OV_WARNING`; VID threshold policy is product-table TODO |
| `VOUT_UV_WARN_LIMIT` | `0x43` | Read Word, Write Word | R/W | Config shadow + Telemetry policy | VOUT_MODE-aware threshold shadow; ULINEAR16/Direct/IEEE can auto-set `STATUS_VOUT.VOUT_UV_WARNING`; VID threshold policy is product-table TODO |
| `VOUT_UV_FAULT_LIMIT` | `0x44` | Read Word, Write Word | R/W | Config shadow + Telemetry policy | VOUT_MODE-aware threshold shadow; ULINEAR16/Direct/IEEE can auto-set `STATUS_VOUT.VOUT_UV_FAULT`; VID threshold policy is product-table TODO |
| `VOUT_UV_FAULT_RESPONSE` | `0x45` | Read Byte, Write Byte | R/W | Config shadow | Response-policy byte shadow; product shutdown/retry behavior is platform policy |
| `IOUT_OC_FAULT_LIMIT` | `0x46` | Read Word, Write Word | R/W | Config shadow + Telemetry policy | LINEAR11 threshold; auto-sets `STATUS_IOUT.IOUT_OC_FAULT` |
| `IOUT_OC_FAULT_RESPONSE` | `0x47` | Read Byte, Write Byte | R/W | Config shadow | Response-policy byte shadow; product shutdown/retry behavior is platform policy |
| `IOUT_OC_LV_FAULT_LIMIT` | `0x48` | Read Word, Write Word | R/W | Config shadow | LINEAR11 low-voltage over-current threshold shadow; TODO bind to protection logic |
| `IOUT_OC_LV_FAULT_RESPONSE` | `0x49` | Read Byte, Write Byte | R/W | Config shadow | Response-policy byte shadow; product shutdown/retry behavior is platform policy |
| `IOUT_OC_WARN_LIMIT` | `0x4A` | Read Word, Write Word | R/W | Config shadow + Telemetry policy | LINEAR11 threshold; auto-sets `STATUS_IOUT.IOUT_OC_WARNING` |
| `IOUT_UC_FAULT_LIMIT` | `0x4B` | Read Word, Write Word | R/W | Config shadow | LINEAR11 under-current threshold shadow; TODO bind to protection logic |
| `IOUT_UC_FAULT_RESPONSE` | `0x4C` | Read Byte, Write Byte | R/W | Config shadow | Response-policy byte shadow; product shutdown/retry behavior is platform policy |
| `OT_FAULT_LIMIT` | `0x4F` | Read Word, Write Word | R/W | Config shadow + Telemetry policy | LINEAR11 threshold; auto-sets `STATUS_TEMPERATURE.OT_FAULT` |
| `OT_FAULT_RESPONSE` | `0x50` | Read Byte, Write Byte | R/W | Config shadow | Response-policy byte shadow; product shutdown/retry behavior is platform policy |
| `OT_WARN_LIMIT` | `0x51` | Read Word, Write Word | R/W | Config shadow + Telemetry policy | LINEAR11 threshold; auto-sets `STATUS_TEMPERATURE.OT_WARNING` |
| `UT_WARN_LIMIT` | `0x52` | Read Word, Write Word | R/W | Config shadow | LINEAR11 under-temperature warning threshold shadow; TODO bind to thermal policy |
| `UT_FAULT_LIMIT` | `0x53` | Read Word, Write Word | R/W | Config shadow | LINEAR11 under-temperature fault threshold shadow; TODO bind to thermal policy |
| `UT_FAULT_RESPONSE` | `0x54` | Read Byte, Write Byte | R/W | Config shadow | Response-policy byte shadow; product shutdown/retry behavior is platform policy |
| `VIN_OV_FAULT_LIMIT` | `0x55` | Read Word, Write Word | R/W | Config shadow + Telemetry policy | LINEAR11 threshold; auto-sets `STATUS_INPUT.VIN_OV_FAULT` |
| `VIN_OV_FAULT_RESPONSE` | `0x56` | Read Byte, Write Byte | R/W | Config shadow | Response-policy byte shadow; product shutdown/retry behavior is platform policy |
| `VIN_OV_WARN_LIMIT` | `0x57` | Read Word, Write Word | R/W | Config shadow + Telemetry policy | LINEAR11 threshold; auto-sets `STATUS_INPUT.VIN_OV_WARNING` |
| `VIN_UV_WARN_LIMIT` | `0x58` | Read Word, Write Word | R/W | Config shadow + Telemetry policy | LINEAR11 threshold; auto-sets `STATUS_INPUT.VIN_UV_WARNING` |
| `VIN_UV_FAULT_LIMIT` | `0x59` | Read Word, Write Word | R/W | Config shadow + Telemetry policy | LINEAR11 threshold; auto-sets `STATUS_INPUT.VIN_UV_FAULT` |
| `VIN_UV_FAULT_RESPONSE` | `0x5A` | Read Byte, Write Byte | R/W | Config shadow | Response-policy byte shadow; product shutdown/retry behavior is platform policy |
| `IIN_OC_FAULT_LIMIT` | `0x5B` | Read Word, Write Word | R/W | Config shadow + Telemetry policy | LINEAR11 threshold; auto-sets `STATUS_INPUT.IIN_OC_FAULT` |
| `IIN_OC_FAULT_RESPONSE` | `0x5C` | Read Byte, Write Byte | R/W | Config shadow | Response-policy byte shadow; product shutdown/retry behavior is platform policy |
| `IIN_OC_WARN_LIMIT` | `0x5D` | Read Word, Write Word | R/W | Config shadow + Telemetry policy | LINEAR11 threshold; auto-sets `STATUS_INPUT.IIN_OC_WARNING` |
| `POWER_GOOD_ON` | `0x5E` | Read Word, Write Word | R/W | Config shadow | VOUT_MODE-aware power-good assert threshold shadow; write path validates active Table 2 format; TODO bind threshold to real PG logic |
| `POWER_GOOD_OFF` | `0x5F` | Read Word, Write Word | R/W | Config shadow | VOUT_MODE-aware power-good deassert threshold shadow; write path validates active Table 2 format; TODO bind threshold to real PG logic |
| `TON_DELAY` | `0x60` | Read Word, Write Word | R/W | Config shadow | LINEAR11 turn-on delay shadow; TODO bind to sequencer timing |
| `TON_RISE` | `0x61` | Read Word, Write Word | R/W | Config shadow | LINEAR11 turn-on rise-time shadow; TODO bind to sequencer timing |
| `TON_MAX_FAULT_LIMIT` | `0x62` | Read Word, Write Word | R/W | Config shadow | LINEAR11 turn-on max fault limit shadow; TODO bind to startup fault detection |
| `TON_MAX_FAULT_RESPONSE` | `0x63` | Read Byte, Write Byte | R/W | Config shadow | Response-policy byte shadow; product startup retry behavior is platform policy |
| `TOFF_DELAY` | `0x64` | Read Word, Write Word | R/W | Config shadow | LINEAR11 turn-off delay shadow; TODO bind to sequencer timing |
| `TOFF_FALL` | `0x65` | Read Word, Write Word | R/W | Config shadow | LINEAR11 turn-off fall-time shadow; TODO bind to sequencer timing |
| `TOFF_MAX_WARN_LIMIT` | `0x66` | Read Word, Write Word | R/W | Config shadow | LINEAR11 turn-off warning limit shadow; TODO bind to shutdown monitoring |
| `POUT_OP_FAULT_LIMIT` | `0x68` | Read Word, Write Word | R/W | Config shadow | LINEAR11 output overpower fault threshold shadow; TODO bind to power-limit policy |
| `POUT_OP_FAULT_RESPONSE` | `0x69` | Read Byte, Write Byte | R/W | Config shadow | Response-policy byte shadow; product power-limit retry behavior is platform policy |
| `POUT_OP_WARN_LIMIT` | `0x6A` | Read Word, Write Word | R/W | Config shadow | LINEAR11 output overpower warning threshold shadow; TODO bind to power-limit policy |
| `PIN_OP_WARN_LIMIT` | `0x6B` | Read Word, Write Word | R/W | Config shadow | LINEAR11 input overpower warning threshold shadow; TODO bind to power-limit policy |
| `STATUS_BYTE` | `0x78` | Read Byte | R | Status shadow | PMBus summary byte |
| `STATUS_WORD` | `0x79` | Read Word | R | Status shadow | PMBus summary word; low byte includes `STATUS_BYTE` |
| `STATUS_VOUT` | `0x7A` | Read Byte | R | Status shadow | VOUT warning/fault bits |
| `STATUS_IOUT` | `0x7B` | Read Byte | R | Status shadow | IOUT/POUT warning/fault bits |
| `STATUS_INPUT` | `0x7C` | Read Byte | R | Status shadow | VIN/IIN/PIN warning/fault bits |
| `STATUS_TEMPERATURE` | `0x7D` | Read Byte | R | Status shadow | OT/UT warning/fault bits |
| `STATUS_CML` | `0x7E` | Read Byte | R | Status shadow | Communication/memory/logic fault bits |
| `STATUS_OTHER` | `0x7F` | Read Byte | R | Status shadow | ORing/fuse/alert-origin fault bits |
| `STATUS_MFR_SPECIFIC` | `0x80` | Read Byte | R | Status shadow | Manufacturer-specific fault summary byte |
| `STATUS_FANS_1_2` | `0x81` | Read Byte | R | Status shadow | Fan warning/fault bits |
| `STATUS_FANS_3_4` | `0x82` | Read Byte | R | Status shadow | Fixed zero placeholder for fan 3/4 not-present. TODO: bind if product has fan 3/4 |
| `READ_KWH_IN` | `0x83` | Read 32 | R | Telemetry shadow | Fixed zero 32-bit placeholder. TODO: bind to input energy accumulator |
| `READ_KWH_OUT` | `0x84` | Read 32 | R | Telemetry shadow | Fixed zero 32-bit placeholder. TODO: bind to output energy accumulator |
| `READ_KWH_CONFIG` | `0x85` | Read Word | R | Telemetry shadow | Fixed zero configuration placeholder. TODO: define energy counter rollover/persistence policy |
| `READ_EIN` | `0x86` | Block Read | R | Telemetry shadow | 6-byte energy/sample-count payload |
| `READ_EOUT` | `0x87` | Block Read | R | Telemetry shadow | 6-byte energy/sample-count payload |
| `READ_VIN` | `0x88` | Read Word | R | Telemetry shadow | LINEAR11 encoded VIN |
| `READ_IIN` | `0x89` | Read Word | R | Telemetry shadow | LINEAR11 encoded IIN |
| `READ_VCAP` | `0x8A` | Read Word | R | Telemetry shadow | Mirrors VIN placeholder. TODO: bind to hold-up/bulk capacitor measurement if available |
| `READ_VOUT` | `0x8B` | Read Word | R | Telemetry shadow | VOUT_MODE-aware READ_VOUT; ULINEAR16/Direct/IEEE encode internal mV source, VID mirrors raw VOUT_COMMAND until a product VID table is assigned |
| `READ_IOUT` | `0x8C` | Read Word | R | Telemetry shadow | LINEAR11 encoded IOUT |
| `READ_TEMPERATURE_1` | `0x8D` | Read Word | R | Telemetry shadow | LINEAR11 encoded temperature 1 |
| `READ_TEMPERATURE_2` | `0x8E` | Read Word | R | Telemetry shadow | LINEAR11 encoded temperature 2 |
| `READ_TEMPERATURE_3` | `0x8F` | Read Word | R | Telemetry shadow | LINEAR11 encoded temperature 3 |
| `READ_FAN_SPEED_1` | `0x90` | Read Word | R | Telemetry shadow | LINEAR11 encoded fan 1 RPM |
| `READ_FAN_SPEED_2` | `0x91` | Read Word | R | Telemetry shadow | LINEAR11 encoded fan 2 RPM |
| `READ_FAN_SPEED_3` | `0x92` | Read Word | R | Telemetry shadow | Fixed zero placeholder. TODO: bind if product has fan 3 |
| `READ_FAN_SPEED_4` | `0x93` | Read Word | R | Telemetry shadow | Fixed zero placeholder. TODO: bind if product has fan 4 |
| `READ_DUTY_CYCLE` | `0x94` | Read Word | R | Telemetry shadow | Fixed zero placeholder. TODO: bind to PWM/control-loop duty reporting |
| `READ_FREQUENCY` | `0x95` | Read Word | R | Telemetry shadow | Mirrors `FREQUENCY_SWITCH` shadow. TODO: bind to real switching-frequency telemetry |
| `READ_POUT` | `0x96` | Read Word | R | Telemetry shadow | LINEAR11 encoded output power |
| `READ_PIN` | `0x97` | Read Word | R | Telemetry shadow | LINEAR11 encoded input power |
| `PMBUS_REVISION` | `0x98` | Read Byte | R | Config shadow | Fixed `0x33` for PMBus 1.3 |
| `MFR_ID` | `0x99` | Block Read | R | Vendor shadow | ASCII block string |
| `MFR_MODEL` | `0x9A` | Block Read | R | Vendor shadow | ASCII block string |
| `MFR_REVISION` | `0x9B` | Block Read | R | Vendor shadow | ASCII block string |
| `MFR_LOCATION` | `0x9C` | Block Read | R | Vendor shadow | ASCII placeholder. TODO: bind to manufacturing/site data |
| `MFR_DATE` | `0x9D` | Block Read | R | Vendor shadow | ASCII placeholder. TODO: bind to manufacturing date data |
| `MFR_SERIAL` | `0x9E` | Block Read | R | Vendor shadow | ASCII block string |
| `APP_PROFILE_SUPPORT` | `0x9F` | Block Read | R | Vendor shadow | Fixed Server AC-DC profile placeholder. TODO: publish only product-validated profiles |
| `MFR_VIN_MIN` | `0xA0` | Read Word | R | Vendor shadow | Mirrors VIN-on threshold placeholder. TODO: bind to SKU rating |
| `MFR_VIN_MAX` | `0xA1` | Read Word | R | Vendor shadow | Mirrors VIN OV fault limit placeholder. TODO: bind to SKU rating |
| `MFR_IIN_MAX` | `0xA2` | Read Word | R | Vendor shadow | Mirrors IIN OC fault limit placeholder. TODO: bind to SKU rating |
| `MFR_PIN_MAX` | `0xA3` | Read Word | R | Vendor shadow | Mirrors PIN OP warn limit placeholder. TODO: bind to SKU rating |
| `MFR_VOUT_MIN` | `0xA4` | Read Word | R | Vendor shadow | Mirrors VOUT_MIN shadow. TODO: bind to SKU rating and selected product voltage format |
| `MFR_VOUT_MAX` | `0xA5` | Read Word | R | Vendor shadow | Mirrors VOUT_MAX shadow. TODO: bind to SKU rating and selected product voltage format |
| `MFR_IOUT_MAX` | `0xA6` | Read Word | R | Vendor shadow | Mirrors IOUT OC fault limit placeholder. TODO: bind to SKU rating |
| `MFR_POUT_MAX` | `0xA7` | Read Word | R | Vendor shadow | Mirrors POUT_MAX shadow. TODO: bind to SKU rating |
| `MFR_TAMBIENT_MAX` | `0xA8` | Read Word | R | Vendor shadow | Mirrors OT fault limit placeholder. TODO: bind to SKU rating |
| `MFR_TAMBIENT_MIN` | `0xA9` | Read Word | R | Vendor shadow | Mirrors UT fault limit placeholder. TODO: bind to SKU rating |
| `MFR_EFFICIENCY_LL` | `0xAA` | Block Read | R | Vendor shadow | Fixed raw efficiency-table placeholder. TODO: bind to qualified efficiency table |
| `MFR_EFFICIENCY_HL` | `0xAB` | Block Read | R | Vendor shadow | Fixed raw efficiency-table placeholder. TODO: bind to qualified efficiency table |
| `MFR_PIN_ACCURACY` | `0xAC` | Read Byte | R | Vendor shadow | Fixed 5.0% placeholder encoded as 0.1% units. TODO: bind to measurement accuracy spec |
| `IC_DEVICE_ID` | `0xAD` | Block Read | R | Vendor shadow | ASCII device-ID placeholder. TODO: bind to silicon/product identity |
| `IC_DEVICE_REV` | `0xAE` | Block Read | R | Vendor shadow | ASCII device-revision placeholder. TODO: bind to silicon/product revision |
| `USER_DATA_00..15` | `0xB0..0xBF` | Block Read, Block Write | R/W | CRPS policy shadow | Volatile 16-byte scratchpad per command, reset on power cycle. TODO: bind to NVM/product owner if persistent USER_DATA is required |
| `MFR_EFFICIENCY_DATA` | `0xB3` | Block Read | R | Vendor shadow | 8-byte M-CRPS efficiency data placeholder. TODO: bind to measured/qualified efficiency table |
| `MFR_MAX_TEMP_1` | `0xC0` | Read Word | R | Vendor shadow + Telemetry shadow | Mirrors current temperature 1 placeholder. TODO: implement peak-hold/latch policy |
| `MFR_MAX_TEMP_2` | `0xC1` | Read Word | R | Vendor shadow + Telemetry shadow | Mirrors current temperature 2 placeholder. TODO: implement peak-hold/latch policy |
| `MFR_MAX_TEMP_3` | `0xC2` | Read Word | R | Vendor shadow + Telemetry shadow | Mirrors current temperature 3 placeholder. TODO: implement peak-hold/latch policy |
| `MFR_SPECIFIC_C4..FD` | `0xC4..0xFD` | Block Read, Block Write | R/W | CRPS policy shadow | Unassigned manufacturer commands use volatile 16-byte scratchpads. Explicit M-CRPS profile behaviors are preserved and not overridden |
| `MFR_COLD_REDUNDANCY_CONFIG` | `0xD0` | Read Byte, Write Byte | R/W | Vendor shadow | Vendor config byte |
| `MFR_READ_CONFIG_FILE_SIZE` | `0xD1` | Block Read | R | Vendor shadow | 4-byte config file-size placeholder. TODO: bind to real image/config storage metadata |
| `MFR_READ_CONFIG_BLOCK_SIZE` | `0xD2` | Read Word | R | Vendor shadow | Config block-size placeholder. TODO: bind to real image/config transport limit |
| `MFR_READ_CONFIG_FILE` | `0xD3` | Block Write-Read Process Call | R/W | Vendor shadow | Config file chunk placeholder. TODO: use request payload as real offset/selector |
| `MFR_HW_COMPATIBILITY` | `0xD4` | Read Word | R | Vendor shadow | M-CRPS hardware compatibility placeholder. TODO: bind to platform hardware compatibility policy |
| `MFR_FWUPLOAD_CAPABILITY` | `0xD5` | Read Byte | R | Vendor shadow | M-CRPS firmware-upload capability placeholder. TODO: bind to product bootloader/update capability |
| `MFR_FWUPLOAD_MODE` | `0xD6` | Read Byte, Write Byte | R/W | Vendor shadow | Firmware upload mode byte |
| `MFR_FWUPLOAD` | `0xD7` | Block Write | W | Vendor shadow | FW upload block parser and state tracking |
| `MFR_FWUPLOAD_STATUS` | `0xD8` | Read Word | R | Vendor shadow | FW upload status word |
| `MFR_FW_REVISION` | `0xD9` | Block Read | R | Vendor shadow | M-CRPS 7-byte firmware revision placeholder. TODO: bind to build metadata |
| `MFR_SPDM` | `0xDA` | Block Write-Read Process Call | R/W | Vendor shadow | SPDM exchange placeholder. TODO: bind to real SPDM processor |
| `MFR_FRU_PROTECTION` | `0xDB` | Read Byte, Write Byte | R/W | Vendor shadow | M-CRPS FRU protection volatile byte shadow. TODO: bind to FRU/NVM write-protect policy |
| `MFR_BLACKBOX` | `0xDC` | Block Read | R | Vendor shadow + Status shadow | M-CRPS long blackbox profile row is currently implemented as a 32-byte live/latched diagnostic snapshot. TODO: add dedicated long-block storage if the full profile size is required |
| `MFR_REAL_TIME_BLACK_BOX` | `0xDD` | Block Read, Block Write-Read Process Call | R/W | Vendor shadow | M-CRPS 4-byte real-time blackbox placeholder. TODO: bind to real-time fault snapshot source |
| `MFR_SYSTEM_BLACK_BOX` | `0xDE` | Block Read, Block Write-Read Process Call | R/W | Vendor shadow | M-CRPS system blackbox placeholder bounded to 32 bytes for the M032 reference. TODO: bind to product blackbox storage and long-block policy |
| `MFR_BLACK_BOX_CONFIG` | `0xDF` | Read Byte, Write Byte | R/W | Vendor shadow | Blackbox config volatile byte shadow. TODO: bind to product blackbox policy |
| `MFR_CLEAR_BLACK_BOX` | `0xE0` | Write Byte | W | Vendor shadow | Clear-blackbox command ACK placeholder. TODO: clear real product blackbox storage |
| `MFR_LINE_STATUS` | `0xE1` | Read Byte, Write Byte | R/W | Vendor shadow | Line-status volatile byte shadow. TODO: bind to AC line status policy |
| `MFR_SYSTEM_LED_CNTL` | `0xE2` | Read Word, Write Word | R/W | Vendor shadow | System LED control word shadow. TODO: bind to real LED control |
| `MFR_FWUPLOAD_BLOCK_SIZE` | `0xE3` | Read Word | R | Vendor shadow | Firmware upload block-size placeholder. TODO: bind to bootloader transport limit |
| `MFR_EN_STATUS_SIMULATION_CMD` | `0xE4` | Read Byte, Write Byte | R/W | Vendor shadow | Status simulation enable shadow. TODO: gate lab-only status injection |
| `MFR_PEAK_CURRENT_RECORD` | `0xE9` | Block Read, Block Write | R/W | Vendor shadow | Peak current record block shadow. TODO: bind to peak-hold current recorder |
| `MFR_COMPONENT_ID` | `0xEB` | Block Read | R | Vendor shadow | 12-byte component-ID placeholder. TODO: bind to real component identity |
| `MFR_TOT_POUT_MAX` | `0xEC` | Read Word | R | Vendor shadow | Total output power placeholder. TODO: bind to line-status-specific rating |
| `MFR_VOUT_MARGINING` | `0xED` | Read Word, Write Word | R/W | Vendor shadow | VOUT margining word shadow. TODO: bind to output-voltage margin control |
| `MFR_OCWPL1_SETTING` | `0xEE` | Block Read, Block Write | R/W | Vendor shadow | OCW PL1 setting block shadow. TODO: bind to overload/current-limit policy |
| `MFR_PWOK_WARNING_TIME` | `0xF0` | Read Word, Write Word | R/W | Vendor shadow | M-CRPS Linear11 PWOK warning time volatile shadow. TODO: bind to timing policy |
| `MFR_MAX_IOUT_CAPABILITY` | `0xF1` | Block Read | R | Vendor shadow | M-CRPS 14-byte current capability placeholder. TODO: bind to PSU current capability table |
| `MFR_FPC_MAIN_MIN_OFF_TIME` | `0xF2` | Read Word, Write Word | R/W | Vendor shadow | Main FPC min-off-time shadow. TODO: bind to real FPC timing policy |
| `MFR_FPC_12VSB_MIN_OFF_TIME` | `0xF3` | Read Word, Write Word | R/W | Vendor shadow | 12VSB FPC min-off-time shadow. TODO: bind to real FPC timing policy |
| `MFR_SPECIFIC_COMMAND_EXT` | `0xFE` | Extended selector | R/W | CRPS policy shadow | Selector recognized; extended subcommands use volatile block shadow until product-specific ownership is allocated |
| `PMBUS_COMMAND_EXT` | `0xFF` | Extended selector | R/W | CRPS policy shadow | Selector recognized; extended subcommands use volatile block shadow until standard/product extended commands are allocated |

## Platform Source Entry Points

The intended platform-facing entry layer is `pmbus_platform.c/.h`.

Recommended usage:
- Telemetry updates:
  - `pmbus_platform_set_input_measurements()`
  - `pmbus_platform_set_output_measurements()`
  - `pmbus_platform_set_temperature_measurements()`
  - `pmbus_platform_set_fan_measurements()`
- Fault/warning events:
  - `pmbus_platform_set_vout_ov_fault()`
  - `pmbus_platform_set_iout_oc_fault()`
  - `pmbus_platform_set_input_vin_uv_fault()`
  - `pmbus_platform_set_temperature_ot_fault()`
  - `pmbus_platform_set_fan_1_fault()`
  - `pmbus_platform_set_cml_pec_fault()`
  - `pmbus_platform_set_status_other_output_oring_device_fault()`

## Current Notes

- Unsupported commands are classified as `STATUS_CML.INVALID_OR_UNSUPPORTED_COMMAND_RECEIVED`.
- Supported commands with malformed data or wrong transaction type are classified as `STATUS_CML.INVALID_OR_UNSUPPORTED_DATA_RECEIVED`.
- `Repeated START` read paths and PEC handling are supported in the transport layer.
- PMBus command capability is centralized in `pmbus_command_descriptor_t g_pmbus_command_descriptors[]`; parser, `QUERY`, and protocol detection must stay table-driven.
- `QUERY (0x1A)` and `SMBALERT_MASK (0x1B)` now use explicit `Block Write-Read Process Call` handling in the slave dispatcher.
- `USER_DATA_00..15`, unassigned `MFR_SPECIFIC_C4..FD`, and extended selectors are implemented as CRPS policy shadows so Table 31 host-visible dispatch coverage is complete.
- The CRPS policy shadows are not final product behavior; bind each entry to NVM, real telemetry/control, or documented product ownership before production use.
- `WRITE_PROTECT (0x10)` currently uses a conservative portable policy:
  - non-zero value blocks writable command handlers
  - `WRITE_PROTECT` itself and `CLEAR_FAULTS` remain writable
  - read-only commands remain available
- Host-side communication formats validated against the current Pico PMBus master path include:
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
    - `Group Command` transport sequencing
    - `CONTROL` line assert/deassert
    - `ALERT#` polling / optional ARA helper
- Pico `Full` checklist auto-validates the restorable `SMBALERT_MASK` shadow plus `ALERT#` poll and `CONTROL` GPIO helper paths against this reference slave.
- End-to-end CRPS product policy for `SMBALERT_MASK`, `ALERT# -> ARA`, and `CONTROL` effects remains product-specific.
- `Group Command` note:
  - the Pico host can now emit grouped write transport without STOP between segments
  - this matrix claims transport compatibility, not yet a formal guarantee that every application-level side effect is deferred until the final STOP for every writable command
- Bit/field semantic note:
  - `pmbus_semantics.c` logs recognized bit/field meanings after successful dispatch.
  - These logs are integration trace hooks; product owners still need to connect the hook to actual ADC/GPIO/control-loop/NVM behavior where required.
  - If a profile document defines bit fields for a command, the corresponding semantic handler must be added before marking the profile command product-complete.
- `ARA` is implemented through the platform alias-address contract when the MCU has spare slave address slots and lab validation policy enables it.
  - CRPS default policy should leave ARA disabled unless the final product contract explicitly requires it.
  - ARA response supports both host read styles:
    - 1-byte no-PEC read: returns the alerting slave write address.
    - 2-byte PEC read: returns the alerting slave write address plus SMBus PEC over `ARA read address + response address`.
  - On M031, the ARA alias is enabled only while ALERT# is asserted and not inhibited.
  - The first ARA response byte is loaded in the `SLA+R ACK` ISR path so the master does not read a stale previous TX byte.
  - The final ARA response byte clears the next ACK state so the controller reaches a transmit-finished status and releases the alias.
  - After ARA is served, the portable alias path is inhibited until ALERT# is actually deasserted; this keeps the normal PMBus address available for `STATUS_CML` / `CLEAR_FAULTS`.
  - The alias is released on the master NACK/STOP completion path.
- `ARP` is implemented as a framework-level alias at `0x61` when lab validation policy enables it.
  - CRPS default policy should leave ARP/UDID disabled unless the final product contract explicitly requires SMBus address resolution.
  - `PREPARE_TO_ARP`, `GET_UDID`, `DIRECTED_GET_UDID`, `ASSIGN_ADDRESS`, and `RESET_DEVICE` are recognized.
  - The 17-byte UDID is a product-data placeholder and must be bound to real manufacturing identity before production.
- `Zone` support includes standard `ZONE_CONFIG (0x07)` and `ZONE_ACTIVE (0x08)` plus a portable Zone Read alias.
  - M031 default alias allocation enables Zone Read at `0x28`.
  - Zone Write alias is configured as disabled by default because M031 has only four slave address registers.
- Fan command policy currently uses a portable heuristic:
  - warning when measured RPM falls below 90% of commanded target
  - fault when measured RPM falls below 80% of commanded target
