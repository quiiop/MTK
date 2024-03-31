
###################################################
# Sources
BC_SRC = middleware/third_party/barcode_scanner

C_FILES  += $(BC_SRC)/bc_cli.c

C_FILES  += $(BC_SRC)/MT84G_UART/mt84g_uart_cli.c
C_FILES  += $(BC_SRC)/MT84G_HID/mt84g_hid_cli.c

#################################################################################
# include path

CFLAGS  += -I$(SOURCE_DIR)/middleware/third_party/barcode_scanner
CFLAGS  += -I$(SOURCE_DIR)/middleware/third_party/barcode_scanner/MT84G_UART
CFLAGS  += -I$(SOURCE_DIR)/middleware/third_party/barcode_scanner/MT84G_HID
