@echo off
:preconfig

::adb shell /data/bin/iwpriv wlan0 set_sw_ctrl 0xa0400000 0x1
adb shell "%BIN_DIR%/iwpriv wlan0 driver 'set_chip sigma E'"
adb shell "%BIN_DIR%/iwpriv wlan0 driver 'SET_BA_SIZE 64'"
adb shell "echo 0xff:0x2f > /proc/net/wlan/dbgLevel"

echo Adjust socket buffer window size to MAX 8MB
if "%DEVICE%" == "ce" (
    adb shell "echo 8388608 > /proc/sys/net/core/wmem_max"
)
adb shell "echo 8388608 > /proc/sys/net/core/wmem_default"
adb shell "cat /proc/sys/net/core/wmem_default"

:preconfig_done
adb shell getprop | findstr ro.hardware | findstr 8695 > nul && goto 8695_performance_mode
adb shell getprop | findstr ro.vendor.wlan.gen | findstr gen4m > nul && goto connac_performance_mode
goto end

:connac_performance_mode
adb shell "svc power stayon true"
adb shell "echo 0 0 > /proc/ppm/policy/ut_fix_freq_idx"
adb shell "echo 100 > /proc/perfmgr/eas/debug_ta_boost"
goto end

:8695_performance_mode
adb shell "/vendor/bin/thermal_manager /vendor/etc/.tp/.ht120.mtc"
adb shell "echo 0 > /proc/hps/enabled"
adb shell "echo 1 > /sys/devices/system/cpu/cpu1/online"
adb shell "echo 1 > /sys/devices/system/cpu/cpu2/online"
adb shell "echo 1 > /sys/devices/system/cpu/cpu3/online"
adb shell "echo performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
adb shell "echo performance > /sys/devices/system/cpu/cpu1/cpufreq/scaling_governor"
adb shell "echo performance > /sys/devices/system/cpu/cpu2/cpufreq/scaling_governor"
adb shell "echo performance > /sys/devices/system/cpu/cpu3/cpufreq/scaling_governor"
goto end

:end

