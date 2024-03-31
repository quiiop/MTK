/**
 *  Copyright (c) 2018 MediaTek Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */

#include <string.h>
#include "bt_driver.h"
#include "bt_buffer_mode.h"
#ifdef MTK_NVDM_ENABLE
#include "nvdm.h"
#endif
#include "hal_flash.h"

static struct btmtk_buf_mode_t btmtk_buffer_mode = {0};
extern int btmtk_send_hci_cmd(const unsigned char *cmd, const int cmd_len,
							  const unsigned char *event, const int event_len,
							  unsigned char *ret_event, int ret_event_len);

static bool btmtk_buffer_mode_check_supported_chip(uint16_t ID)
{
	/*
	 * To support 7933 and 7931, we only compare 79xx to check chip ID.
	 * This method is aligned with WIFI.
	 */
	if ((ID & 0xFF00) == 0x7900)
		return pdTRUE;

	return pdFALSE;
}

static int btmtk_buffer_mode_efuse_read(uint16_t addr, uint16_t *value)
{
	uint8_t efuse_r[READ_EFUSE_CMD_LEN] = {0x01, 0x6F, 0xFC, 0x0E,
										   0x01, 0x0D, 0x0A, 0x00, 0x02, 0x04,
										   0x00, 0x00, 0x00, 0x00,
										   0x00, 0x00, 0x00, 0x00};  // 4 sub block num(sub block 0~3)

	uint8_t efuse_r_event[READ_EFUSE_EVT_HDR_LEN] = {0x04, 0xE4, 0x1E, 0x02,
													 0x0D, 0x1A, 0x00, 02, 04};
	uint8_t ret_event_buffer[READ_EFUSE_EVT_COMPLETE_LEN] = {0};
	/*
	 * check event
	 * 04 E4 LEN(1B) 02 0D LEN(2Byte) 02 04 ADDR(2Byte) VALUE(4B) ADDR(2Byte)
	 * VALUE(4Byte) ADDR(2Byte) VALUE(4B)  ADDR(2Byte) VALUE(4Byte)
	 */
	int ret = 0;
	uint16_t sub_block = (addr / 16) * 4;
	uint8_t temp = 0;

	efuse_r[READ_EFUSE_CMD_BLOCK_OFFSET] = sub_block & 0xFF;
	efuse_r[READ_EFUSE_CMD_BLOCK_OFFSET + 1] = (sub_block & 0xFF00) >> 8;
	efuse_r[READ_EFUSE_CMD_BLOCK_OFFSET + 2] = (sub_block + 1) & 0xFF;
	efuse_r[READ_EFUSE_CMD_BLOCK_OFFSET + 3] = ((sub_block + 1) & 0xFF00) >> 8;
	efuse_r[READ_EFUSE_CMD_BLOCK_OFFSET + 4] = (sub_block + 2) & 0xFF;
	efuse_r[READ_EFUSE_CMD_BLOCK_OFFSET + 5] = ((sub_block + 2) & 0xFF00) >> 8;
	efuse_r[READ_EFUSE_CMD_BLOCK_OFFSET + 6] = (sub_block + 3) & 0xFF;
	efuse_r[READ_EFUSE_CMD_BLOCK_OFFSET + 7] = ((sub_block + 3) & 0xFF00) >> 8;

	//btif_util_dump_buffer("buf_mode_set_addr: Send ", efuse_r, READ_EFUSE_CMD_LEN, 0);
	ret = btmtk_send_hci_cmd(efuse_r, READ_EFUSE_CMD_LEN,
							 efuse_r_event, READ_EFUSE_EVT_HDR_LEN,
							 ret_event_buffer, READ_EFUSE_EVT_COMPLETE_LEN);
	if (ret >= 0) {
		BT_DRV_LOGD("buf_bin efuse-r: OK Event length %d", ret);
		//btif_util_dump_buffer("buf_mode_set_addr: Event ", ret_event_buffer,
		//					  READ_EFUSE_EVT_COMPLETE_LEN, 0);
	} else {
		BT_DRV_LOGE("buf_bin efuse-r: failed!!, err: %d", ret);
		return ret;
	}

	if (memcmp(ret_event_buffer, efuse_r_event, READ_EFUSE_EVT_HDR_LEN) == 0) {
		/*compare rxbuf format ok, compare addr*/
		BT_DRV_LOGD("buf_bin efuse-r: compare rxbuf format ok");
		if (efuse_r[READ_EFUSE_CMD_BLOCK_OFFSET] == ret_event_buffer[9] &&
			efuse_r[READ_EFUSE_CMD_BLOCK_OFFSET + 1] == ret_event_buffer[10] &&
			efuse_r[READ_EFUSE_CMD_BLOCK_OFFSET + 2] == ret_event_buffer[15] &&
			efuse_r[READ_EFUSE_CMD_BLOCK_OFFSET + 3] == ret_event_buffer[16] &&
			efuse_r[READ_EFUSE_CMD_BLOCK_OFFSET + 4] == ret_event_buffer[21] &&
			efuse_r[READ_EFUSE_CMD_BLOCK_OFFSET + 5] == ret_event_buffer[22] &&
			efuse_r[READ_EFUSE_CMD_BLOCK_OFFSET + 6] == ret_event_buffer[27] &&
			efuse_r[READ_EFUSE_CMD_BLOCK_OFFSET + 7] == ret_event_buffer[28]) {

			BT_DRV_LOGD("buf_bin efuse-r: addr compare ok");
			/*Get value*/
			temp = addr % 16;
			BT_DRV_LOGD("buf_bin efuse-r: addr in block %d",
						temp);
			switch (temp) {
			case 0:
			case 2:
				*value = ret_event_buffer[11 + temp];
				*value |= ret_event_buffer[11 + temp + 1] << 8;
				break;
			case 4:
			case 6:
				*value = ret_event_buffer[13 + temp];
				*value |= ret_event_buffer[13 + temp + 1] << 8;
				break;
			case 8:
			case 10:
				*value = ret_event_buffer[15 + temp];
				*value |= ret_event_buffer[15 + temp + 1] << 8;
				break;

			case 12:
			case 14:
				*value = ret_event_buffer[17 + temp];
				*value |= ret_event_buffer[17 + temp + 1] << 8;
				break;
			}
		} else {
			BT_DRV_LOGW("buf_bin efuse-r: addr compare fail");
			ret = -1;
		}
	} else {
		BT_DRV_LOGW("buf_bin efuse-r: compare rxbuf format fail");
		ret = -1;
	}

	return ret;
}

