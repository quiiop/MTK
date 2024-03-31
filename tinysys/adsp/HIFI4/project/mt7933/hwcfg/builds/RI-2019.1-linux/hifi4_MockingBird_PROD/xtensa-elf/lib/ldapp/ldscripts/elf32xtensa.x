/* This linker script generated from xt-genldscripts.tpp for LSP ldapp */
/* Linker Script for default link */
MEMORY
{
  cached0_seg :                       	org = 0x40000000, len = 0x20000000
  noncached0_seg :                    	org = 0x60000000, len = 0x20000000
  sram0_seg :                         	org = 0xA0000000, len = 0x640
  sram1_seg :                         	org = 0xA0000640, len = 0x2E0
  sram3_seg :                         	org = 0xA0000C00, len = 0x178
  sram4_seg :                         	org = 0xA0000D78, len = 0x4
  sram5_seg :                         	org = 0xA0000D7C, len = 0x1C
  sram6_seg :                         	org = 0xA0000D98, len = 0x4
  sram7_seg :                         	org = 0xA0000D9C, len = 0x1C
  sram8_seg :                         	org = 0xA0000DB8, len = 0x4
  sram9_seg :                         	org = 0xA0000DBC, len = 0x1C
  sram10_seg :                        	org = 0xA0000DD8, len = 0x4
  sram11_seg :                        	org = 0xA0000DDC, len = 0x1C
  sram12_seg :                        	org = 0xA0000DF8, len = 0x4
  sram13_seg :                        	org = 0xA0000DFC, len = 0x1C
  sram14_seg :                        	org = 0xA0000E18, len = 0x4
  sram15_seg :                        	org = 0xA0000E1C, len = 0x1C
  sram16_seg :                        	org = 0xA0000E38, len = 0xFFFF1C8
}

PHDRS
{
  cached0_phdr PT_LOAD;
  noncached0_phdr PT_LOAD;
  sram0_phdr PT_LOAD;
  sram1_phdr PT_LOAD;
  sram2_phdr PT_LOAD;
  sram3_phdr PT_LOAD;
  sram4_phdr PT_LOAD;
  sram5_phdr PT_LOAD;
  sram6_phdr PT_LOAD;
  sram7_phdr PT_LOAD;
  sram8_phdr PT_LOAD;
  sram9_phdr PT_LOAD;
  sram10_phdr PT_LOAD;
  sram11_phdr PT_LOAD;
  sram12_phdr PT_LOAD;
  sram13_phdr PT_LOAD;
  sram14_phdr PT_LOAD;
  sram15_phdr PT_LOAD;
  sram16_phdr PT_LOAD;
  sram16_bss_phdr PT_LOAD;
}


/*  Default entry point:  */
ENTRY(_ResetVector)


/*  Memory boundary addresses:  */
_memmap_mem_cached_start = 0x40000000;
_memmap_mem_cached_end   = 0x60000000;
_memmap_mem_noncached_start = 0x60000000;
_memmap_mem_noncached_end   = 0x80000000;
_memmap_mem_sram_start = 0xa0000000;
_memmap_mem_sram_end   = 0xb0000000;

