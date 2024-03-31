/* Memory Spaces Definitions: 8MB flash  */
MEMORY
{
    ROM_BL(rx)        : ORIGIN = 0x18000000, LENGTH = 64K
    ROM_RBL(rx)       : ORIGIN = 0x18010000, LENGTH = 8K
    ROM_TFM(rx)       : ORIGIN = 0x18012000, LENGTH = 200K
    ROM_RTOS(rx)      : ORIGIN = 0x18044080, LENGTH = 2092K - 0x80
    /*
    Reserved          : LENGTH = 228 K, 152 K For TFM
    Free For Customer : LENGTH = 500 K
    */
    ROM_FOTA(r)       : ORIGIN = 0x18305000, LENGTH = 3968K
    ROM_NVDM(rx)      : ORIGIN = 0x186E5000, LENGTH = 64K
    ROM_LOG(rx)       : ORIGIN = 0x186F5000, LENGTH = 64K
    ROM_BT(rx)        : ORIGIN = 0x18705000, LENGTH = 304K
    /*
    Reserved For FW   : LENGTH = 40 K
    */
    ROM_WIFI_PWRTBL(rx): ORIGIN = 0x1875B000, LENGTH = 20K
    ROM_WIFI_EXT(rx)  : ORIGIN = 0x18760000, LENGTH = 636K
    ROM_BUFFER_BIN(rx): ORIGIN = 0x187FF000, LENGTH = 4K

    TCM(rwx)          : ORIGIN = 0x0010C000, LENGTH = 20K
    RAM_WFFW(rwx)     : ORIGIN = 0xA0000000, LENGTH = 312K
    RAM(rwx)          : ORIGIN = 0xA004E000, LENGTH = 3272K
    RAM_TFM(rwx)      : ORIGIN = 0xA0380000, LENGTH = 512K
    SYSRAM(rwx)       : ORIGIN = 0x80000000, LENGTH = 992K
    /*
    Reserved          : LENGTH = 32K
    */

    VROM(rx)          : ORIGIN = 0x18000000, LENGTH = 4096K
    VRAM_WFFW(rwx)    : ORIGIN = 0x10000000, LENGTH = 312K
    VRAM(rwx)         : ORIGIN = 0x1004E000, LENGTH = 3272K
    VRAM_TFM(rwx)     : ORIGIN = 0x10380000, LENGTH = 512K
    VSYSRAM(rwx)      : ORIGIN = 0x08000000, LENGTH = 992K
    /*
    Reserved          : LENGTH = 32K
    */
}
