###################################################
# Sources
###################################################

BT_DEVICE_MGR_SOURCE = middleware/MTK/bluetooth_service/src
BT_DEVICE_MGR_FILES =  $(BT_DEVICE_MGR_SOURCE)/bt_device_manager_common.c \
                       $(BT_DEVICE_MGR_SOURCE)/bt_device_manager_db.c \

ifeq ($(MTK_BT_DUO_ENABLE), y)
BT_DEVICE_MGR_FILES +=  $(BT_DEVICE_MGR_SOURCE)/bt_device_manager.c \
                       $(BT_DEVICE_MGR_SOURCE)/bt_device_manager_remote_info.c \
                       $(BT_DEVICE_MGR_SOURCE)/bt_device_manager_power.c
endif

C_FILES += $(BT_DEVICE_MGR_FILES)
###################################################
# include path
###################################################

CFLAGS  += -I$(SOURCE_DIR)/middleware/MTK/bluetooth_service/inc
