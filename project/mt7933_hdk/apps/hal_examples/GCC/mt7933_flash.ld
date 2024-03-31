/*
 *        MT7933 CM33 Memory Map
 */

HIDDEN(__STACK_SIZE     = 0x1000);
HIDDEN(__SYSRAM_OFFSET  = 0x0);

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)

/* Entry Point */
ENTRY(Reset_Handler)

/* Memory Spaces Definitions: 16MB flash  */
MEMORY
{
    ROM_BL(rx)        : ORIGIN = 0x18000000, LENGTH = 64K
    ROM_RBL(rx)       : ORIGIN = 0x18010000, LENGTH = 8K
    ROM_TFM(rx)       : ORIGIN = 0x18012000, LENGTH = 200K
    ROM_RTOS(rx)      : ORIGIN = 0x18044080, LENGTH = 3696K - 0x80
    ROM_NVDM(rx)      : ORIGIN = 0x183E0000, LENGTH = 64K
    ROM_LOG(rx)       : ORIGIN = 0x183F0000, LENGTH = 64K
    /*
    Reserved For TFM  : LENGTH = 152 K
    Free For Customer : LENGTH = 500 K
    FOTA              : LENGTH = 8424 K
    */
    ROM_DSP(rx)       : ORIGIN = 0x18C41000, LENGTH = 2556K
    ROM_BT(rx)        : ORIGIN = 0x18EC0000, LENGTH = 580K
    /*
    Reserved For FW   : LENGTH = 40 K
    */
    ROM_WIFI_PWRTBL(rx): ORIGIN = 0x18F5B000, LENGTH = 20K
    ROM_WIFI_EXT(rx)  : ORIGIN = 0x18F60000, LENGTH = 636K
    ROM_BUFFER_BIN(rx): ORIGIN = 0x18FFF000, LENGTH = 4K

    TCM(rwx)          : ORIGIN = 0x0010C000, LENGTH = 20K
    RAM_BTFW(rwx)     : ORIGIN = 0xA0000000, LENGTH = 352K
    RAM_TFM(rwx)      : ORIGIN = 0xA0380000, LENGTH = 512K
    RAM_WFFW(rwx)     : ORIGIN = 0xA0400000, LENGTH = 312K
    RAM(rwx)          : ORIGIN = 0xA044E000, LENGTH = 3784K
    SYSRAM(rwx)       : ORIGIN = 0x80000000, LENGTH = 1024K

    VROM(rx)          : ORIGIN = 0x18000000, LENGTH = 4096K
    VRAM_BTFW(rwx)    : ORIGIN = 0x10000000, LENGTH = 352K
    VRAM_TFM(rwx)     : ORIGIN = 0x10380000, LENGTH = 512K
    VRAM_WFFW(rwx)    : ORIGIN = 0x10400000, LENGTH = 312K
    VRAM(rwx)         : ORIGIN = 0x1044E000, LENGTH = 3784K
    VSYSRAM(rwx)      : ORIGIN = 0x08000000, LENGTH = 1024K
}


/****************************************************************************
 *
 * FORWARD DECLARATIONS - SECTION SYMBOLS @ FLASH PHYSICAL ADDRESS
 *
 ****************************************************************************/


_rom_bl_start           = ORIGIN(ROM_BL)            - ORIGIN(ROM_BL);
_rom_bl_length          = LENGTH(ROM_BL);
_rom_rbl_start          = ORIGIN(ROM_RBL)           - ORIGIN(ROM_BL);
_rom_rbl_length         = LENGTH(ROM_RBL);
_rom_tfm_start          = ORIGIN(ROM_TFM)           - ORIGIN(ROM_BL);
_rom_tfm_length         = LENGTH(ROM_TFM);
_rom_rtos_start         = ORIGIN(ROM_RTOS)          - ORIGIN(ROM_BL);
_rom_rtos_length        = LENGTH(ROM_RTOS);
_rom_wifi_patch_start   = 0;/*ORIGIN(ROM_WIFI_PATCH)    - ORIGIN(ROM_BL);*/
_rom_wifi_patch_length  = 0;/*LENGTH(ROM_WIFI_PATCH);*/
_rom_wifi_start         = 0;/*ORIGIN(ROM_WIFI)          - ORIGIN(ROM_BL);*/
_rom_wifi_length        = 0;/* LENGTH(ROM_WIFI);*/
_rom_wifi_ext_start     = ORIGIN(ROM_WIFI_EXT)      - ORIGIN(ROM_BL);
_rom_wifi_ext_length    = LENGTH(ROM_WIFI_EXT);
_rom_wifi_pwrtbl_start  = ORIGIN(ROM_WIFI_PWRTBL)   - ORIGIN(ROM_BL);
_rom_wifi_pwrtbl_length = LENGTH(ROM_WIFI_PWRTBL);
_rom_buffer_bin_start   = ORIGIN(ROM_BUFFER_BIN)    - ORIGIN(ROM_BL);
_rom_buffer_bin_length  = LENGTH(ROM_BUFFER_BIN);
_rom_bt_start           = ORIGIN(ROM_BT)            - ORIGIN(ROM_BL);
_rom_bt_length          = LENGTH(ROM_BT);
_rom_dsp_start          = ORIGIN(ROM_DSP)           - ORIGIN(ROM_BL);
_rom_dsp_length         = LENGTH(ROM_DSP);
_rom_nvdm_start         = ORIGIN(ROM_NVDM)          - ORIGIN(ROM_BL);
_rom_nvdm_length        = LENGTH(ROM_NVDM);
_rom_log_start          = ORIGIN(ROM_LOG)           - ORIGIN(ROM_BL);
_rom_log_length         = LENGTH(ROM_LOG);


