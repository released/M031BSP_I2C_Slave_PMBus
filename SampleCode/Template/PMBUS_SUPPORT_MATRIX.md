# PMBus Support Matrix

This matrix documents the current PMBus device support in the portable framework.

Compliance target:
- Treat this M032 PMBus slave as a standard-product target for:
  - `PMBus-Specification-Rev-1-3-1-Part-II-20150313.pdf`
  - `PMBus_rev_1.2_part_1_september_2010.pdf`
- A command is product-complete only when its protocol, data format, status/error behavior, PEC behavior, and product-side side effects match the compliance target.
- Host-visible placeholders and config shadows are allowed during staged CRPS development, but they are not final product behavior until bound to real CRPS data/control sources or explicitly accepted by product policy.
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
| `VOUT_MODE` | `0x20` | Read Byte, Write Byte | R/W | Config shadow | ULINEAR16 exponent byte |
| `VOUT_COMMAND` | `0x21` | Read Word, Write Word, Process Call | R/W | Config shadow | ULINEAR16 target voltage word |
| `VOUT_TRIM` | `0x22` | Read Word, Write Word | R/W | Config shadow | ULINEAR16 trim shadow |
| `VOUT_CAL_OFFSET` | `0x23` | Read Word, Write Word | R/W | Config shadow | ULINEAR16 calibration offset shadow |
| `VOUT_MAX` | `0x24` | Read Word, Write Word | R/W | Config shadow | ULINEAR16 maximum output-voltage shadow |
| `VOUT_MARGIN_HIGH` | `0x25` | Read Word, Write Word | R/W | Config shadow | ULINEAR16 high-margin voltage shadow |
| `VOUT_MARGIN_LOW` | `0x26` | Read Word, Write Word | R/W | Config shadow | ULINEAR16 low-margin voltage shadow |
| `VOUT_TRANSITION_RATE` | `0x27` | Read Word, Write Word | R/W | Config shadow | LINEAR11 transition-rate shadow |
| `VOUT_DROOP` | `0x28` | Read Word, Write Word | R/W | Config shadow | LINEAR11 droop shadow |
| `VOUT_SCALE_LOOP` | `0x29` | Read Word, Write Word | R/W | Config shadow | LINEAR11 loop-scale shadow |
| `VOUT_SCALE_MONITOR` | `0x2A` | Read Word, Write Word | R/W | Config shadow | LINEAR11 monitor-scale shadow |
| `VOUT_MIN` | `0x2B` | Read Word, Write Word | R/W | Config shadow | ULINEAR16 minimum output-voltage shadow |
| `COEFFICIENTS` | `0x30` | Block Write-Read Process Call | R | Config shadow | Returns fixed 5-byte DIRECT coefficient placeholder; TODO replace if DIRECT-format commands are exposed |
| `POUT_MAX` | `0x31` | Read Word, Write Word | R/W | Config shadow | LINEAR11 output-power max shadow; TODO bind to CRPS rating/control policy |
| `MAX_DUTY` | `0x32` | Read Word, Write Word | R/W | Config shadow | LINEAR11 duty-cycle max shadow; TODO bind to PWM/control-loop limit |
| `FREQUENCY_SWITCH` | `0x33` | Read Word, Write Word | R/W | Config shadow | LINEAR11 switching-frequency shadow; TODO bind to power-stage control |
| `POWER_MODE` | `0x34` | Read Byte, Write Byte | R/W | Config shadow | Power-mode byte shadow; TODO bind to actual power-mode state machine |
| `VIN_ON` | `0x35` | Read Word, Write Word | R/W | Config shadow | LINEAR11 VIN-on threshold shadow; TODO bind to startup policy |
| `VIN_OFF` | `0x36` | Read Word, Write Word | R/W | Config shadow | LINEAR11 VIN-off threshold shadow; TODO bind to shutdown policy |
| `INTERLEAVE` | `0x37` | Read Word, Write Word | R/W | Config shadow | 16-bit interleave shadow; TODO bind to phase/interleave implementation if used |
| `IOUT_CAL_GAIN` | `0x38` | Read Word, Write Word | R/W | Config shadow | LINEAR11 current-sense gain calibration shadow; TODO bind to calibration data |
| `IOUT_CAL_OFFSET` | `0x39` | Read Word, Write Word | R/W | Config shadow | LINEAR11 current-sense offset calibration shadow; TODO bind to calibration data |
| `VOUT_OV_FAULT_LIMIT` | `0x40` | Read Word, Write Word | R/W | Config shadow + Telemetry policy | ULINEAR16 threshold; auto-sets `STATUS_VOUT.VOUT_OV_FAULT` |
| `VOUT_OV_FAULT_RESPONSE` | `0x41` | Read Byte, Write Byte | R/W | Config shadow | Response-policy byte shadow; product shutdown/retry behavior is platform policy |
| `VOUT_OV_WARN_LIMIT` | `0x42` | Read Word, Write Word | R/W | Config shadow + Telemetry policy | ULINEAR16 threshold; auto-sets `STATUS_VOUT.VOUT_OV_WARNING` |
| `VOUT_UV_WARN_LIMIT` | `0x43` | Read Word, Write Word | R/W | Config shadow + Telemetry policy | ULINEAR16 threshold; auto-sets `STATUS_VOUT.VOUT_UV_WARNING` |
| `VOUT_UV_FAULT_LIMIT` | `0x44` | Read Word, Write Word | R/W | Config shadow + Telemetry policy | ULINEAR16 threshold; auto-sets `STATUS_VOUT.VOUT_UV_FAULT` |
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
| `POWER_GOOD_ON` | `0x5E` | Read Word, Write Word | R/W | Config shadow | ULINEAR16 power-good assert threshold shadow; TODO bind to PG logic |
| `POWER_GOOD_OFF` | `0x5F` | Read Word, Write Word | R/W | Config shadow | ULINEAR16 power-good deassert threshold shadow; TODO bind to PG logic |
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
| `READ_VOUT` | `0x8B` | Read Word | R | Telemetry shadow | ULINEAR16 encoded VOUT |
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
| `MFR_VOUT_MIN` | `0xA4` | Read Word | R | Vendor shadow | Mirrors VOUT_MIN shadow. TODO: bind to SKU rating |
| `MFR_VOUT_MAX` | `0xA5` | Read Word | R | Vendor shadow | Mirrors VOUT_MAX shadow. TODO: bind to SKU rating |
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
| `MFR_MAX_TEMP_1` | `0xC0` | Read Word | R | Vendor shadow + Telemetry shadow | Mirrors current temperature 1 placeholder. TODO: implement peak-hold/latch policy |
| `MFR_MAX_TEMP_2` | `0xC1` | Read Word | R | Vendor shadow + Telemetry shadow | Mirrors current temperature 2 placeholder. TODO: implement peak-hold/latch policy |
| `MFR_MAX_TEMP_3` | `0xC2` | Read Word | R | Vendor shadow + Telemetry shadow | Mirrors current temperature 3 placeholder. TODO: implement peak-hold/latch policy |
| `MFR_SPECIFIC_C4..FD` | `0xC4..0xFD` | Block Read, Block Write | R/W | CRPS policy shadow | Unassigned manufacturer commands use volatile 16-byte scratchpads. Existing `D0/D6/D7/D8/DC` product behaviors are preserved and not overridden |
| `MFR_COLD_REDUNDANCY_CONFIG` | `0xD0` | Read Byte, Write Byte | R/W | Vendor shadow | Vendor config byte |
| `MFR_FWUPLOAD_MODE` | `0xD6` | Read Byte, Write Byte | R/W | Vendor shadow | Firmware upload mode byte |
| `MFR_FWUPLOAD` | `0xD7` | Block Write | W | Vendor shadow | FW upload block parser and state tracking |
| `MFR_FWUPLOAD_STATUS` | `0xD8` | Read Word | R | Vendor shadow | FW upload status word |
| `MFR_BLACKBOX` | `0xDC` | Block Read | R | Vendor shadow + Status shadow | 32-byte live/latched diagnostic snapshot |
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
- `Group Command` note:
  - the Pico host can now emit grouped write transport without STOP between segments
  - this matrix claims transport compatibility, not yet a formal guarantee that every application-level side effect is deferred until the final STOP for every writable command
