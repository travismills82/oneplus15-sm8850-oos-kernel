#include "insensor_eeprom_read.h"

int insensor_eeprom_read(struct cam_sensor_ctrl_t *s_ctrl, struct cam_control *cmd)
{
	int rc = 0;
	CAM_ERR(CAM_SENSOR, "s_ctrl->sensor_name:%s", s_ctrl->sensor_name);
	if (strcmp(s_ctrl->sensor_name, "lafatele") == 0){
		uint8_t *otp_data = kmalloc(LAFATELE_MAP_SIZE, GFP_KERNEL);

		if (!otp_data) {
			pr_err("Failed to allocate OTP data buffer\n");
			return -ENOMEM;
		}
		memset(otp_data, 0, LAFATELE_MAP_SIZE);
		rc = lafatele_oplus_cam_insensor_eeprom(s_ctrl, otp_data);
		if (!rc) {
			CAM_ERR(CAM_SENSOR, "OTP data filled successfully");
			if (copy_to_user(u64_to_user_ptr(cmd->handle), otp_data, LAFATELE_MAP_SIZE)) {
				CAM_ERR(CAM_SENSOR, "Failed Copy to User, handle=%llx", cmd->handle);
				rc = -EFAULT;
				kfree(otp_data);
			}
		}
		kfree(otp_data);
	}
	return rc;
}