/****************************************************************************
 *
 * FORWARD DECLARATIONS - SECTION SYMBOLS @ MEMORY BUS ADDRESS
 *
 ****************************************************************************/


_xip_bl_addr            = ORIGIN(ROM_BL);
_xip_rbl_addr           = ORIGIN(ROM_RBL);
_xip_tfm_addr           = ORIGIN(ROM_TFM);
_xip_rtos_addr          = ORIGIN(ROM_RTOS);
_xip_nvdm_start         = ORIGIN(ROM_NVDM);
_xip_log_start          = ORIGIN(ROM_LOG);
_xip_dsp_start          = ORIGIN(ROM_DSP);
_xip_bt_start           = ORIGIN(ROM_BT);

_sysram_start           = ORIGIN(SYSRAM);
_sysram_length          = LENGTH(SYSRAM);
_ram_start              = ORIGIN(RAM);
_ram_length             = LENGTH(RAM);
_ram_wifi_ext_start     = ORIGIN(RAM_WFFW);
_ram_wifi_ext_length    = LENGTH(RAM_WFFW);
_vram_start             = ORIGIN(VRAM);
_vram_length            = LENGTH(VRAM);
_vsysram_start          = ORIGIN(VSYSRAM);
_vsysram_length         = LENGTH(VSYSRAM);
_tcm_start              = ORIGIN(TCM);
_tcm_length             = LENGTH(TCM);

 /* Highest address of the stack */
_stack_end = ORIGIN(TCM) + LENGTH(TCM);    /* end of TCM */

/* stack start */
_stack_start = _stack_end - __STACK_SIZE;

/* SYSRAM Region0 Offset for BROM */
_region0_offset = 0x13000 ;


