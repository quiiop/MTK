set LOCAL_DIR=%~dp0

echo "Turn on voe test mode if needed..."

adb push %LOCAL_DIR%\wifi_fw.cfg /data/misc/wifi
adb push %LOCAL_DIR%\wifi.cfg /data/misc/wifi
