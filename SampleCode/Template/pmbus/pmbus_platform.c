#include "pmbus_types.h"
#include "board_config.h"
#include "pmbus_app.h"
#include "pmbus_platform.h"

static pmbus_platform_status_t g_pmbus_platform_status;
static unsigned long g_pmbus_platform_vin_mv;
static unsigned long g_pmbus_platform_iin_ma;
static unsigned long g_pmbus_platform_pin_mw;
static unsigned long g_pmbus_platform_vout_mv;
static unsigned long g_pmbus_platform_iout_ma;
static unsigned long g_pmbus_platform_pout_mw;
static unsigned long g_pmbus_platform_temp1_mc;
static unsigned long g_pmbus_platform_temp2_mc;
static unsigned long g_pmbus_platform_temp3_mc;
static unsigned long g_pmbus_platform_fan1_rpm;
static unsigned long g_pmbus_platform_fan2_rpm;

static pmbus_platform_status_t *pmbus_platform_status_reader(void)
{
    return &g_pmbus_platform_status;
}

static void pmbus_platform_update_source_byte(unsigned char *source_value,
    unsigned char set_mask,
    unsigned char clear_mask)
{
    *source_value = (unsigned char)((*source_value | set_mask) & (unsigned char)(~clear_mask));
}

static void pmbus_platform_set_source_bit(unsigned char *source_value,
    unsigned char bit_mask,
    unsigned char active)
{
    if (active != 0U)
    {
        pmbus_platform_update_source_byte(source_value, bit_mask, 0x00U);
    }
    else
    {
        pmbus_platform_update_source_byte(source_value, 0x00U, bit_mask);
    }
}

void pmbus_platform_init(void)
{
    g_pmbus_platform_status.power_good_asserted = 1U;
    g_pmbus_platform_status.mfr_fault_active = 0U;
    g_pmbus_platform_status.status_vout_source = 0x00U;
    g_pmbus_platform_status.status_iout_source = 0x00U;
    g_pmbus_platform_status.status_input_source = 0x00U;
    g_pmbus_platform_status.status_temperature_source = 0x00U;
    g_pmbus_platform_status.status_cml_source = 0x00U;
    g_pmbus_platform_status.status_other_source = 0x00U;
    g_pmbus_platform_status.status_mfr_specific_source = 0x00U;
    g_pmbus_platform_status.status_fans_1_2_source = 0x00U;
    g_pmbus_platform_vin_mv = 230000UL;
    g_pmbus_platform_iin_ma = 1250UL;
    g_pmbus_platform_pin_mw = 276000UL;
    g_pmbus_platform_vout_mv = 12000UL;
    g_pmbus_platform_iout_ma = 18500UL;
    g_pmbus_platform_pout_mw = 222000UL;
    g_pmbus_platform_temp1_mc = 35000UL;
    g_pmbus_platform_temp2_mc = 43000UL;
    g_pmbus_platform_temp3_mc = 50000UL;
    g_pmbus_platform_fan1_rpm = 12000UL;
    g_pmbus_platform_fan2_rpm = 11800UL;

    pmbus_app_register_platform_status_reader(pmbus_platform_status_reader);
}

void pmbus_platform_background_task(void)
{
    /*
        Keep this hook as the only platform polling entry.
        Future MS51/M032/M480 ports can sample GPIO, ADC, tach,
        or protection events here and only update the platform shadow.
    */
    pmbus_app_update_input_source(g_pmbus_platform_vin_mv, g_pmbus_platform_iin_ma, g_pmbus_platform_pin_mw);
    pmbus_app_update_output_source(g_pmbus_platform_vout_mv, g_pmbus_platform_iout_ma, g_pmbus_platform_pout_mw);
    pmbus_app_update_temperature_source(g_pmbus_platform_temp1_mc, g_pmbus_platform_temp2_mc, g_pmbus_platform_temp3_mc);
    pmbus_app_update_fan_source(g_pmbus_platform_fan1_rpm, g_pmbus_platform_fan2_rpm);
}

