set LOCAL_DIR=%~dp0

echo "Push wfd wifi.cfg"

adb shell chmod 777 /data/misc/wifi/

if exist %LOCAL_DIR%\wifi.cfg (
    adb shell getprop | findstr ro.vendor.wlan.gen | findstr gen4m > nul && adb push %LOCAL_DIR%wifi.cfg data/misc/wifi/
)

adb shell "pm list package | grep com.mediatek.wificert" > nul || (echo "install WifiCertTest.apk" && adb install %LOCAL_DIR%WifiCertTest.apk)

