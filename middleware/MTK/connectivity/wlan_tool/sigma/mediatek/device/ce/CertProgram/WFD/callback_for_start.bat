:: Disable AGING_TIMEOUT mechanism
adb shell "/data/bin/iwpriv wlan0 driver 'SET_SW_CTRL 0x10010004 1'"

echo CTRL_IFACE_DIR=%CTRL_IFACE_DIR%
:: set wfd_devType=1, wfd_sessionAvail=1, wfd_maxThroughput=300, wfd_rtspPort=554
adb shell "wpa_cli -i p2p0 -p%CTRL_IFACE_DIR% -s%CTRL_IFACE_DIR% wfd_subelem_set 0 00060011022a012c"
adb shell "wpa_cli -i p2p0 -p%CTRL_IFACE_DIR% -s%CTRL_IFACE_DIR% set device_name MTK-DTV-Sink"

:: Enable driver to append WFD IE to the association response
adb shell "/data/bin/iwpriv wlan0 driver 'miracast 2'"

:: Enable WiFi Display
adb shell "wpa_cli -i p2p0 -p%CTRL_IFACE_DIR% -s%CTRL_IFACE_DIR% set wifi_display 1"

:: Notify Miracast APP we are going to do WFD certification i.e. listen rtp/rtsp on p2p0
adb shell "setprop Miracast_cert running"

:: Start WFD cert APP to make background transparent
adb shell "am start -n com.mediatek.wificert/.BoxActivity"
:: Start Miracast APP
adb shell "am start -n com.mediatek.androidbox/.BoxActivity"

:: need to start twice to make sure tcp connecting, no idea why need to do so.
:: Start WFD cert APP to make background transparent
adb shell "am start -n com.mediatek.wificert/.BoxActivity"
:: Start Miracast APP
adb shell "am start -n com.mediatek.androidbox/.BoxActivity"

:: Waiting for WFD Cert APP to be ready
timeout /t 5