/*  Memory segment boundary addresses:  */
_memmap_seg_cached0_start = 0x40000000;
_memmap_seg_cached0_max   = 0x60000000;
_memmap_seg_noncached0_start = 0x60000000;
_memmap_seg_noncached0_max   = 0x80000000;
_memmap_seg_sram0_start = 0xa0000000;
_memmap_seg_sram0_max   = 0xa0000640;
_memmap_seg_sram1_start = 0xa0000640;
_memmap_seg_sram1_max   = 0xa0000920;
_memmap_seg_sram3_start = 0xa0000c00;
_memmap_seg_sram3_max   = 0xa0000d78;
_memmap_seg_sram4_start = 0xa0000d78;
_memmap_seg_sram4_max   = 0xa0000d7c;
_memmap_seg_sram5_start = 0xa0000d7c;
_memmap_seg_sram5_max   = 0xa0000d98;
_memmap_seg_sram6_start = 0xa0000d98;
_memmap_seg_sram6_max   = 0xa0000d9c;
_memmap_seg_sram7_start = 0xa0000d9c;
_memmap_seg_sram7_max   = 0xa0000db8;
_memmap_seg_sram8_start = 0xa0000db8;
_memmap_seg_sram8_max   = 0xa0000dbc;
_memmap_seg_sram9_start = 0xa0000dbc;
_memmap_seg_sram9_max   = 0xa0000dd8;
_memmap_seg_sram10_start = 0xa0000dd8;
_memmap_seg_sram10_max   = 0xa0000ddc;
_memmap_seg_sram11_start = 0xa0000ddc;
_memmap_seg_sram11_max   = 0xa0000df8;
_memmap_seg_sram12_start = 0xa0000df8;
_memmap_seg_sram12_max   = 0xa0000dfc;
_memmap_seg_sram13_start = 0xa0000dfc;
_memmap_seg_sram13_max   = 0xa0000e18;
_memmap_seg_sram14_start = 0xa0000e18;
_memmap_seg_sram14_max   = 0xa0000e1c;
_memmap_seg_sram15_start = 0xa0000e1c;
_memmap_seg_sram15_max   = 0xa0000e38;
_memmap_seg_sram16_start = 0xa0000e38;
_memmap_seg_sram16_max   = 0xb0000000;

_rom_store_table = 0;
PROVIDE(_memmap_reset_vector = 0xa0000640);
PROVIDE(_memmap_vecbase_reset = 0xa0000c00);
/* Various memory-map dependent cache attribute settings: */
_memmap_cacheattr_wb_base = 0x00101100;
_memmap_cacheattr_wt_base = 0x00303300;
_memmap_cacheattr_bp_base = 0x00404400;
_memmap_cacheattr_unused_mask = 0xFF0F00FF;
_memmap_cacheattr_wb_trapnull = 0x44141140;
_memmap_cacheattr_wba_trapnull = 0x44141140;
_memmap_cacheattr_wbna_trapnull = 0x44242240;
_memmap_cacheattr_wt_trapnull = 0x44343340;
_memmap_cacheattr_bp_trapnull = 0x44444440;
_memmap_cacheattr_wb_strict = 0x00101100;
_memmap_cacheattr_wt_strict = 0x00303300;
_memmap_cacheattr_bp_strict = 0x00404400;
_memmap_cacheattr_wb_allvalid = 0x44141144;
_memmap_cacheattr_wt_allvalid = 0x44343344;
_memmap_cacheattr_bp_allvalid = 0x44444444;
_memmap_region_map = 0x0000002C;
PROVIDE(_memmap_cacheattr_reset = _memmap_cacheattr_wb_trapnull);