- `ARA` is implemented through the platform alias-address contract when the MCU has spare slave address slots.
  - ARA response supports both host read styles:
    - 1-byte no-PEC read: returns the alerting slave write address.
    - 2-byte PEC read: returns the alerting slave write address plus SMBus PEC over `ARA read address + response address`.
  - On M031, the ARA alias is enabled only while ALERT# is asserted and not inhibited.
  - The first ARA response byte is loaded in the `SLA+R ACK` ISR path so the master does not read a stale previous TX byte.
  - The final ARA response byte clears the next ACK state so the controller reaches a transmit-finished status and releases the alias.
  - After ARA is served, the portable alias path is inhibited until ALERT# is actually deasserted; this keeps the normal PMBus address available for `STATUS_CML` / `CLEAR_FAULTS`.
  - The alias is released on the master NACK/STOP completion path.
- `ARP` is implemented as a framework-level alias at `0x61`.
  - `PREPARE_TO_ARP`, `GET_UDID`, `DIRECTED_GET_UDID`, `ASSIGN_ADDRESS`, and `RESET_DEVICE` are recognized.
  - The 17-byte UDID is a product-data placeholder and must be bound to real manufacturing identity before production.
- `Zone` support includes standard `ZONE_CONFIG (0x07)` and `ZONE_ACTIVE (0x08)` plus a portable Zone Read alias.
  - M031 default alias allocation enables Zone Read at `0x28`.
  - Zone Write alias is configured as disabled by default because M031 has only four slave address registers.
- Fan command policy currently uses a portable heuristic:
  - warning when measured RPM falls below 90% of commanded target
  - fault when measured RPM falls below 80% of commanded target