SECTIONS
{
    . = ORIGIN(ROM_RTOS);
    .text ALIGN(4):
    {
        _text_start = .;
        KEEP(*(.vectorsTop))
        KEEP(*(.vectors))
        *(.text*)
        KEEP(*(.init))
        KEEP(*(.fini))
        *crtbegin.o(.ctors)
        *crtbegin?.o(.ctors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
        *(SORT(.ctors.*))
        *(.ctors)
        *crtbegin.o(.dtors)
        *crtbegin?.o(.dtors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
        *(SORT(.dtors.*))
        *(.dtors)
        *(.rodata*)
        *(.rom_rtos)
        KEEP(*(.eh_frame*))
        . = ALIGN(4);
        _text_end = .;
    } > ROM_RTOS AT> ROM_RTOS

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > ROM_RTOS AT> ROM_RTOS

    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > ROM_RTOS AT> ROM_RTOS
    __exidx_end = .;

    . = ALIGN(32);
    _sysram_code_load = LOADADDR(.sysram_text);
    .sysram_text ORIGIN(VSYSRAM)+_region0_offset :
    {
        _sysram_code_start = .;
        *(.sysram_code)
        *(.sysram_rodata)
        . = ALIGN(4);
        _sysram_code_end = .;
    } > VSYSRAM   AT> ROM_RTOS

    . = ALIGN(4);
    _sysram_data_load = LOADADDR(.cached_sysram_data);
    .cached_sysram_data  :
    {
        _sysram_data_start = .;
        *(vtable)
        *(.data)
        *(.data.*)
        *(.sysram_rwdata)
        . = ALIGN(4);
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN (__preinit_array_end = .);

        . = ALIGN(4);
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE_HIDDEN (__init_array_end = .);

        . = ALIGN(4);
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        PROVIDE_HIDDEN (__fini_array_end = .);
        KEEP(*(.jcr*))
        . = ALIGN(4);
        _sysram_data_end = .;
    } > VSYSRAM  AT> ROM_RTOS

    . = ALIGN(4);
    .bss (NOLOAD) :
    {
        _sysram_bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        *(.sysram_zidata)
        *(.sysram_swla_zidata)
        . = ALIGN(4);
        _sysram_bss_end = .;
    } > VSYSRAM  AT> ROM_RTOS


    . = ALIGN(32);
    _noncached_sysram_code_load = LOADADDR(.noncached_sysram_text);
    . = . + ORIGIN(SYSRAM) - ORIGIN(VSYSRAM);
    .noncached_sysram_text . :
    {
        _noncached_sysram_code_start = .;
        *(.noncached_sysram_code)
        *(.noncached_sysram_rodata)
         . = ALIGN(4);
        _noncached_sysram_code_end = .;
    } > SYSRAM  AT> ROM_RTOS

    . = ALIGN(4);
    _noncached_sysram_data_load = LOADADDR(.noncached_sysram_data);
    .noncached_sysram_data . :
    {
        _noncached_sysram_data_start = .;
        *(.noncached_sysram_rwdata)
        *(.secure_settings_section)
         . = ALIGN(4);
        _noncached_sysram_data_end = .;
    } > SYSRAM  AT> ROM_RTOS

    . = ALIGN(4);
    .noncached_sysram_bss . (NOLOAD) :
    {
        _noncached_sysram_bss_start = .;
        *(.noncached_sysram_zidata)
        . = ALIGN(4);
        _noncached_sysram_bss_end = .;
    } > SYSRAM  AT> ROM_RTOS

    . = ALIGN(32);
    .btfw_code (NOLOAD) :
    {
        *(.bt_firmware_emi_code)
        . = ALIGN(4);
    } > VRAM_BTFW   AT> ROM_RTOS

    . = ALIGN(32);
    .wffw_code (NOLOAD) :
    {
        *(.wf_firmware_emi_code)
        . = ALIGN(4);
    } > RAM_WFFW   AT> ROM_RTOS

    . = ALIGN(32);
    _ram_code_load = LOADADDR(.cached_ram_text);
    .cached_ram_text :
    {
        _ram_code_start = .;
        *(.ram_code)
        *(.ram_rodata)
        . = ALIGN(4);
        _ram_code_end = .;
    } > VRAM   AT> ROM_RTOS

    . = ALIGN(4);
    _ram_data_load = LOADADDR(.cached_ram_data);
    .cached_ram_data :
    {
        _ram_data_start = .;
        *(.ram_rwdata)
        . = ALIGN(4);
        _ram_data_end = .;
    } > VRAM  AT> ROM_RTOS

    . = ALIGN(4);
    _ram_bss_load = LOADADDR(.cached_ram_bss);
    .cached_ram_bss (NOLOAD) :
    {
        _ram_bss_start = .;
        *(.ram_zidata)
        . = ALIGN(4);
        _ram_bss_end = .;
    } > VRAM  AT> ROM_RTOS


    . = ALIGN(32);
    _noncached_ram_code_load = LOADADDR(.noncached_ram_text);
    . = . + ORIGIN(RAM) - ORIGIN(VRAM);
    .noncached_ram_text . :
    {
        _noncached_ram_code_start = .;
        *(.noncached_ram_code)
        *(.noncached_ram_rodata)
        . = ALIGN(4);
        _noncached_ram_code_end = .;
    } > RAM  AT> ROM_RTOS

    . = ALIGN(4);
    _noncached_ram_data_load = LOADADDR(.noncached_ram_data);
    .noncached_ram_data . :
    {
        _noncached_ram_data_start = .;
        *(.noncached_ram_rwdata)
        . = ALIGN(4);
        _noncached_ram_data_end = .;
    } > RAM  AT> ROM_RTOS

    . = ALIGN(4);
    _noncached_ram_bss_load = LOADADDR(.noncached_ram_bss);
    .noncached_ram_bss . (NOLOAD) :
    {
        _noncached_ram_bss_start = .;
        *(.noncached_ram_zidata)
        . = ALIGN(4);
        _noncached_ram_bss_end = .;
    } > RAM  AT> ROM_RTOS


    . = ALIGN(4);
    _tcm_code_load = LOADADDR(.tcm_text);
    .tcm_text :
    {
        _tcm_code_start = .;
        *(.tcm_code)
        *(.tcm_rodata)
        . = ALIGN(4);
        _tcm_code_end = .;
    }> TCM  AT> ROM_RTOS

   . = ALIGN(4);
    _tcm_data_load = LOADADDR(.tcm_data);
    .tcm_data :
    {
        _tcm_data_start = .;
        . = ALIGN(4);
        *(.tcm_rwdata)
        . = ALIGN(4);
        _tcm_data_end = .;
    }> TCM  AT> ROM_RTOS

    . = ALIGN(4);
    .tcm_bss (NOLOAD) :
    {
        _tcm_bss_start = .;
        *(.tcm_zidata)
        . = ALIGN(4);
        *(.tcm_wakeup_info)
        . = ALIGN(4);
        _tcm_bss_end = .;
    }> TCM AT> ROM_RTOS


    __end__ = .;

    .stack (ORIGIN(TCM) + LENGTH(TCM) - __STACK_SIZE) (COPY) :
    {
        . = ALIGN(4);
        __StackLimit = .;
        PROVIDE ( end = . );
        PROVIDE ( _end = . );
        . = . + __STACK_SIZE;
        . = ALIGN(4);
        __StackTop = .;
    } > TCM

    PROVIDE(__stack = __StackTop);

    ASSERT(__StackLimit >= __end__, "region TCM overflowed with stack")
}

