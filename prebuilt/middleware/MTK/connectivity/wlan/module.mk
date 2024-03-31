LIBS    += $(SOURCE_DIR)/prebuilt/middleware/MTK/connectivity/wlan/lib/$(WF_LIB)

CFLAGS += -I$(SOURCE_DIR)/prebuilt/middleware/MTK/connectivity/wlan/include
CFLAGS += -I$(SOURCE_DIR)/prebuilt/middleware/MTK/connectivity/wlan/os/
CFLAGS += -I$(SOURCE_DIR)/prebuilt/middleware/MTK/connectivity/wlan/include/nic
CFLAGS += -I$(SOURCE_DIR)/prebuilt/middleware/MTK/connectivity/wlan/include/mgmt
CFLAGS += -I$(SOURCE_DIR)/prebuilt/middleware/MTK/connectivity/wlan/include/chips
CFLAGS += -I$(SOURCE_DIR)/prebuilt/middleware/MTK/connectivity/wlan/os/freertos/hif/axi/include
CFLAGS += -I$(SOURCE_DIR)/prebuilt/middleware/MTK/connectivity/wlan/os/freertos/hif/common/include
CFLAGS += -I$(SOURCE_DIR)/prebuilt/middleware/MTK/connectivity/wlan/os/freertos/include
CFLAGS += -I$(SOURCE_DIR)/prebuilt/middleware/MTK/connectivity/wlan_service/glue/osal/include
CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/minisupp/src_protected/wpa_supplicant_2.6/src

CFLAGS  += -I$(SOURCE_DIR)/middleware/third_party/lwip/src/include
CFLAGS  += -I$(SOURCE_DIR)/middleware/third_party/lwip/ports/include


