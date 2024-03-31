/*
 *        MT7933 CM33 Memory Map
 */

HIDDEN(__STACK_SIZE     = 0x3000);
HIDDEN(__SYSRAM_OFFSET  = 0x1000);

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)

/* Entry Point */
ENTRY(ResetISR)

/* HEADER_OFFSET to be specified by linker command */

INCLUDE memory.ld

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
_rom_fota_start         = ORIGIN(ROM_FOTA)          - ORIGIN(ROM_BL);
_rom_fota_length        = LENGTH(ROM_FOTA);
_rom_mfg_start          = ORIGIN(ROM_MFG)          - ORIGIN(ROM_BL);
_rom_mfg_length         = LENGTH(ROM_MFG);
_rom_wifi_patch_start   = ORIGIN(ROM_WIFI_PATCH)    - ORIGIN(ROM_BL);
_rom_wifi_patch_length  = LENGTH(ROM_WIFI_PATCH);
_rom_wifi_start         = ORIGIN(ROM_WIFI_EXT)      - ORIGIN(ROM_BL);
_rom_wifi_length        = LENGTH(ROM_WIFI_EXT);
_rom_wifi_pwrtbl_start  = ORIGIN(ROM_WIFI_PWRTBL)   - ORIGIN(ROM_BL);
_rom_wifi_pwrtbl_length = LENGTH(ROM_WIFI_PWRTBL);
_rom_buffer_bin_start   = ORIGIN(ROM_BUFFER_BIN)    - ORIGIN(ROM_BL);
_rom_buffer_bin_length  = LENGTH(ROM_BUFFER_BIN);
_rom_dsp_start          = ORIGIN(ROM_DSP)           - ORIGIN(ROM_BL);
_rom_dsp_length         = LENGTH(ROM_DSP);
_rom_bt_start           = ORIGIN(ROM_BT)            - ORIGIN(ROM_BL);
_rom_bt_length          = LENGTH(ROM_BT);
_rom_nvdm_start         = ORIGIN(ROM_NVDM)          - ORIGIN(ROM_BL);
_rom_nvdm_length        = LENGTH(ROM_NVDM);


/****************************************************************************
 *
 * FORWARD DECLARATIONS - SECTION SYMBOLS @ MEMORY BUS ADDRESS
 *
 ****************************************************************************/


_xip_tfm_addr           = ORIGIN(ROM_TFM);
_xip_rtos_addr          = ORIGIN(ROM_RTOS);
_xip_fota_addr          = ORIGIN(ROM_FOTA);
_xip_mfg_addr           = ORIGIN(ROM_MFG);
_xip_dsp_addr           = ORIGIN(ROM_DSP);
_xip_bt_addr            = ORIGIN(ROM_BT);
_xip_wifi_addr          = ORIGIN(ROM_WIFI_EXT);

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


SECTIONS
{
    . = ORIGIN(ROM_BL);

    .text ORIGIN(ROM_BL) + HEADER_OFFSET :
    {
        _text_start = .;

        *(.except_vectors)
        *(.reset_isr)
        *(.text .text.*)
        *(.rodata .rodata.*)
        *hal_cache.o(.tcm_code .tcm_rodata)

        _text_end = .;

    } > ROM_BL

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > ROM_BL

    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > ROM_BL

    .copy.table :
    {
        . = ALIGN(4);
        _copy_table_start = .;
        LONG(_sysram_code_load);
        LONG(_sysram_code_start);
        LONG(_sysram_code_end - _sysram_code_start);

        LONG(_data_lma_start);
        LONG(_data_start);
        LONG(_data_end - _data_start);

        LONG(_tcm_text_lma_start);
        LONG(_tcm_text_start);
        LONG(_tcm_text_end - _tcm_text_start);

        LONG(_tcm_data_lma_start);
        LONG(_tcm_data_start);
        LONG(_tcm_data_end - _tcm_data_start);
        _copy_table_end = .;
    } > ROM_BL

    .zero.table :
    {
        . = ALIGN(4);
        _zero_table_start = .;
        LONG(_tcm_bss_start);
        LONG(_tcm_bss_end - _tcm_bss_start);
        LONG(_bss_start);
        LONG(_bss_end - _bss_start);
        LONG(_bss2_start);
        LONG(_bss2_end - _bss2_start);
        _zero_table_end = .;
    } > ROM_BL

    . = ALIGN(4);
    _sysram_code_load = LOADADDR(.sysram_text);
    .sysram_text ORIGIN(VSYSRAM) + __SYSRAM_OFFSET :
    {
        _sysram_code_start = .;
        *(.sysram_code)
        *(.sysram_rodata)
        . = ALIGN(4);
        _sysram_code_end = .;
    } > VSYSRAM AT> ROM_BL

    . = ALIGN(4);
    _data_lma_start = LOADADDR(.data);
    .data :
    {
        . = ALIGN(4);
        _data_start = .;
        *(.data .data.*)
        *(.sysram_rwdata)
        . = ALIGN(4);
        _data_end = .;

    } > VSYSRAM AT> ROM_BL

    . = ALIGN(4);
    .bss (NOLOAD) :
    {
        _bss_start = .;

        *(EXCLUDE_FILE(*fota_osal.o) .bss)
        *(EXCLUDE_FILE(*fota_osal.o) .bss.*)
        *(COMMON)
        *(.sysram_zidata)

        . = ALIGN(4);
        _bss_end   = .;
    } > VSYSRAM

    . = ALIGN(4);
    .bss2 (NOLOAD) :
    {
        _bss2_start = .;

        *(.bss .bss.*)

        . = ALIGN(4);
        _bss2_end   = .;
    } > VSYSRAM

    .stack ALIGN(4) (NOLOAD) :
    {
        _stack_limit = .;
        . = . + __STACK_SIZE;
        . = ALIGN(4);
        _stack_top = .;
    } > VSYSRAM

    . = ALIGN(4);
    _end = . ; /* after stack, reserved for _sbrk heap */
    end  = . ;

    .sysram_rsvd_for_rtos (NOLOAD) :
    {
        /* 0x1000   4KB for REGION 0 of BootROM */
        /* 0x12000 72KB for BL ram-text/data    */
        . = . + LENGTH(VSYSRAM) - 0x12000 - __SYSRAM_OFFSET;
    } > VSYSRAM

    _tcm_text_lma_start = LOADADDR(.tcm_text);
    .tcm_text :
    {
        . = ALIGN(4);
        _tcm_text_start = .;
        KEEP(*(.ramTEXT))
        *(.tcm_code)
        *(.tcm_rodata)
        . = ALIGN(4);
        _tcm_text_end = .;
    } > TCM AT> ROM_BL
 
   . = ALIGN(4);
    _tcm_data_lma_start = LOADADDR(.tcm_data);
    .tcm_data :
    {
        _tcm_data_start = .;
        *(.tcm_rwdata)
        . = ALIGN(4);
        _tcm_data_end = .;
    } > VSYSRAM AT> ROM_BL

    .tcm_bss (NOLOAD) :
    {
        . = ALIGN(4);
        _tcm_bss_start = .;
        KEEP(*(.tcmBSS))
        KEEP(*(.bssRandom))
        *(.tcm_zidata)
        . = ALIGN(4);
        _tcm_bss_end = .;
    } > VSYSRAM
}
