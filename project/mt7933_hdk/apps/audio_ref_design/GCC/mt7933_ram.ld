OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)

/* Memory Spaces Definitions: 1M SYSRAM */
MEMORY
{
    ROM_BL(rx)        : ORIGIN = 0x90000000, LENGTH = 64K
    ROM_RBL(rx)       : ORIGIN = 0x90010000, LENGTH = 8K
    ROM_TFM(rx)       : ORIGIN = 0x90012000, LENGTH = 200K
    ROM_RTOS(rx)      : ORIGIN = 0x90044000, LENGTH = 2616K
    ROM_WIFI_PATCH(rx): ORIGIN = 0x902D2000, LENGTH = 56K
    ROM_WIFI_EXT(rx)  : ORIGIN = 0x902E0000, LENGTH = 224K
    ROM_WIFI(rx)      : ORIGIN = 0x90318000, LENGTH = 192K
    ROM_BT(rx)        : ORIGIN = 0x90348000, LENGTH = 850K
    ROM_DSP(rx)       : ORIGIN = 0x9041C800, LENGTH = 2556K
    ROM_NVDM(rx)      : ORIGIN = 0x907F0000, LENGTH = 64K

    TCM(rwx)          : ORIGIN = 0x0010C000, LENGTH = 20K
    RAM(rwx)          : ORIGIN = 0xA0000000, LENGTH = 4096K
    SYSRAM(rwx)       : ORIGIN = 0x80000000, LENGTH = 992K /* FPGA: 256K Real: 1024K */
    /*
    Reserved          : LENGTH = 32K
    */

    VROM(rx)          : ORIGIN = 0x18000000, LENGTH = 4096K
    VRAM(rwx)         : ORIGIN = 0x10000000, LENGTH = 4096K
    VSYSRAM(rwx)      : ORIGIN = 0x08000000, LENGTH = 992K /* FPGA: 256K Real: 1024K */
    /*
    Reserved          : LENGTH = 32K
    */
}

_rom_bl_start = ORIGIN(ROM_BL);
_rom_bl_length = LENGTH(ROM_BL);
_rom_rbl_start = ORIGIN(ROM_RBL);
_rom_rbl_length = LENGTH(ROM_RBL);
_rom_tfm_start = ORIGIN(ROM_TFM);
_rom_tfm_length = LENGTH(ROM_TFM);
_rom_rtos_start = ORIGIN(ROM_RTOS);
_rom_rtos_length = LENGTH(ROM_RTOS);
_rom_wifi_patch_start = ORIGIN(ROM_WIFI_PATCH);
_rom_wifi_patch_length = LENGTH(ROM_WIFI_PATCH);
_rom_wifi_start = ORIGIN(ROM_WIFI);
_rom_wifi_length = LENGTH(ROM_WIFI);
_rom_wifi_ext_start = ORIGIN(ROM_WIFI_EXT);
_rom_wifi_ext_length = LENGTH(ROM_WIFI_EXT);
_rom_bt_start = ORIGIN(ROM_BT);
_rom_bt_length = LENGTH(ROM_BT);
_rom_dsp_start = ORIGIN(ROM_DSP);
_rom_dsp_length = LENGTH(ROM_DSP);
_rom_nvdm_start = ORIGIN(ROM_NVDM);
_rom_nvdm_length = LENGTH(ROM_NVDM);

_sysram_start   = ORIGIN(SYSRAM);
_sysram_length  = LENGTH(SYSRAM);
_ram_start      = ORIGIN(RAM);
_ram_length     = LENGTH(RAM);
_vram_start     = ORIGIN(VRAM);
_vram_length    = LENGTH(VRAM);
_vsysram_start  = ORIGIN(VSYSRAM);
_vsysram_length = LENGTH(VSYSRAM);
_tcm_start      = ORIGIN(TCM);
_tcm_length     = LENGTH(TCM);

 /* Highest address of the stack */
_stack_end = ORIGIN(TCM) + LENGTH(TCM);    /* end of TCM */

/* Generate a link error if stack don't fit into TCM */
_stack_size = 0x1000; /* required amount of stack 4KB*/

_region0_offset = 0x13000 ;

/* stack start */
_stack_start = _stack_end - _stack_size;

/* Entry Point */
ENTRY(Reset_Handler)

