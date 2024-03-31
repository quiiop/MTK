call:%~1
goto end

:wifi_start

set prefix=wpa
call :COPY_CONF
set prefix=p2p
call :COPY_CONF

call :ENABLE_WIFI
ping 127.0.0.1 -n 2 > nul
adb shell ifconfig wlan0 up
ping 127.0.0.1 -n 2 > nul
call :START_WPA
goto :eof


:COPY_CONF
adb shell "rm %BIN_DIR%/%prefix%_supplicant.conf"
if exist %PROGRAM_DIR%/%prefix%_supplicant.conf (
echo Copy %prefix%_supplicant.conf from %PROGRAM_DIR%
adb push %PROGRAM_DIR%/%prefix%_supplicant.conf %BIN_DIR%/
) else (
echo Copy %prefix%_supplicant.conf from %WPA_CONF_PATH%
adb shell "cat %WPA_CONF_PATH%/wpa_supplicant.conf >> %BIN_DIR%/%prefix%_supplicant.conf"
adb shell "cat %WPA_CONF_PATH%/%prefix%_supplicant_overlay.conf >> %BIN_DIR%/%prefix%_supplicant.conf"
)
goto :eof

:wifi_stop
echo wifi stop
call :STOP_WPA
ping 127.0.0.1 -n 2 > nul
adb shell ifconfig wlan0 down
ping 127.0.0.1 -n 2 > nul
call :DISABLE_WIFI
ping 127.0.0.1 -n 2 > nul
goto :eof

:START_WPA
echo WPA_SUPPLICANT_BIN=%WPA_SUPPLICANT_BIN%
echo CTRL_IFACE_DIR=%CTRL_IFACE_DIR%
adb shell "%WPA_SUPPLICANT_BIN% -g %CTRL_IFACE_DIR%/wpa_wlan0 -O %CTRL_IFACE_DIR% -ddd -iwlan0 -Dnl80211 -c %BIN_DIR%/wpa_supplicant.conf -N -ip2p0 -Dnl80211 -c %BIN_DIR%/p2p_supplicant.conf -B"
goto :eof

:STOP_WPA
adb shell "killall wpa_supplicant"
goto :eof

:ENABLE_WIFI
if "%DEVICE%" == "mobile" (adb shell "echo S > /dev/wmtWifi")
if "%DEVICE%" == "ce" (adb shell insmod %DRIVER_KO%)
:: don't change driver status because it should be only set by android framework
::adb shell setprop wlan.driver.status "ok"
goto :eof

:DISABLE_WIFI
adb shell svc wifi disable > nul
if "%DEVICE%" == "mobile" (adb shell "echo 0 > /dev/wmtWifi")
if "%DEVICE%" == "ce" (adb shell rmmod %DRIVER_NAME%)
:: don't change driver status because it should be only set by android framework
::adb shell setprop wlan.driver.status "unloaded"
goto :eof


:enable_device_log
echo Start to enable %device% log..

if "%device%" == "mobile" (
adb shell uname -r|findstr 3.18>nul || goto SUPPRESS_KERNEL_44_LOG_TOOMUCH
echo Disable kernel log too much for 3.18
adb shell "echo 3 > /proc/mtprintk"
goto ENABLE_DRIVER_LOG

:SUPPRESS_KERNEL_44_LOG_TOOMUCH
echo Disable kernel log too much for 4.4
adb shell "echo 2 > /proc/mtprintk"

)

if "%device%" == "ce" (
echo ce kernel log:
adb shell "cat /proc/sys/kernel/printk"
)

:ENABLE_DRIVER_LOG
echo Enable driver debug log
adb shell "echo 0xff:0x7f > /proc/net/wlan/dbgLevel"

echo Finish enabling %device% log.
goto :eof

:end