void pmbus_platform_set_power_good(unsigned char asserted)
{
    g_pmbus_platform_status.power_good_asserted = (unsigned char)(asserted & 0x01U);
}

void pmbus_platform_set_mfr_fault(unsigned char active)
{
    g_pmbus_platform_status.mfr_fault_active = (unsigned char)(active & 0x01U);
}

void pmbus_platform_set_input_measurements(unsigned long vin_mv, unsigned long iin_ma, unsigned long pin_mw)
{
    g_pmbus_platform_vin_mv = vin_mv;
    g_pmbus_platform_iin_ma = iin_ma;
    g_pmbus_platform_pin_mw = pin_mw;
}

void pmbus_platform_set_output_measurements(unsigned long vout_mv, unsigned long iout_ma, unsigned long pout_mw)
{
    g_pmbus_platform_vout_mv = vout_mv;
    g_pmbus_platform_iout_ma = iout_ma;
    g_pmbus_platform_pout_mw = pout_mw;
}

void pmbus_platform_set_temperature_measurements(unsigned long temp1_mc, unsigned long temp2_mc, unsigned long temp3_mc)
{
    g_pmbus_platform_temp1_mc = temp1_mc;
    g_pmbus_platform_temp2_mc = temp2_mc;
    g_pmbus_platform_temp3_mc = temp3_mc;
}

void pmbus_platform_set_fan_measurements(unsigned long fan1_rpm, unsigned long fan2_rpm)
{
    g_pmbus_platform_fan1_rpm = fan1_rpm;
    g_pmbus_platform_fan2_rpm = fan2_rpm;
}

void pmbus_platform_set_status_vout_source(unsigned char active_bits)
{
    g_pmbus_platform_status.status_vout_source = active_bits;
}

void pmbus_platform_set_status_iout_source(unsigned char active_bits)
{
    g_pmbus_platform_status.status_iout_source = active_bits;
}

void pmbus_platform_set_status_input_source(unsigned char active_bits)
{
    g_pmbus_platform_status.status_input_source = active_bits;
}

void pmbus_platform_set_status_temperature_source(unsigned char active_bits)
{
    g_pmbus_platform_status.status_temperature_source = active_bits;
}

void pmbus_platform_set_status_cml_source(unsigned char active_bits)
{
    g_pmbus_platform_status.status_cml_source = active_bits;
}

void pmbus_platform_set_status_other_source(unsigned char active_bits)
{
    g_pmbus_platform_status.status_other_source = active_bits;
}

void pmbus_platform_set_status_mfr_specific_source(unsigned char active_bits)
{
    g_pmbus_platform_status.status_mfr_specific_source = active_bits;
}

void pmbus_platform_set_status_fans_1_2_source(unsigned char active_bits)
{
    g_pmbus_platform_status.status_fans_1_2_source = active_bits;
}

void pmbus_platform_set_vout_ov_fault(unsigned char active)
{
    pmbus_platform_set_source_bit(&g_pmbus_platform_status.status_vout_source,
        PMBUS_STATUS_VOUT_VOUT_OV_FAULT,
        active);
}

void pmbus_platform_set_vout_ov_warning(unsigned char active)
{
    pmbus_platform_set_source_bit(&g_pmbus_platform_status.status_vout_source,
        PMBUS_STATUS_VOUT_VOUT_OV_WARNING,
        active);
}

void pmbus_platform_set_vout_uv_fault(unsigned char active)
{
    pmbus_platform_set_source_bit(&g_pmbus_platform_status.status_vout_source,
        PMBUS_STATUS_VOUT_VOUT_UV_FAULT,
        active);
}

void pmbus_platform_set_vout_uv_warning(unsigned char active)
{
    pmbus_platform_set_source_bit(&g_pmbus_platform_status.status_vout_source,
        PMBUS_STATUS_VOUT_VOUT_UV_WARNING,
        active);
}

