ifeq ($(MTK_BT_ENABLE), y)
###################################################
# Libs
###################################################
LIB_DIR := $(SOURCE_DIR)/prebuilt/middleware/MTK/bluetooth/lib

ifeq ($(IC_CONFIG), mt7933)
LIB_DIR := $(SOURCE_DIR)/prebuilt/middleware/MTK/bluetooth/lib/mt7933
endif

ifeq ($(IC_CONFIG), mt8512)
LIB_DIR := $(SOURCE_DIR)/prebuilt/middleware/MTK/bluetooth/lib/mt8512
endif

ifeq ($(MTK_BT_LIB_RELEASE_ENABLE), y)
ifeq ($(MTK_BLE_ONLY_ENABLE), y)
ifeq ($(IC_CONFIG), $(filter $(IC_CONFIG),mt2523 mt2523s mt2533))
$(error mt25x3 serail not support MTK_BLE_ONLY_ENABLE)
endif
ifeq ($(MTK_BT_NO_HB_BT_5_2), y)
LIBS += $(LIB_DIR)/libble_release.a
else
LIBS += $(LIB_DIR)/libble_la_release.a
endif

ifeq ($(MTK_MT7933_BT_ENABLE), y)
LIBS += $(LIB_DIR)/libbledriver_7933_release.a
endif
else

ifeq ($(MTK_BT_NO_HB_BT_5_2), y)
LIBS += $(LIB_DIR)/libbt_release.a
else
LIBS += $(LIB_DIR)/libbt_la_release.a
endif

ifeq ($(MTK_BT_HFP_ENABLE), y)
LIBS += $(LIB_DIR)/libbt_hfp_release.a
endif
ifeq ($(MTK_BT_A2DP_ENABLE), y)
LIBS += $(LIB_DIR)/libbt_a2dp_release.a
endif
ifeq ($(MTK_BT_AVRCP_ENABLE), y)
LIBS += $(LIB_DIR)/libbt_avrcp_release.a
endif
ifeq ($(MTK_BT_AVRCP_ENH_ENABLE), y)
LIBS += $(LIB_DIR)/libbt_avrcp_enhance_release.a
endif
ifeq ($(MTK_BT_PBAP_ENABLE), y)
LIBS += $(LIB_DIR)/libbt_pbapc_release.a
endif
ifeq ($(MTK_BT_SPP_ENABLE), y)
LIBS += $(LIB_DIR)/libbt_spp_release.a
endif
ifeq ($(MTK_MT7933_BT_ENABLE), y)
LIBS += $(LIB_DIR)/libbtdriver_7933_release.a
endif
endif

ifeq ($(MTK_BT_DONGLE_TEST), y)
LIBS += $(LIB_DIR)/libbtdriver_bt_dongle.a
else ifeq ($(PLATFORM_DEVICE), BAND)
LIBS += $(LIB_DIR)/libbtdriver_2523_tx.a
else ifeq ($(IC_CONFIG), $(filter $(IC_CONFIG),mt2523 mt2523s))
LIBS += $(LIB_DIR)/libbtdriver_2523_release.a
else ifeq ($(IC_CONFIG), mt2533)
LIBS += $(LIB_DIR)/libbtdriver_2523_release.a
else ifeq ($(IC_CONFIG), mt7687)
LIBS += $(LIB_DIR)/libbtdriver_7697_release.a
else ifeq ($(IC_CONFIG), mt7697)
LIBS += $(LIB_DIR)/libbtdriver_7697_release.a
LIBS += $(LIB_DIR)/libble_multi_adv.a
endif

else #MTK_BT_LIB_RELEASE_ENABLE

ifeq ($(MTK_BLE_ONLY_ENABLE), y)
ifeq ($(IC_CONFIG), $(filter $(IC_CONFIG),mt2523 mt2523s mt2533))
$(error mt25x3 serail not support MTK_BLE_ONLY_ENABLE)
endif
ifneq ($(wildcard $(strip $(SOURCE_DIR))/middleware/MTK/bt_hummingbird),)
ifeq ($(MTK_BT_NO_HB_BT_5_2), y)
 MODULE_PATH += $(SOURCE_DIR)/middleware/MTK/bt_hummingbird/project/rtos_src_build/hblib_ble
else
 MODULE_PATH += $(SOURCE_DIR)/middleware/MTK/bt_hummingbird/project/rtos_src_build/hblib_ble_la
endif
endif
ifeq ($(MTK_BT_NO_HB_BT_5_2), y)
LIBS += $(LIB_DIR)/libble.a
else
LIBS += $(LIB_DIR)/libble_la.a
endif
ifeq ($(MTK_MT7933_BT_ENABLE), y)
ifneq ($(wildcard $(strip $(SOURCE_DIR))/middleware/MTK/bt_hummingbird),)
 MODULE_PATH += $(SOURCE_DIR)/middleware/MTK/bt_hummingbird/project/rtos_src_build/driverlib_7933/
endif
 LIBS += $(LIB_DIR)/libbledriver_7933.a
endif
else

#Duo BT setting
ifneq ($(wildcard $(strip $(SOURCE_DIR))/middleware/MTK/bt_hummingbird),)
ifeq ($(MTK_BT_NO_HB_BT_5_2), y)
 MODULE_PATH += $(SOURCE_DIR)/middleware/MTK/bt_hummingbird/project_duo/rtos_src_build/hblib_ble
else
 MODULE_PATH += $(SOURCE_DIR)/middleware/MTK/bt_hummingbird/project_duo/rtos_src_build/hblib_ble_la
