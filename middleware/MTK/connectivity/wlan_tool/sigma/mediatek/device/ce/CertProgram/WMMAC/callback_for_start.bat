@echo off

if "%DEVICE%" == "mobile" (
    adb shell /data/bin/iwpriv wlan0 set_mcr 2011 2011
    adb shell /data/bin/iwpriv wlan0 set_mcr 0x601100FC 0x01010101
)