static void btmtk_buffer_mode_check_auto_mode(struct btmtk_buf_mode_t *buffer_mode)
{
	uint16_t chipID = 0;
	uint16_t ver = 0;

	if (buffer_mode->efuse_mode != AUTO_MODE) {
		BT_DRV_LOGI("buf_bin check mode: mode %d",
					buffer_mode->efuse_mode);
		return;
	}

	if (btmtk_buffer_mode_efuse_read((uint16_t)EFUSE_CHIP_ID_ADDR, &chipID) < 0) {
		BT_DRV_LOGW("buf_bin check mode: #1 Use EEPROM mode!");
		buffer_mode->efuse_mode = BIN_FILE_MODE;
		return;
	}

	if (btmtk_buffer_mode_efuse_read((uint16_t)EFUSE_VERSION, &ver) < 0) {
		BT_DRV_LOGW("buf_bin check mode: #2 Use EEPROM mode!");
		buffer_mode->efuse_mode = BIN_FILE_MODE;
		return;
	}

	BT_DRV_LOGI("buf_bin check mode: efuse: 0x%04x(0x%02x)",
				chipID, ver);
	/* Only when chip ID check pass and eFuse version is not 0, we will use eFuse mode */
	if (btmtk_buffer_mode_check_supported_chip(chipID) == pdTRUE && (ver & 0xF)) {
		BT_DRV_LOGI("buf_bin check mode: use efuse mode");
		buffer_mode->efuse_mode = EFUSE_MODE;
	} else {
		BT_DRV_LOGI("buf_bin check mode: use EEPROM mode");
		buffer_mode->efuse_mode = BIN_FILE_MODE;
	}
}