endif
endif
ifeq ($(MTK_BT_NO_HB_BT_5_2), y)
LIBS += $(LIB_DIR)/libbt.a
else
LIBS += $(LIB_DIR)/libbt_la.a
endif

ifeq ($(MTK_BT_HFP_ENABLE), y)
LIBS += $(LIB_DIR)/libbt_hfp.a
endif

ifeq ($(MTK_BT_A2DP_ENABLE), y)
ifneq ($(wildcard $(strip $(SOURCE_DIR))/middleware/MTK/bt_hummingbird),)
 MODULE_PATH += $(SOURCE_DIR)/middleware/MTK/bt_hummingbird/project_duo/rtos_src_build/a2dp_lib
endif
 LIBS += $(LIB_DIR)/libbt_a2dp.a
endif

ifeq ($(MTK_BT_AVRCP_ENABLE), y)
ifneq ($(wildcard $(strip $(SOURCE_DIR))/middleware/MTK/bt_hummingbird),)
 MODULE_PATH += $(SOURCE_DIR)/middleware/MTK/bt_hummingbird/project_duo/rtos_src_build/avrcp_lib
endif
 LIBS += $(LIB_DIR)/libbt_avrcp.a
endif

ifeq ($(MTK_BT_AVRCP_ENH_ENABLE), y)
ifneq ($(wildcard $(strip $(SOURCE_DIR))/middleware/MTK/bt_hummingbird),)
 MODULE_PATH += $(SOURCE_DIR)/middleware/MTK/bt_hummingbird/project_duo/rtos_src_build/avrcp_eh_lib
endif
 LIBS += $(LIB_DIR)/libbt_avrcp_enhance.a
endif

ifeq ($(MTK_BT_PBAP_ENABLE), y)
LIBS += $(LIB_DIR)/libbt_pbapc.a
endif

ifeq ($(MTK_BT_SPP_ENABLE), y)
ifneq ($(wildcard $(strip $(SOURCE_DIR))/middleware/MTK/bt_hummingbird),)
 MODULE_PATH += $(SOURCE_DIR)/middleware/MTK/bt_hummingbird/project_duo/rtos_src_build/spp_lib
endif
LIBS += $(LIB_DIR)/libbt_spp.a
endif

ifeq ($(MTK_BT_AWS_ENABLE), y)
CFLAGS += -DMTK_BT_AWS_ENABLE
LIBS += $(LIB_DIR)/libbt_aws.a
endif

ifeq ($(MTK_BT_HID_ENABLE), y)
LIBS += $(LIB_DIR)/libbt_hid.a
endif

ifeq ($(MTK_MT7933_BT_ENABLE), y)
ifneq ($(wildcard $(strip $(SOURCE_DIR))/middleware/MTK/bt_hummingbird),)
 MODULE_PATH += $(SOURCE_DIR)/middleware/MTK/bt_hummingbird/project_duo/rtos_src_build/driverlib_7933/
endif
 LIBS += $(LIB_DIR)/libbtdriver_7933.a
endif

endif

ifeq ($(MTK_BT_DONGLE_TEST), y)
LIBS += $(LIB_DIR)/libbtdriver_bt_dongle.a
else ifeq ($(PLATFORM_DEVICE), BAND)
LIBS += $(LIB_DIR)/libbtdriver_2523_tx.a
else ifeq ($(IC_CONFIG), $(filter $(IC_CONFIG),mt2523 mt2523s))
LIBS += $(LIB_DIR)/libbtdriver_2523.a
else ifeq ($(IC_CONFIG), mt2533)
LIBS += $(LIB_DIR)/libbtdriver_2523.a
else ifeq ($(IC_CONFIG), mt7687)
LIBS += $(LIB_DIR)/libbtdriver_7697.a
else ifeq ($(IC_CONFIG), mt7697)
LIBS += $(LIB_DIR)/libbtdriver_7697.a
LIBS += $(LIB_DIR)/libble_multi_adv.a
endif


endif

ifeq ($(MTK_BT_TIMER_EXTERNAL_ENABLE), y)
CFLAGS += -DMTK_BT_TIMER_EXTERNAL_ENABLE
endif

###################################################
# Sources
###################################################

CFLAGS += -D__BT_DEBUG__
BLUETOOTH_SRC = middleware/MTK/bluetooth/src
BLUETOOTH_FILES = $(BLUETOOTH_SRC)/bt_debug.c \
               $(BLUETOOTH_SRC)/bt_hci_log.c \
               $(BLUETOOTH_SRC)/bt_log.c \
               $(BLUETOOTH_SRC)/bt_os_layer_api.c \
               $(BLUETOOTH_SRC)/bt_task.c              

ifeq ($(MTK_BT_TIMER_EXTERNAL_ENABLE), y)
BLUETOOTH_FILES += $(BLUETOOTH_SRC)/bt_timer_external.c
endif

ifeq ($(IC_CONFIG), mt7687)
BLUETOOTH_FILES += $(BLUETOOTH_SRC)/bt_cli.c
else ifeq ($(IC_CONFIG), mt7697)
BLUETOOTH_FILES += $(BLUETOOTH_SRC)/bt_cli.c
else ifeq ($(IC_CONFIG), mt7933)
BLUETOOTH_FILES += $(BLUETOOTH_SRC)/bt_cli.c
endif

C_FILES += $(BLUETOOTH_FILES)

###################################################
# include path
###################################################

CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/bluetooth/inc
CFLAGS += -I$(SOURCE_DIR)/prebuilt/middleware/MTK/bluetooth/inc
CFLAGS += -I$(SOURCE_DIR)/middleware/third_party/mbedtls/include
CFLAGS += -I$(SOURCE_DIR)/middleware/third_party/mbedtls/configs
# definition

endif
