###################################################
# Sources
###################################################

BT_CONNECTION_MANAGER_SOURCE = middleware/MTK/bt_connection_manager/src

C_FILES  += $(BT_CONNECTION_MANAGER_SOURCE)/bt_connection_manager_config.c \
            $(BT_CONNECTION_MANAGER_SOURCE)/bt_connection_manager.c \
            $(BT_CONNECTION_MANAGER_SOURCE)/bt_connection_manager_utils.c \
            $(BT_CONNECTION_MANAGER_SOURCE)/bt_connection_manager_adapt.c
#            $(BT_CONNECTION_MANAGER_SOURCE)/bt_connection_manager_atci_cmd.c \
#            $(BT_CONNECTION_MANAGER_SOURCE)/bt_connection_manager_power.c
#            $(BT_CONNECTION_MANAGER_SOURCE)/bt_connection_manager_device_local_info.c \


###################################################
# include path
###################################################
CFLAGS  += -I$(SOURCE_DIR)/middleware/MTK/bt_connection_manager/inc
CFLAGS  += -I$(SOURCE_DIR)/prebuilt/middleware/MTK/bluetooth/inc
 