SECTIONS
{

  .cached.rodata : ALIGN(4)
  {
    _cached_rodata_start = ABSOLUTE(.);
    *(.cached.rodata)
    . = ALIGN (4);
    _cached_rodata_end = ABSOLUTE(.);
  } >cached0_seg :cached0_phdr

  .cached.text : ALIGN(4)
  {
    _cached_text_start = ABSOLUTE(.);
    *(.cached.literal .cached.text)
    . = ALIGN (4);
    _cached_text_end = ABSOLUTE(.);
  } >cached0_seg :cached0_phdr

  .cached.data : ALIGN(4)
  {
    _cached_data_start = ABSOLUTE(.);
    *(.cached.data)
    . = ALIGN (4);
    _cached_data_end = ABSOLUTE(.);
    _memmap_seg_cached0_end = ALIGN(0x8);
  } >cached0_seg :cached0_phdr


  .noncached.rodata : ALIGN(4)
  {
    _noncached_rodata_start = ABSOLUTE(.);
    *(.noncached.rodata)
    . = ALIGN (4);
    _noncached_rodata_end = ABSOLUTE(.);
  } >noncached0_seg :noncached0_phdr

  .noncached.text : ALIGN(4)
  {
    _noncached_text_start = ABSOLUTE(.);
    *(.noncached.literal .noncached.text)
    . = ALIGN (4);
    _noncached_text_end = ABSOLUTE(.);
  } >noncached0_seg :noncached0_phdr

  .noncached.data : ALIGN(4)
  {
    _noncached_data_start = ABSOLUTE(.);
    *(.noncached.data)
    . = ALIGN (4);
    _noncached_data_end = ABSOLUTE(.);
    _memmap_seg_noncached0_end = ALIGN(0x8);
  } >noncached0_seg :noncached0_phdr


  .ResetVector.literal : ALIGN(4)
  {
    _ResetVector_literal_start = ABSOLUTE(.);
    *(.ResetVector.literal)
    . = ALIGN (4);
    _ResetVector_literal_end = ABSOLUTE(.);
    _memmap_seg_sram0_end = ALIGN(0x8);
  } >sram0_seg :sram0_phdr


  .ResetVector.text : ALIGN(4)
  {
    _ResetVector_text_start = ABSOLUTE(.);
    KEEP (*(.ResetVector.text))
    . = ALIGN (4);
    _ResetVector_text_end = ABSOLUTE(.);
  } >sram1_seg :sram1_phdr

  .ResetHandler.text : ALIGN(4)
  {
    _ResetHandler_text_start = ABSOLUTE(.);
    *(.ResetHandler.literal .ResetHandler.text)
    . = ALIGN (4);
    _ResetHandler_text_end = ABSOLUTE(.);
    _memmap_seg_sram1_end = ALIGN(0x8);
  } >sram1_seg :sram1_phdr



  .WindowVectors.text : ALIGN(4)
  {
    _WindowVectors_text_start = ABSOLUTE(.);
    KEEP (*(.WindowVectors.text))
    . = ALIGN (4);
    _WindowVectors_text_end = ABSOLUTE(.);
    _memmap_seg_sram3_end = ALIGN(0x8);
  } >sram3_seg :sram3_phdr


  .Level2InterruptVector.literal : ALIGN(4)
  {
    _Level2InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level2InterruptVector.literal)
    . = ALIGN (4);
    _Level2InterruptVector_literal_end = ABSOLUTE(.);
    _memmap_seg_sram4_end = ALIGN(0x8);
  } >sram4_seg :sram4_phdr


  .Level2InterruptVector.text : ALIGN(4)
  {
    _Level2InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level2InterruptVector.text))
    . = ALIGN (4);
    _Level2InterruptVector_text_end = ABSOLUTE(.);
    _memmap_seg_sram5_end = ALIGN(0x8);
  } >sram5_seg :sram5_phdr


  .Level3InterruptVector.literal : ALIGN(4)
  {
    _Level3InterruptVector_literal_start = ABSOLUTE(.);
    *(.Level3InterruptVector.literal)
    . = ALIGN (4);
    _Level3InterruptVector_literal_end = ABSOLUTE(.);
    _memmap_seg_sram6_end = ALIGN(0x8);
  } >sram6_seg :sram6_phdr


  .Level3InterruptVector.text : ALIGN(4)
  {
    _Level3InterruptVector_text_start = ABSOLUTE(.);
    KEEP (*(.Level3InterruptVector.text))
    . = ALIGN (4);
    _Level3InterruptVector_text_end = ABSOLUTE(.);
    _memmap_seg_sram7_end = ALIGN(0x8);
  } >sram7_seg :sram7_phdr


  .DebugExceptionVector.literal : ALIGN(4)
  {
    _DebugExceptionVector_literal_start = ABSOLUTE(.);
    *(.DebugExceptionVector.literal)
    . = ALIGN (4);
    _DebugExceptionVector_literal_end = ABSOLUTE(.);
    _memmap_seg_sram8_end = ALIGN(0x8);
  } >sram8_seg :sram8_phdr


  .DebugExceptionVector.text : ALIGN(4)
  {
    _DebugExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.DebugExceptionVector.text))
    . = ALIGN (4);
    _DebugExceptionVector_text_end = ABSOLUTE(.);
    _memmap_seg_sram9_end = ALIGN(0x8);
  } >sram9_seg :sram9_phdr


  .KernelExceptionVector.literal : ALIGN(4)
  {
    _KernelExceptionVector_literal_start = ABSOLUTE(.);
    *(.KernelExceptionVector.literal)
    . = ALIGN (4);
    _KernelExceptionVector_literal_end = ABSOLUTE(.);
    _memmap_seg_sram10_end = ALIGN(0x8);
  } >sram10_seg :sram10_phdr


  .KernelExceptionVector.text : ALIGN(4)
  {
    _KernelExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.KernelExceptionVector.text))
    . = ALIGN (4);
    _KernelExceptionVector_text_end = ABSOLUTE(.);
    _memmap_seg_sram11_end = ALIGN(0x8);
  } >sram11_seg :sram11_phdr


  .UserExceptionVector.literal : ALIGN(4)
  {
    _UserExceptionVector_literal_start = ABSOLUTE(.);
    *(.UserExceptionVector.literal)
    . = ALIGN (4);
    _UserExceptionVector_literal_end = ABSOLUTE(.);
    _memmap_seg_sram12_end = ALIGN(0x8);
  } >sram12_seg :sram12_phdr


  .UserExceptionVector.text : ALIGN(4)
  {
    _UserExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.UserExceptionVector.text))
    . = ALIGN (4);
    _UserExceptionVector_text_end = ABSOLUTE(.);
    _memmap_seg_sram13_end = ALIGN(0x8);
  } >sram13_seg :sram13_phdr


  .DoubleExceptionVector.literal : ALIGN(4)
  {
    _DoubleExceptionVector_literal_start = ABSOLUTE(.);
    *(.DoubleExceptionVector.literal)
    . = ALIGN (4);
    _DoubleExceptionVector_literal_end = ABSOLUTE(.);
    _memmap_seg_sram14_end = ALIGN(0x8);
  } >sram14_seg :sram14_phdr


  .DoubleExceptionVector.text : ALIGN(4)
  {
    _DoubleExceptionVector_text_start = ABSOLUTE(.);
    KEEP (*(.DoubleExceptionVector.text))
    . = ALIGN (4);
    _DoubleExceptionVector_text_end = ABSOLUTE(.);
    _memmap_seg_sram15_end = ALIGN(0x8);
  } >sram15_seg :sram15_phdr


  .sram.rodata : ALIGN(4)
  {
    _sram_rodata_start = ABSOLUTE(.);
    *(.sram.rodata)
    . = ALIGN (4);
    _sram_rodata_end = ABSOLUTE(.);
  } >sram16_seg :sram16_phdr

  .clib.rodata : ALIGN(4)
  {
    _clib_rodata_start = ABSOLUTE(.);
    *(.clib.rodata)
    . = ALIGN (4);
    _clib_rodata_end = ABSOLUTE(.);
  } >sram16_seg :sram16_phdr

  .rtos.rodata : ALIGN(4)
  {
    _rtos_rodata_start = ABSOLUTE(.);
    *(.rtos.rodata)
    . = ALIGN (4);
    _rtos_rodata_end = ABSOLUTE(.);
  } >sram16_seg :sram16_phdr

  .rodata : ALIGN(4)
  {
    _rodata_start = ABSOLUTE(.);
    *(.rodata)
    *(SORT(.rodata.sort.*))
    KEEP (*(SORT(.rodata.keepsort.*) .rodata.keep.*))
    *(.rodata.*)
    *(.gnu.linkonce.r.*)
    *(.rodata1)
    __XT_EXCEPTION_TABLE__ = ABSOLUTE(.);
    KEEP (*(.xt_except_table))
    KEEP (*(.gcc_except_table))
    *(.gnu.linkonce.e.*)
    *(.gnu.version_r)
    KEEP (*(.eh_frame))
    /*  C++ constructor and destructor tables, properly ordered:  */
    KEEP (*crtbegin.o(.ctors))
    KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
    KEEP (*crtbegin.o(.dtors))
    KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))
    /*  C++ exception handlers table:  */
    __XT_EXCEPTION_DESCS__ = ABSOLUTE(.);
    *(.xt_except_desc)
    *(.gnu.linkonce.h.*)
    __XT_EXCEPTION_DESCS_END__ = ABSOLUTE(.);
    *(.xt_except_desc_end)
    *(.dynamic)
    *(.gnu.version_d)
    . = ALIGN(4);		/* this table MUST be 4-byte aligned */
    _bss_table_start = ABSOLUTE(.);
    LONG(_bss_start)
    LONG(_bss_end)
    _bss_table_end = ABSOLUTE(.);
    . = ALIGN (4);
    _rodata_end = ABSOLUTE(.);
  } >sram16_seg :sram16_phdr

  .sram.text : ALIGN(4)
  {
    _sram_text_start = ABSOLUTE(.);
    *(.sram.literal .sram.text)
    . = ALIGN (4);
    _sram_text_end = ABSOLUTE(.);
  } >sram16_seg :sram16_phdr

  .text : ALIGN(4)
  {
    _stext = .;
    _text_start = ABSOLUTE(.);
    *(.entry.text)
    *(.init.literal)
    KEEP(*(.init))
    *(.literal.sort.* SORT(.text.sort.*))
    KEEP (*(.literal.keepsort.* SORT(.text.keepsort.*) .literal.keep.* .text.keep.* .literal.*personality* .text.*personality*))
    *(.literal .text .literal.* .text.* .stub .gnu.warning .gnu.linkonce.literal.* .gnu.linkonce.t.*.literal .gnu.linkonce.t.*)
    *(.fini.literal)
    KEEP(*(.fini))
    *(.gnu.version)
    . = ALIGN (4);
    _text_end = ABSOLUTE(.);
    _etext = .;
  } >sram16_seg :sram16_phdr

  .clib.text : ALIGN(4)
  {
    _clib_text_start = ABSOLUTE(.);
    *(.clib.literal .clib.text)
    . = ALIGN (4);
    _clib_text_end = ABSOLUTE(.);
  } >sram16_seg :sram16_phdr

  .rtos.text : ALIGN(4)
  {
    _rtos_text_start = ABSOLUTE(.);
    *(.rtos.literal .rtos.text)
    . = ALIGN (4);
    _rtos_text_end = ABSOLUTE(.);
  } >sram16_seg :sram16_phdr

  .clib.data : ALIGN(4)
  {
    _clib_data_start = ABSOLUTE(.);
    *(.clib.data)
    . = ALIGN (4);
    _clib_data_end = ABSOLUTE(.);
  } >sram16_seg :sram16_phdr

  .clib.percpu.data : ALIGN(4)
  {
    _clib_percpu_data_start = ABSOLUTE(.);
    *(.clib.percpu.data)
    . = ALIGN (4);
    _clib_percpu_data_end = ABSOLUTE(.);
  } >sram16_seg :sram16_phdr

  .rtos.percpu.data : ALIGN(4)
  {
    _rtos_percpu_data_start = ABSOLUTE(.);
    *(.rtos.percpu.data)
    . = ALIGN (4);
    _rtos_percpu_data_end = ABSOLUTE(.);
  } >sram16_seg :sram16_phdr

  .rtos.data : ALIGN(4)
  {
    _rtos_data_start = ABSOLUTE(.);
    *(.rtos.data)
    . = ALIGN (4);
    _rtos_data_end = ABSOLUTE(.);
  } >sram16_seg :sram16_phdr

  .sram.data : ALIGN(4)
  {
    _sram_data_start = ABSOLUTE(.);
    *(.sram.data)
    . = ALIGN (4);
    _sram_data_end = ABSOLUTE(.);
  } >sram16_seg :sram16_phdr

  .data : ALIGN(4)
  {
    _data_start = ABSOLUTE(.);
    *(.data)
    *(SORT(.data.sort.*))
    KEEP (*(SORT(.data.keepsort.*) .data.keep.*))
    *(.data.*)
    *(.gnu.linkonce.d.*)
    KEEP(*(.gnu.linkonce.d.*personality*))
    *(.data1)
    *(.sdata)
    *(.sdata.*)
    *(.gnu.linkonce.s.*)
    *(.sdata2)
    *(.sdata2.*)
    *(.gnu.linkonce.s2.*)
    KEEP(*(.jcr))
    *(__llvm_prf_cnts)
    *(__llvm_prf_data)
    *(__llvm_prf_vnds)
    . = ALIGN (4);
    _data_end = ABSOLUTE(.);
  } >sram16_seg :sram16_phdr

  __llvm_prf_names : ALIGN(4)
  {
    __llvm_prf_names_start = ABSOLUTE(.);
    *(__llvm_prf_names)
    . = ALIGN (4);
    __llvm_prf_names_end = ABSOLUTE(.);
  } >sram16_seg :sram16_phdr

  .bss (NOLOAD) : ALIGN(8)
  {
    . = ALIGN (8);
    _bss_start = ABSOLUTE(.);
    *(.dynsbss)
    *(.sbss)
    *(.sbss.*)
    *(.gnu.linkonce.sb.*)
    *(.scommon)
    *(.sbss2)
    *(.sbss2.*)
    *(.gnu.linkonce.sb2.*)
    *(.dynbss)
    *(.bss)
    *(SORT(.bss.sort.*))
    KEEP (*(SORT(.bss.keepsort.*) .bss.keep.*))
    *(.bss.*)
    *(.gnu.linkonce.b.*)
    *(COMMON)
    *(.clib.bss)
    *(.clib.percpu.bss)
    *(.rtos.percpu.bss)
    *(.rtos.bss)
    *(.sram.bss)
    . = ALIGN (8);
    _bss_end = ABSOLUTE(.);
    _end = ALIGN(0x8);
    PROVIDE(end = ALIGN(0x8));
    _stack_sentry = ALIGN(0x8);
    _memmap_seg_sram16_end = ALIGN(0x8);
  } >sram16_seg :sram16_bss_phdr

  PROVIDE(__stack = 0xb0000000);
  _heap_sentry = 0xb0000000;
  .debug  0 :  { *(.debug) }
  .line  0 :  { *(.line) }
  .debug_srcinfo  0 :  { *(.debug_srcinfo) }
  .debug_sfnames  0 :  { *(.debug_sfnames) }
  .debug_aranges  0 :  { *(.debug_aranges) }
  .debug_pubnames  0 :  { *(.debug_pubnames) }
  .debug_info  0 :  { *(.debug_info) }
  .debug_abbrev  0 :  { *(.debug_abbrev) }
  .debug_line  0 :  { *(.debug_line) }
  .debug_frame  0 :  { *(.debug_frame) }
  .debug_str  0 :  { *(.debug_str) }
  .debug_loc  0 :  { *(.debug_loc) }
  .debug_macinfo  0 :  { *(.debug_macinfo) }
  .debug_weaknames  0 :  { *(.debug_weaknames) }
  .debug_funcnames  0 :  { *(.debug_funcnames) }
  .debug_typenames  0 :  { *(.debug_typenames) }
  .debug_varnames  0 :  { *(.debug_varnames) }
  .xt.insn 0 :
  {
    KEEP (*(.xt.insn))
    KEEP (*(.gnu.linkonce.x.*))
  }
  .xt.prop 0 :
  {
    KEEP (*(.xt.prop))
    KEEP (*(.xt.prop.*))
    KEEP (*(.gnu.linkonce.prop.*))
  }
  .xt.lit 0 :
  {
    KEEP (*(.xt.lit))
    KEEP (*(.xt.lit.*))
    KEEP (*(.gnu.linkonce.p.*))
  }
  .debug.xt.callgraph 0 :
  {
    KEEP (*(.debug.xt.callgraph .debug.xt.callgraph.* .gnu.linkonce.xt.callgraph.*))
  }
}

