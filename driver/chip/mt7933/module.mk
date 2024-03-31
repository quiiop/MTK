include $(SOURCE_DIR)/driver/chip/mt7933/src/GCC/module.mk

include $(SOURCE_DIR)/driver/board/mt7933_hdk/hw_resource_assignment/module.mk

ifneq ($(wildcard $(strip $(SOURCE_DIR))/driver/chip/mt7933/src_core/),)
    include $(SOURCE_DIR)/driver/chip/mt7933/src_core/GCC/module.mk
else
    ifeq ($(MTK_TFM_ENABLE),y)
        ifneq ($(findstring bootloader, $(APP_PATH)), bootloader)
            ifeq ($(MTK_RELEASE_MODE),release)
                LIBS += $(SOURCE_DIR)/prebuilt/driver/chip/mt7933/lib/libhal_core_CM33_TFM_NS_slim_GCC.a
            else
                LIBS += $(SOURCE_DIR)/prebuilt/driver/chip/mt7933/lib/libhal_core_CM33_TFM_NS_GCC.a
            endif
        else
            LIBS += $(SOURCE_DIR)/prebuilt/driver/chip/mt7933/lib/libhal_core_CM33_GCC.a
        endif
    else
        ifeq ($(MTK_RELEASE_MODE),release)
            LIBS += $(SOURCE_DIR)/prebuilt/driver/chip/mt7933/lib/libhal_core_CM33_slim_GCC.a
        else
            LIBS += $(SOURCE_DIR)/prebuilt/driver/chip/mt7933/lib/libhal_core_CM33_GCC.a
        endif
    endif

    ifneq ($(MTK_HAL_SLA_LIB_ALL_IN_ONE), y)
        ifeq ($(MTK_HAL_EFUSE_MODULE_ENABLE), y)
            LIBS += $(SOURCE_DIR)/prebuilt/driver/chip/mt7933/lib/libefuse.a
        endif
        ifeq ($(MTK_HAL_GCPU_MODULE_ENABLE), y)
            LIBS += $(SOURCE_DIR)/prebuilt/driver/chip/mt7933/lib/lib_gcpu.a
        endif
    endif
endif


ifeq ($(MTK_SECURE_BOOT_ENABLE),y)

    dir := $(wildcard $(strip $(SOURCE_DIR))/middleware/MTK/sboot/GCC)
    CFLAGS += -DMTK_SECURE_BOOT_ENABLE

    ifneq ($(dir),)
        include $(dir)/module.mk
    else
        LIBS += $(SOURCE_DIR)/prebuilt/middleware/MTK/sboot/libsboot_CM33_GCC.a
    endif

endif