static int btmtk_buffer_mode_set_addr(struct btmtk_buf_mode_t *buffer_mode)
{
	uint8_t cmd[SET_ADDRESS_CMD_LEN] = {0x01, 0x1A, 0xFC, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	uint8_t event[SET_ADDRESS_EVT_LEN] = {0x04, 0x0E, 0x04, 0x01, 0x1A, 0xFC, 0x00};
	int ret = 0;

	cmd[SET_ADDRESS_CMD_PAYLOAD_OFFSET + 5] = buffer_mode->bt_mac[0];
	cmd[SET_ADDRESS_CMD_PAYLOAD_OFFSET + 4] = buffer_mode->bt_mac[1];
	cmd[SET_ADDRESS_CMD_PAYLOAD_OFFSET + 3] = buffer_mode->bt_mac[2];
	cmd[SET_ADDRESS_CMD_PAYLOAD_OFFSET + 2] = buffer_mode->bt_mac[3];
	cmd[SET_ADDRESS_CMD_PAYLOAD_OFFSET + 1] = buffer_mode->bt_mac[4];
	cmd[SET_ADDRESS_CMD_PAYLOAD_OFFSET] = buffer_mode->bt_mac[5];

	//btif_util_dump_buffer("buf_mode_set_addr: Send ", cmd, SET_ADDRESS_CMD_LEN, 0);
	ret = btmtk_send_hci_cmd(cmd, SET_ADDRESS_CMD_LEN, event, SET_ADDRESS_EVT_LEN, NULL, 0);
	if (ret < 0)
		BT_DRV_LOGE("buf_bin set addr failed!!, err: %d", ret);
	else
		BT_DRV_LOGI("buf_bin set addr: OK");

	return ret;
}

static int btmtk_buffer_mode_set_radio(struct btmtk_buf_mode_t *buffer_mode)
{
	uint8_t cmd[SET_RADIO_CMD_LEN] = {0x01, 0x2C, 0xFC, 0x08, 0x00, 0x00,
									  0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	uint8_t event[SET_RADIO_EVT_LEN] = {0x04, 0x0E, 0x04, 0x01, 0x2C, 0xFC, 0x00};
	int ret = 0;

	/* edr_init_pwr */
	cmd[SET_RADIO_CMD_EDR_DEF_OFFSET] = buffer_mode->bt_radio.radio_0 & 0x3F;
	/* ble_default_pwr */
	cmd[SET_RADIO_CMD_BLE_OFFSET] = buffer_mode->bt_radio.radio_2 & 0x3F;
	/* edr_max_pwr */
	cmd[SET_RADIO_CMD_EDR_MAX_OFFSET] = buffer_mode->bt_radio.radio_1 & 0x3F;
	/* edr_pwr_mode */
	cmd[SET_RADIO_CMD_EDR_MODE_OFFSET] = (buffer_mode->bt_radio.radio_0 & 0xC0) >> 6;

	//btif_util_dump_buffer("buf_mode_set_radio: Send", cmd, SET_RADIO_CMD_LEN, 0);
	ret = btmtk_send_hci_cmd(cmd, SET_RADIO_CMD_LEN, event, SET_RADIO_EVT_LEN, NULL, 0);
	if (ret < 0)
		BT_DRV_LOGE("buf_bin set radio failed!!, err: %d", ret);
	else
		BT_DRV_LOGI("buf_bin set radio: OK");

	return ret;
}

static int btmtk_buffer_mode_set_group_boundary(struct btmtk_buf_mode_t *buffer_mode)
{
	uint8_t cmd[SET_GRP_CMD_LEN] = {0x01, 0xEA, 0xFC, 0x09, 0x02, 0x0B, 0x00,
									0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	uint8_t event[SET_GRP_EVT_LEN] = {0x04, 0x0E, 0x04, 0x01, 0xEA, 0xFC, 0x00};
	int ret = 0;

	memcpy(&cmd[SET_GRP_CMD_PAYLOAD_OFFSET],
			buffer_mode->bt_ant0_grp_boundary, BUFFER_MODE_GROUP_LENGTH);

	//btif_util_dump_buffer("buf_mode_set_grp_bdry: Send ", cmd, SET_GRP_CMD_LEN, 0);
	ret = btmtk_send_hci_cmd(cmd, SET_GRP_CMD_LEN, event, SET_GRP_EVT_LEN, NULL, 0);
	if (ret < 0)
		BT_DRV_LOGE("buf_bin set gp bound failed!!, err: %d", ret);
	else
		BT_DRV_LOGI("buf_bin set gp bound: OK");

	return ret;
}

static int btmtk_buffer_mode_set_power_offset(struct btmtk_buf_mode_t *buffer_mode)
{
	uint8_t cmd[SET_PWR_OFFSET_CMD_LEN] = {0x01, 0xEA, 0xFC, 0x0A, 0x02, 0x0A, 0x00,
										   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	uint8_t event[SET_PWR_OFFSET_EVT_LEN] = {0x04, 0x0E, 0x04, 0x01, 0xEA, 0xFC, 0x00};
	int ret = 0;

	memcpy(&cmd[SET_PWR_OFFSET_CMD_PAYLOAD_OFFSET],
		   buffer_mode->bt_ant0_pwr_offset, BUFFER_MODE_CAL_LENGTH);

	//btif_util_dump_buffer("buf_mode_set_pwr_offset: Send ", cmd, SET_PWR_OFFSET_CMD_LEN, 0);
	ret = btmtk_send_hci_cmd(cmd, SET_PWR_OFFSET_CMD_LEN, event, SET_PWR_OFFSET_EVT_LEN, NULL, 0);
	if (ret < 0)
		BT_DRV_LOGE("buf_bin set pw offset failed!!, err: %d", ret);
	else
		BT_DRV_LOGI("buf_bin set pw offset: OK");

	return ret;
}

static int btmtk_buffer_mode_set_ant_to_pin_loss(struct btmtk_buf_mode_t *buffer_mode)
{
	uint8_t cmd[SET_ANT_TO_PIN_LOSS_CMD_LEN] = {0x01, 0xEA, 0xFC, 0x04, 0x02, 0x09, 0, 0};
	uint8_t event[SET_ANT_TO_PIN_LOSS_EVT_LEN] = {0x04, 0x0E, 0x04, 0x01, 0xEA, 0xFC, 0x00};
	int ret = 0;

	memcpy(&cmd[SET_ANT_TO_PIN_LOSS_CMD_PAYLOAD_OFFSET],
		   buffer_mode->bt_ant0_loss, BUFFER_MODE_LOSS_LENGHT);

	//btif_util_dump_buffer("buf_mode_set_ant: Send ", cmd, SET_PWR_OFFSET_CMD_LEN, 0);
	ret = btmtk_send_hci_cmd(cmd, SET_ANT_TO_PIN_LOSS_CMD_LEN, event,
							 SET_ANT_TO_PIN_LOSS_EVT_LEN, NULL, 0);
	if (ret < 0)
		BT_DRV_LOGE("buf_bin ant2pin loss failed!!, err: %d", ret);
	else
		BT_DRV_LOGI("buf_bin ant2pin loss: OK");

	return ret;
}

static int btmtk_buffer_mode_set_tx_power_offset(struct btmtk_buf_mode_t *buffer_mode)
{
	uint8_t cmd[SET_TX_POWER_CAL_CMD_LEN] = {0x01, 0x93, 0xFC, 0x10, 0x00, 0x00, 0x00,
											 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
											 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	uint8_t event[SET_TX_POWER_CAL_EVT_LEN] = {0x04, 0x0E, 0x04, 0x01, 0x93, 0xFC, 0x00};
	int ret = 0;

	memcpy(&cmd[SET_TX_POWER_CAL_CMD_PAYLOAD_OFFSET],
		   buffer_mode->bt_power_cal, BUFFER_MODE_LOSS_LENGHT);

	//btif_util_dump_buffer("buf_mode_set_tx_pwr_offset: Send ", cmd, SET_TX_POWER_CAL_CMD_LEN, 0);
	ret = btmtk_send_hci_cmd(
			cmd, SET_TX_POWER_CAL_CMD_LEN,
			event, SET_TX_POWER_CAL_EVT_LEN,
			NULL, 0);
	if (ret < 0)
		BT_DRV_LOGE("buf_bin set tx pw offset failed!!(%d)",
					 ret);
	else
		BT_DRV_LOGI("buf_bin set tx pw offset: OK");

	return ret;
}

int btmtk_buffer_mode_send(void)
{
	int ret = 0;

	btmtk_buffer_mode_check_auto_mode(&btmtk_buffer_mode);

	if (btmtk_buffer_mode.efuse_mode != BIN_FILE_MODE) {
		BT_DRV_LOGI("buf_bin send: mode %d",
					btmtk_buffer_mode.efuse_mode);
		return 0;
	}

	if (!btmtk_buffer_mode.ready) {
		BT_DRV_LOGE("buf_bin send: eeprom.bon did not been set!");
		return -1;
	}

	if ((btmtk_buffer_mode.bt_mac[0] == 0 && btmtk_buffer_mode.bt_mac[1] == 0 &&
		btmtk_buffer_mode.bt_mac[2] == 0 && btmtk_buffer_mode.bt_mac[3] == 0 &&
		btmtk_buffer_mode.bt_mac[4] == 0 && btmtk_buffer_mode.bt_mac[5] == 0) ||
		(btmtk_buffer_mode.bt_mac[0] == 0xFF && btmtk_buffer_mode.bt_mac[1] == 0xFF &&
		btmtk_buffer_mode.bt_mac[2] == 0xFF && btmtk_buffer_mode.bt_mac[3] == 0xFF &&
		btmtk_buffer_mode.bt_mac[4] == 0xFF && btmtk_buffer_mode.bt_mac[5] == 0xFF)) {
		BT_DRV_LOGW("buf_bin send: addr is invalid");
	} else {
		ret = btmtk_buffer_mode_set_addr(&btmtk_buffer_mode);
		if (ret < 0)
			BT_DRV_LOGE("buf_bin send: set addr failed");
	}

	ret = btmtk_buffer_mode_set_radio(&btmtk_buffer_mode);
	if (ret < 0)
		BT_DRV_LOGE("buf_bin send: set radio failed");

	ret = btmtk_buffer_mode_set_group_boundary(&btmtk_buffer_mode);
	if (ret < 0)
		BT_DRV_LOGE("buf_bin send: set group_boundary failed");

	// Only when Tx Power of EDR/BLE is 20 dbm would use 16 Groups offset
	if ((btmtk_buffer_mode.bt_radio.radio_1 & 0x3F) == 0x14 ||
		(btmtk_buffer_mode.bt_radio.radio_2 & 0x3F) == 0x14) {
		ret = btmtk_buffer_mode_set_tx_power_offset(&btmtk_buffer_mode);
		if (ret < 0)
			BT_DRV_LOGE("buf_bin send: set tx pwr offset(16 groups) failed");
	} else {
		ret = btmtk_buffer_mode_set_power_offset(&btmtk_buffer_mode);
		if (ret < 0)
			BT_DRV_LOGE("buf_bin send: set pwr offset(6 groups) failed");
	}

	ret = btmtk_buffer_mode_set_ant_to_pin_loss(&btmtk_buffer_mode);
	if (ret < 0)
		BT_DRV_LOGE("buf_bin send: set ant to pin loss failed");

	return 0;
}

bt_drv_addr_t *btmtk_get_eeprom_bt_addr(void)
{
	return &(btmtk_buffer_mode.bt_mac);
}

static void btmtk_buffer_mode_deinit_buffer(uint8_t *bufA, uint8_t *bufB, uint8_t *bufC)
{
	if (bufA)
		vPortFree(bufA);
	if (bufB)
		vPortFree(bufB);
	if (bufC)
		vPortFree(bufC);
}

int btmtk_buffer_mode_initialize(void)
{
	hal_flash_status_t flash_status;
	uint16_t eepromID = 0;
	uint8_t *buffer_addr_radio = NULL;
	uint8_t *buffer_power_cal = NULL;
	uint8_t *buffer_group_cal = NULL;

	btmtk_buffer_mode.efuse_mode = AUTO_MODE;

#ifdef MTK_NVDM_ENABLE
	uint32_t size = sizeof(int);
	nvdm_status_t nvdm_status;

	/*Read WIFI.cfg*/
	nvdm_status = nvdm_read_data_item(BUFFER_MODE_SWITCH_FILE, BUFFER_MODE_SWITCH_FIELD,
									  (uint8_t *)&btmtk_buffer_mode.efuse_mode, &size);
	if (nvdm_status != NVDM_STATUS_OK) {
		BT_DRV_LOGE("buf_bin init: read NVDM fail! %d", nvdm_status);
		return -1;
	}

	btmtk_buffer_mode.efuse_mode = btmtk_buffer_mode.efuse_mode - '0';

	if (btmtk_buffer_mode.efuse_mode == EFUSE_MODE) {
		BT_DRV_LOGI("buf_bin init: mode is %d",
					btmtk_buffer_mode.efuse_mode);
		return -1;
	}
#endif

	/*Check EEPROM Chip ID*/
	flash_status = hal_flash_read(CONN_BUF_BIN_BASE + EEPROM_CHIP_ID_OFFSET, (uint8_t *)&eepromID, EEPROM_CHIP_ID_LEN);
	if (flash_status != HAL_FLASH_STATUS_OK) {
		BT_DRV_LOGE("buf_bin init: read for Chip ID fail %d",
					flash_status);
		return -1;
	}

	/* If chip ID not in support list, we would regard it as not valid eeprom bin. */
	if (btmtk_buffer_mode_check_supported_chip(eepromID) == pdFALSE) {
		BT_DRV_LOGE("buf_bin init: Chip %x not support", eepromID);
		return -1;
	}
	BT_DRV_LOGI("buf_bin init: EEPROM for Chip %x", eepromID);

	/*Malloc buffer for read eeprom*/
	buffer_addr_radio = (uint8_t *)pvPortMalloc(BUFFER_LEN_ADDR_RADIO);
	if (buffer_addr_radio == NULL) {
		BT_DRV_LOGE("buf_bin init: malloc first buffer FAIL!");
		return -1;
	}

	buffer_power_cal = (uint8_t *)pvPortMalloc(BUFFER_MODE_POWER_CAL_LENGTH);
	if (buffer_power_cal == NULL) {
		BT_DRV_LOGE("buf_bin init: malloc second buffer FAIL!");
		btmtk_buffer_mode_deinit_buffer(buffer_addr_radio, buffer_power_cal, buffer_group_cal);
		return -1;
	}

	buffer_group_cal = (uint8_t *)pvPortMalloc(BUFFER_LEN_OFFSET_CAL);
	if (buffer_group_cal == NULL) {
		BT_DRV_LOGE("buf_bin init: malloc third buffer FAIL!");
		btmtk_buffer_mode_deinit_buffer(buffer_addr_radio, buffer_power_cal, buffer_group_cal);
		return -1;
	}

	/*Read EEPROM*/
	flash_status = hal_flash_read(CONN_BUF_BIN_BASE + BT_MAC_OFFSET,
								  buffer_addr_radio, BUFFER_LEN_ADDR_RADIO);
	if (flash_status != HAL_FLASH_STATUS_OK) {
		BT_DRV_LOGE("buf_bin init: read addr & radio fail %d",
					flash_status);
		btmtk_buffer_mode_deinit_buffer(buffer_addr_radio, buffer_power_cal, buffer_group_cal);
		return -1;
	}

	flash_status = hal_flash_read(CONN_BUF_BIN_BASE + BT_POWER_CAL_OFFSET,
								  buffer_power_cal, BUFFER_MODE_POWER_CAL_LENGTH);
	if (flash_status != HAL_FLASH_STATUS_OK) {
		BT_DRV_LOGE("buf_bin init: read addr & radio fail %d",
					flash_status);
		btmtk_buffer_mode_deinit_buffer(buffer_addr_radio, buffer_power_cal, buffer_group_cal);
		return -1;
	}

	flash_status = hal_flash_read(CONN_BUF_BIN_BASE + BT_LOSS_ANT0_OFFSET,
								  buffer_group_cal, BUFFER_LEN_OFFSET_CAL);
	if (flash_status != HAL_FLASH_STATUS_OK) {
		BT_DRV_LOGE("buf_bin init: read offset & cal fail %d",
					flash_status);
		btmtk_buffer_mode_deinit_buffer(buffer_addr_radio, buffer_power_cal, buffer_group_cal);
		return -1;
	}

	/* Already check the content, set it as ready */
	btmtk_buffer_mode.ready = 1;

	/* Write the content to data structure */
	//btif_util_dump_buffer("buf_mode_init: mac ",
	//	&buffer_addr_radio[0], BUFFER_MODE_MAC_LENGTH, 0);
	if ((buffer_addr_radio[0] == 0 && buffer_addr_radio[1] == 0 &&
		buffer_addr_radio[2] == 0 && buffer_addr_radio[3] == 0 &&
		buffer_addr_radio[4] == 0 && buffer_addr_radio[5] == 0) ||
		(buffer_addr_radio[0] == 0xFF && buffer_addr_radio[1] == 0xFF &&
		buffer_addr_radio[2] == 0xFF && buffer_addr_radio[3] == 0xFF &&
		buffer_addr_radio[4] == 0xFF && buffer_addr_radio[5] == 0xFF)) {
		BT_DRV_LOGW("buf_bin init: addr is invalid");
	} else {
		BT_DRV_LOGD("buf_bin init: copy addr to data structure");
		memcpy(btmtk_buffer_mode.bt_mac, &buffer_addr_radio[0], BUFFER_MODE_MAC_LENGTH);
	}

	//btif_util_dump_buffer("buf_mode_init: radio ",
	//					  &buffer_addr_radio[BT_RADIO_OFFSET - BT_MAC_OFFSET],
	//					  BUFFER_MODE_RADIO_LENGTH, 0);
	memcpy(&btmtk_buffer_mode.bt_radio,
		   &buffer_addr_radio[BT_RADIO_OFFSET - BT_MAC_OFFSET], BUFFER_MODE_RADIO_LENGTH);

	//btif_util_dump_buffer("buf_mode_init: POWER Cal ", buffer_power_cal,
	//					  BUFFER_MODE_POWER_CAL_LENGTH, 0);
	memcpy(&btmtk_buffer_mode.bt_power_cal, buffer_power_cal, BUFFER_MODE_POWER_CAL_LENGTH);

	//btif_util_dump_buffer("buf_mode_init: ant0 bound ",
	//					  &buffer_group_cal[BT_GROUP_ANT0_OFFSET - BT_LOSS_ANT0_OFFSET],
	//					  BUFFER_MODE_GROUP_LENGTH, 0);
	memcpy(btmtk_buffer_mode.bt_ant0_grp_boundary,
		   &buffer_group_cal[BT_GROUP_ANT0_OFFSET - BT_LOSS_ANT0_OFFSET], BUFFER_MODE_GROUP_LENGTH);

	//btif_util_dump_buffer("buf_mode_init: ant1 bound ",
	//					  &buffer_group_cal[BT_GROUP_ANT1_OFFSET - BT_LOSS_ANT0_OFFSET],
	//					  BUFFER_MODE_GROUP_LENGTH, 0);
	memcpy(btmtk_buffer_mode.bt_ant1_grp_boundary,
		   &buffer_group_cal[BT_GROUP_ANT1_OFFSET - BT_LOSS_ANT0_OFFSET],
		   BUFFER_MODE_GROUP_LENGTH);

	//btif_util_dump_buffer("buf_mode_init: ant0 offset ",
	//					  &buffer_group_cal[BT_CAL_ANT0_OFFSET - BT_LOSS_ANT0_OFFSET],
	//					  BUFFER_MODE_CAL_LENGTH, 0);
	memcpy(btmtk_buffer_mode.bt_ant0_pwr_offset,
		   &buffer_group_cal[BT_CAL_ANT0_OFFSET - BT_LOSS_ANT0_OFFSET], BUFFER_MODE_CAL_LENGTH);

	//btif_util_dump_buffer("buf_mode_init: ant1 offset ",
	//					  &buffer_group_cal[BT_CAL_ANT1_OFFSET - BT_LOSS_ANT0_OFFSET],
	//					  BUFFER_MODE_CAL_LENGTH, 0);
	memcpy(btmtk_buffer_mode.bt_ant1_pwr_offset,
		   &buffer_group_cal[BT_CAL_ANT1_OFFSET - BT_LOSS_ANT0_OFFSET], BUFFER_MODE_CAL_LENGTH);

	//btif_util_dump_buffer("buf_mode_init: ant0 loss ",
	//					  &buffer_group_cal[0], BUFFER_MODE_LOSS_LENGHT, 0);
	memcpy(btmtk_buffer_mode.bt_ant0_loss, &buffer_group_cal[0], BUFFER_MODE_LOSS_LENGHT);

	//btif_util_dump_buffer("buf_mode_init: ant1 loss ",
	//					  &buffer_group_cal[BT_LOSS_ANT1_OFFSET - BT_LOSS_ANT0_OFFSET],
	//					  BUFFER_MODE_LOSS_LENGHT, 0);
	memcpy(btmtk_buffer_mode.bt_ant1_loss,
		   &buffer_group_cal[BT_LOSS_ANT1_OFFSET - BT_LOSS_ANT0_OFFSET], BUFFER_MODE_LOSS_LENGHT);

	btmtk_buffer_mode_deinit_buffer(buffer_addr_radio, buffer_power_cal, buffer_group_cal);

	return 0;
}