SECTIONS
{
        . = ORIGIN(SYSRAM);
        . = . + _region0_offset;
        _text_load = LOADADDR(.text);
        _noncached_sysram_code_load = LOADADDR(.text);
        .text . : ALIGN(4)
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
            _sysram_code_load = .;
            _sysram_code_start = .;
            *(.sysram_code)
            *(.sysram_rodata)
            . = ALIGN(4);
            _sysram_code_end = .;
            _noncached_sysram_code_start = .;
            *(.noncached_sysram_code)
            *(.noncached_sysram_rodata)
            . = ALIGN(4) ;
            _noncached_sysram_code_end = .;
        } > SYSRAM

        .zero.table :
        {
            . = ALIGN(4);
            __zero_table_start__ = .;
            LONG( ADDR(.bss));
            LONG( SIZEOF(.bss));
            __zero_table_end__ = .;
        } > SYSRAM

        .ARM.extab :
        {
            *(.ARM.extab* .gnu.linkonce.armextab.*)
        } > SYSRAM

        __exidx_start = .;
        .ARM.exidx :
        {
            *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        } > SYSRAM
        __exidx_end = .;
        __etext = ALIGN (4);

        . = ALIGN(32);
        _sysram_data_load = LOADADDR(.data);
        _noncached_sysram_data_load = LOADADDR(.data);
        .data  . :
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
            _noncached_sysram_data_start = .;
            *(.noncached_sysram_rwdata)
             . = ALIGN(4);
            _noncached_sysram_data_end = .;
        } > SYSRAM

        . = ALIGN(4);
        .bss :
        {
            _sysram_bss_start = .;
            *(.bss)
            *(.bss.*)
            *(COMMON)
            *(.sysram_zidata)
            *(.sysram_swla_zidata)
            . = ALIGN(4);
            _sysram_bss_end = .;
            _noncached_sysram_bss_start = .;
            *(.noncached_sysram_zidata)
            . = ALIGN(4);
            _noncached_sysram_bss_end = .;
        } > SYSRAM

        . = ALIGN(32);
        _ram_code_load = LOADADDR(.cached_ram_text);
        .cached_ram_text :
        {
            _ram_code_start = .;
            *(.ram_code)
            *(.ram_rodata)
            . = ALIGN(4);
            _ram_code_end = .;
        } > VRAM  AT> SYSRAM

        . = ALIGN(4);
        _ram_data_load = LOADADDR(.cached_ram_data);
        .cached_ram_data :
        {
            _ram_data_start = .;
            *(.ram_rwdata)
            . = ALIGN(4);
            _ram_data_end = .;
        } > VRAM  AT> SYSRAM

        . = ALIGN(4);
        _ram_bss_load = LOADADDR(.cached_ram_bss);
        .cached_ram_bss :
        {
            _ram_bss_start = .;
            *(.ram_zidata)
            . = ALIGN(4);
            _ram_bss_end = .;
        } > VRAM  AT> SYSRAM


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
        } > RAM  AT> SYSRAM

        . = ALIGN(4);
        _noncached_ram_data_load = LOADADDR(.noncached_ram_data);
        .noncached_ram_data . :
        {
            _noncached_ram_data_start = .;
            *(.noncached_ram_rwdata)
            . = ALIGN(4);
            _noncached_ram_data_end = .;
        } > RAM  AT> SYSRAM

        . = ALIGN(4);
        _noncached_ram_bss_load = LOADADDR(.noncached_ram_bss);
        .noncached_ram_bss :
        {
            _noncached_ram_bss_start = .;
            *(.noncached_ram_zidata)
            . = ALIGN(4);
            _noncached_ram_bss_end = .;
        } > RAM  AT> SYSRAM

       . = ALIGN(4);
        _tcm_code_load = LOADADDR(.tcm_text);
        .tcm_text :
        {
            _tcm_code_start = .;
            *(.tcm_code)
            *(.tcm_rodata)
            . = ALIGN(4);
            _tcm_code_end = .;
        }> TCM  AT> SYSRAM

       . = ALIGN(4);
        _tcm_data_load = LOADADDR(.tcm_data);
        .tcm_data :
        {
            _tcm_data_start = .;
            *(.secure_settings_section)
            . = ALIGN(4);
            *(.tcm_rwdata)
            . = ALIGN(4);
            _tcm_data_end = .;
        }> TCM  AT> SYSRAM

        . = ALIGN(4);
        .tcm_bss :
        {
            _tcm_bss_start = .;
            *(.tcm_zidata)
            . = ALIGN(4);
            *(.tcm_wakeup_info)
            . = ALIGN(4);
            _tcm_bss_end = .;
        }> TCM AT> SYSRAM


        __end__ = .;

        /* use to check if the stack exceeds the total TCM size*/
        .stack (ORIGIN(TCM) + LENGTH(TCM) - _stack_size) (COPY) :
        {
            . = ALIGN(4);
            __StackLimit = .;
            PROVIDE ( end = . );
            PROVIDE ( _end = . );
            . = . + _stack_size;
            . = ALIGN(4);
            __StackTop = .;
        } > TCM

        PROVIDE(__stack = __StackTop);
        ASSERT(__StackLimit >= __end__, "region SYSRAM overflowed with stack")
}

