/* Memory Spaces Definitions: 16MB flash  */
MEMORY
{
    ROM_BL(rx)        : ORIGIN = 0x18000000, LENGTH = 64K
    ROM_RBL(rx)       : ORIGIN = 0x18010000, LENGTH = 8K
    ROM_TFM(rx)       : ORIGIN = 0x18012000, LENGTH = 200K
    ROM_RTOS(rx)      : ORIGIN = 0x18044080, LENGTH = 3072K - 0x80
    ROM_FOTA(r)       : ORIGIN = 0x18344000, LENGTH = 8424K
    /*
    Reserved For TFM  : LENGTH = 152 K
    Free For Customer : LENGTH = 500 K
    */
    ROM_NVDM(rx)      : ORIGIN = 0x18C21000, LENGTH = 64K
    ROM_LOG(rx)       : ORIGIN = 0x18C31000, LENGTH = 64K
    ROM_DSP(rx)       : ORIGIN = 0x18C41000, LENGTH = 2556K
    ROM_BT(rx)        : ORIGIN = 0x18705000, LENGTH = 304K
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
    SYSRAM(rwx)       : ORIGIN = 0x80000000, LENGTH = 992K
    /*
    Reserved          : LENGTH = 32K
    */

    VROM(rx)          : ORIGIN = 0x18000000, LENGTH = 4096K
    VRAM_BTFW(rwx)    : ORIGIN = 0x10000000, LENGTH = 352K
    VRAM_TFM(rwx)     : ORIGIN = 0x10380000, LENGTH = 512K
    VRAM_WFFW(rwx)    : ORIGIN = 0x10400000, LENGTH = 312K
    VRAM(rwx)         : ORIGIN = 0x1044E000, LENGTH = 3784K
    VSYSRAM(rwx)      : ORIGIN = 0x08000000, LENGTH = 992K
    /*
    Reserved          : LENGTH = 32K
    */
}