void pmbus_platform_set_vout_tracking_error(unsigned char active)
{
    pmbus_platform_set_source_bit(&g_pmbus_platform_status.status_vout_source,
        PMBUS_STATUS_VOUT_VOUT_TRACKING_ERROR,
        active);
}

void pmbus_platform_set_iout_oc_fault(unsigned char active)
{
    pmbus_platform_set_source_bit(&g_pmbus_platform_status.status_iout_source,
        PMBUS_STATUS_IOUT_IOUT_OC_FAULT,
        active);
}

void pmbus_platform_set_iout_oc_warning(unsigned char active)
{
    pmbus_platform_set_source_bit(&g_pmbus_platform_status.status_iout_source,
        PMBUS_STATUS_IOUT_IOUT_OC_WARNING,
        active);
}

void pmbus_platform_set_iout_uc_fault(unsigned char active)
{
    pmbus_platform_set_source_bit(&g_pmbus_platform_status.status_iout_source,
        PMBUS_STATUS_IOUT_IOUT_UC_FAULT,
        active);
}

void pmbus_platform_set_iout_current_share_fault(unsigned char active)
{
    pmbus_platform_set_source_bit(&g_pmbus_platform_status.status_iout_source,
        PMBUS_STATUS_IOUT_CURRENT_SHARE_FAULT,
        active);
}

void pmbus_platform_set_iout_power_limiting(unsigned char active)
{
    pmbus_platform_set_source_bit(&g_pmbus_platform_status.status_iout_source,
        PMBUS_STATUS_IOUT_IN_POWER_LIMITING_MODE,
        active);
}

void pmbus_platform_set_input_vin_uv_fault(unsigned char active)
{
    pmbus_platform_set_source_bit(&g_pmbus_platform_status.status_input_source,
        PMBUS_STATUS_INPUT_VIN_UV_FAULT,
        active);
}

void pmbus_platform_set_input_vin_ov_fault(unsigned char active)
{
    pmbus_platform_set_source_bit(&g_pmbus_platform_status.status_input_source,
        PMBUS_STATUS_INPUT_VIN_OV_FAULT,
        active);
}

void pmbus_platform_set_input_iin_oc_fault(unsigned char active)
{
    pmbus_platform_set_source_bit(&g_pmbus_platform_status.status_input_source,
        PMBUS_STATUS_INPUT_IIN_OC_FAULT,
        active);
}

void pmbus_platform_set_input_unit_off_for_insufficient_input(unsigned char active)
{
    pmbus_platform_set_source_bit(&g_pmbus_platform_status.status_input_source,
        PMBUS_STATUS_INPUT_UNIT_OFF_FOR_INSUFFICIENT_INPUT_VOLTAGE,
        active);
}

void pmbus_platform_set_input_pin_op_warning(unsigned char active)
{
    pmbus_platform_set_source_bit(&g_pmbus_platform_status.status_input_source,
        PMBUS_STATUS_INPUT_PIN_OP_WARNING,
        active);
}

void pmbus_platform_set_temperature_ot_fault(unsigned char active)
{
    pmbus_platform_set_source_bit(&g_pmbus_platform_status.status_temperature_source,
        PMBUS_STATUS_TEMPERATURE_OT_FAULT,
        active);
}

void pmbus_platform_set_temperature_ot_warning(unsigned char active)
{
    pmbus_platform_set_source_bit(&g_pmbus_platform_status.status_temperature_source,
        PMBUS_STATUS_TEMPERATURE_OT_WARNING,
        active);
}

void pmbus_platform_set_temperature_ut_fault(unsigned char active)
{
    pmbus_platform_set_source_bit(&g_pmbus_platform_status.status_temperature_source,
        PMBUS_STATUS_TEMPERATURE_UT_FAULT,
        active);
}

void pmbus_platform_set_temperature_ut_warning(unsigned char active)
{
    pmbus_platform_set_source_bit(&g_pmbus_platform_status.status_temperature_source,
        PMBUS_STATUS_TEMPERATURE_UT_WARNING,
        active);
}

