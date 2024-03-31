
UTIL_SRC = driver/board/mt7933_hdk/util

ifeq ($(MTK_MINICLI_ENABLE),y)
C_FILES  += $(UTIL_SRC)/src/board_cli.c
#C_FILES  += $(UTIL_SRC)/src/gpio_cli.c
ifeq ($(MTK_HAL_SLEEP_MANAGER_MODULE_ENABLE), y)
C_FILES  += $(UTIL_SRC)/src/sleep_manager_cli.c
endif
C_FILES  += $(UTIL_SRC)/src/lp_dvt_cli_aud.c
C_FILES  += $(UTIL_SRC)/src/lp_dvt_cli_bt.c
C_FILES  += $(UTIL_SRC)/src/lp_dvt_cli_wf.c
C_FILES  += $(UTIL_SRC)/src/lp_dvt_cli_clk.c
C_FILES  += $(UTIL_SRC)/src/lp_dvt_cli_conn.c
C_FILES  += $(UTIL_SRC)/src/lp_dvt_cli_dcm.c
C_FILES  += $(UTIL_SRC)/src/lp_dvt_cli_dsp.c
C_FILES  += $(UTIL_SRC)/src/lp_dvt_cli_pmu.c
C_FILES  += $(UTIL_SRC)/src/lp_dvt_cli_spm.c
C_FILES  += $(UTIL_SRC)/src/lp_dvt_cli_rtc.c
C_FILES  += $(UTIL_SRC)/src/dvt_cli.c
ifeq ($(MTK_GPIO_CLI_ENABLE), y)
C_FILES  += $(UTIL_SRC)/src/gpio_cli.c
endif
ifeq ($(MTK_THERMAL_CLI_ENABLE), y)
C_FILES  += $(UTIL_SRC)/src/thermal_cli.c
endif

ifeq ($(MTK_BOOT_CLI_ENABLE),y)
C_FILES  += $(UTIL_SRC)/src/boot_cli.c
endif

ifneq ($(filter y,$(MTK_SDIO_SLV_CLI_ENABLE) $(MTK_BSP_DVT_CLI_ENABLE)),)
C_FILES  += $(UTIL_SRC)/src/sdio_cli.c
endif

ifneq ($(filter y,$(MTK_SDIO_MASTER_CLI_ENABLE) $(MTK_BSP_DVT_CLI_ENABLE)),)
C_FILES  += $(UTIL_SRC)/src/msdc_cli.c
endif
endif  #MTK_MINICLI_ENABLE

C_FILES  += $(UTIL_SRC)/src/io_def.c
C_FILES  += $(UTIL_SRC)/src/format.c

#################################################################################
#include path
CFLAGS	+= -Iinc
CFLAGS  += -I$(SOURCE_DIR)/driver/board/mt7933_hdk/util/inc

ifeq ($(MTK_THERMAL_CLI_ENABLE), y)
CFLAGS  += -DMTK_THERMAL_CLI_ENABLE
endif

ifeq ($(MTK_GPIO_CLI_ENABLE), y)
CFLAGS  += -DMTK_GPIO_CLI_ENABLE
endif
