###################################################
# Sources
#  Add MTK_BLE_DM_SUPPORT = y in feature.mk
###################################################

BT_COMP_SOURCE = middleware/MTK/bluetooth_service/src

C_FILES  += $(BT_COMP_SOURCE)/bt_device_manager_le.c
#            $(BT_COMP_SOURCE)/bt_gatts_service.c

C_FILES  += $(BT_COMP_SOURCE)/bt_gap_le_service.c \
            $(BT_COMP_SOURCE)/bt_utils.c \
            $(BT_COMP_SOURCE)/bt_gattc_discovery.c

#ifeq ($(MTK_BLE_GAP_SRV_ENABLE), y)
#C_FILES  += $(BT_COMP_SOURCE)/bt_gap_le_service.c \
#            $(BT_COMP_SOURCE)/bt_gap_le_service_atci.c \
#            $(BT_COMP_SOURCE)/bt_gap_le_service_utils.c
#CFLAGS += -DMTK_BLE_GAP_SRV_ENABLE
#endif
###################################################
# include path
###################################################
CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/bluetooth_service/inc
CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/bluetooth/inc
CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/bt_callback_manager/inc
ifeq ($(MTK_NVDM_ENABLE), y)
CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/nvdm/inc
endif