void pmbus_platform_set_fan_1_fault(unsigned char active)
{
    pmbus_platform_set_source_bit(&g_pmbus_platform_status.status_fans_1_2_source,
        PMBUS_STATUS_FANS_1_2_FAN_1_FAULT,
        active);
}

void pmbus_platform_set_fan_2_fault(unsigned char active)
{
    pmbus_platform_set_source_bit(&g_pmbus_platform_status.status_fans_1_2_source,
        PMBUS_STATUS_FANS_1_2_FAN_2_FAULT,
        active);
}

void pmbus_platform_set_fan_1_warning(unsigned char active)
{
    pmbus_platform_set_source_bit(&g_pmbus_platform_status.status_fans_1_2_source,
        PMBUS_STATUS_FANS_1_2_FAN_1_WARNING,
        active);
}

void pmbus_platform_set_fan_2_warning(unsigned char active)
{
    pmbus_platform_set_source_bit(&g_pmbus_platform_status.status_fans_1_2_source,
        PMBUS_STATUS_FANS_1_2_FAN_2_WARNING,
        active);
}

void pmbus_platform_set_airflow_fault(unsigned char active)
{
    pmbus_platform_set_source_bit(&g_pmbus_platform_status.status_fans_1_2_source,
        PMBUS_STATUS_FANS_1_2_AIRFLOW_FAULT,
        active);
}

void pmbus_platform_set_cml_other_communication_fault(unsigned char active)
{
    pmbus_platform_set_source_bit(&g_pmbus_platform_status.status_cml_source,
        PMBUS_STATUS_CML_OTHER_COMMUNICATION_FAULT,
        active);
}

void pmbus_platform_set_cml_invalid_command(unsigned char active)
{
    pmbus_platform_set_source_bit(&g_pmbus_platform_status.status_cml_source,
        PMBUS_STATUS_CML_INVALID_OR_UNSUPPORTED_COMMAND_RECEIVED,
        active);
}

void pmbus_platform_set_cml_invalid_data(unsigned char active)
{
    pmbus_platform_set_source_bit(&g_pmbus_platform_status.status_cml_source,
        PMBUS_STATUS_CML_INVALID_OR_UNSUPPORTED_DATA_RECEIVED,
        active);
}

void pmbus_platform_set_cml_pec_fault(unsigned char active)
{
    pmbus_platform_set_source_bit(&g_pmbus_platform_status.status_cml_source,
        PMBUS_STATUS_CML_PACKET_ERROR_CHECK_FAILED,
        active);
}

void pmbus_platform_set_cml_processor_fault(unsigned char active)
{
    pmbus_platform_set_source_bit(&g_pmbus_platform_status.status_cml_source,
        PMBUS_STATUS_CML_PROCESSOR_FAULT_DETECTED,
        active);
}

void pmbus_platform_set_status_other_output_oring_device_fault(unsigned char active)
{
    pmbus_platform_set_source_bit(&g_pmbus_platform_status.status_other_source,
        PMBUS_STATUS_OTHER_OUTPUT_ORING_DEVICE_FAULT,
        active);
}

void pmbus_platform_set_status_other_input_a_fuse_fault(unsigned char active)
{
    pmbus_platform_set_source_bit(&g_pmbus_platform_status.status_other_source,
        PMBUS_STATUS_OTHER_INPUT_A_FUSE_OR_BREAKER_FAULT,
        active);
}

void pmbus_platform_set_status_other_input_b_fuse_fault(unsigned char active)
{
    pmbus_platform_set_source_bit(&g_pmbus_platform_status.status_other_source,
        PMBUS_STATUS_OTHER_INPUT_B_FUSE_OR_BREAKER_FAULT,
        active);
}

void pmbus_platform_set_status_other_first_to_assert_smbalert(unsigned char active)
{
    pmbus_platform_set_source_bit(&g_pmbus_platform_status.status_other_source,
        PMBUS_STATUS_OTHER_FIRST_TO_ASSERT_SMBALERT,
        active);
}
