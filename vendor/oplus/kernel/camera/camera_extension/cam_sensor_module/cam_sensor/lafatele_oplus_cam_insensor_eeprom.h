#ifndef _OPLUS_CAM_INSENSOR_EEPROM_BEFOREREAD_H_
#define _OPLUS_CAM_INSENSOR_EEPROM_BEFOREREAD_H_

#include "cam_eeprom_dev.h"
#include "cam_sensor_dev.h"

#define RETRY_NUM_MAX 3
#define CHECK_NUM_MAX 5
#define PAGE_NUM_MAX 10
#define PAGE_INIT_REG_NUM_MAX 5280
#define PAGE_LOAD_REG_NUM_MAX 8
#define PAGE_INIT_PRE1_REG_NUM_MAX 8
#define PAGE_INIT_PRE2_REG_NUM_MAX 8
#define LAFATELE_MAP_SIZE 38272


struct OtpGroupInfo
{
	uint32_t IsAvailable;
	uint32_t ItemNum;
	uint32_t CheckItemOffset[CHECK_NUM_MAX];
	uint32_t GroupFlag;
	uint32_t SelectGroupNum;
};
struct OtpItemInfo
{
	uint32_t IsAvailable;
	uint32_t start_addr;
	uint32_t end_addr;
	uint32_t checksum_addr;
};

struct OtpCheckInfo
{
	struct OtpGroupInfo groupInfo;
	struct OtpItemInfo ItemInfo[CHECK_NUM_MAX];
};

struct camera_eeprom_map_info
{
	uint32_t page_num;
	uint32_t group_num;
	uint32_t page_num_pergroup;
	uint32_t max_retry_num;
	uint32_t select_group;
	uint32_t valid_data_size;
	struct cam_eeprom_map_t eeprom_page_map[PAGE_NUM_MAX];
};

struct camera_eeprom_reg_info
{
	uint32_t slave_addr;
	uint32_t init_regs_num;
	uint32_t load_regs_num;
	uint32_t init_pre1_regs_num;
	uint32_t init_pre2_regs_num;
	struct cam_sensor_i2c_reg_array init_regs[RETRY_NUM_MAX][PAGE_INIT_REG_NUM_MAX];
	struct cam_sensor_i2c_reg_array load_regs[PAGE_NUM_MAX][PAGE_LOAD_REG_NUM_MAX];
	struct cam_sensor_i2c_reg_array init_pre1_regs[PAGE_NUM_MAX][PAGE_INIT_PRE1_REG_NUM_MAX];
	struct cam_sensor_i2c_reg_array init_pre2_regs[PAGE_NUM_MAX][PAGE_INIT_PRE2_REG_NUM_MAX];
};

int lafatele_oplus_cam_insensor_eeprom(struct cam_sensor_ctrl_t *s_ctrl, uint8_t *otp_data_out);
#endif
