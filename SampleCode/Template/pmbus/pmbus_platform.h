#ifndef PMBUS_PLATFORM_H
#define PMBUS_PLATFORM_H

void pmbus_platform_init(void);
void pmbus_platform_background_task(void);

void pmbus_platform_set_power_good(unsigned char asserted);
void pmbus_platform_set_mfr_fault(unsigned char active);

void pmbus_platform_set_input_measurements(unsigned long vin_mv, unsigned long iin_ma, unsigned long pin_mw);
void pmbus_platform_set_output_measurements(unsigned long vout_mv, unsigned long iout_ma, unsigned long pout_mw);
void pmbus_platform_set_temperature_measurements(unsigned long temp1_mc, unsigned long temp2_mc, unsigned long temp3_mc);
void pmbus_platform_set_fan_measurements(unsigned long fan1_rpm, unsigned long fan2_rpm);

void pmbus_platform_set_status_vout_source(unsigned char active_bits);
void pmbus_platform_set_status_iout_source(unsigned char active_bits);
void pmbus_platform_set_status_input_source(unsigned char active_bits);
void pmbus_platform_set_status_temperature_source(unsigned char active_bits);
void pmbus_platform_set_status_cml_source(unsigned char active_bits);
void pmbus_platform_set_status_other_source(unsigned char active_bits);
void pmbus_platform_set_status_mfr_specific_source(unsigned char active_bits);
void pmbus_platform_set_status_fans_1_2_source(unsigned char active_bits);

void pmbus_platform_set_vout_ov_fault(unsigned char active);
void pmbus_platform_set_vout_ov_warning(unsigned char active);
void pmbus_platform_set_vout_uv_fault(unsigned char active);
void pmbus_platform_set_vout_uv_warning(unsigned char active);
void pmbus_platform_set_vout_tracking_error(unsigned char active);
void pmbus_platform_set_iout_oc_fault(unsigned char active);
void pmbus_platform_set_iout_oc_warning(unsigned char active);
void pmbus_platform_set_iout_uc_fault(unsigned char active);
void pmbus_platform_set_iout_current_share_fault(unsigned char active);
void pmbus_platform_set_iout_power_limiting(unsigned char active);
void pmbus_platform_set_input_vin_uv_fault(unsigned char active);
void pmbus_platform_set_input_vin_ov_fault(unsigned char active);
void pmbus_platform_set_input_iin_oc_fault(unsigned char active);
void pmbus_platform_set_input_unit_off_for_insufficient_input(unsigned char active);
void pmbus_platform_set_input_pin_op_warning(unsigned char active);
void pmbus_platform_set_temperature_ot_fault(unsigned char active);
void pmbus_platform_set_temperature_ot_warning(unsigned char active);
void pmbus_platform_set_temperature_ut_fault(unsigned char active);
void pmbus_platform_set_temperature_ut_warning(unsigned char active);
void pmbus_platform_set_fan_1_fault(unsigned char active);
void pmbus_platform_set_fan_2_fault(unsigned char active);
void pmbus_platform_set_fan_1_warning(unsigned char active);
void pmbus_platform_set_fan_2_warning(unsigned char active);
void pmbus_platform_set_airflow_fault(unsigned char active);
void pmbus_platform_set_cml_other_communication_fault(unsigned char active);
void pmbus_platform_set_cml_invalid_command(unsigned char active);
void pmbus_platform_set_cml_invalid_data(unsigned char active);
void pmbus_platform_set_cml_pec_fault(unsigned char active);
void pmbus_platform_set_cml_processor_fault(unsigned char active);
void pmbus_platform_set_status_other_output_oring_device_fault(unsigned char active);
void pmbus_platform_set_status_other_input_a_fuse_fault(unsigned char active);
void pmbus_platform_set_status_other_input_b_fuse_fault(unsigned char active);
void pmbus_platform_set_status_other_first_to_assert_smbalert(unsigned char active);

#endif
