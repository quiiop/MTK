@echo off
adb shell "%BIN_DIR%/iwpriv wlan0 driver 'set_chip sigma D'"
adb shell "echo 0xff:0x2f > /proc/net/wlan/dbgLevel"
