adb shell "setprop vendor.wifi.sigma.ip.addr %PCENDPOINT_IP_ADDRESS%"
adb shell "setprop vendor.wifi.sigma.ip.netmask %PCENDPOINT_IP_NETMASK%"

:: Set driver's log level to default
adb shell "echo 0xff:0x2f > /proc/net/wlan/dbgLevel"

:: Enlarge kernel's fragment queue size to 20MB
adb shell "echo 20971520 > /proc/sys/net/ipv4/ipfrag_high_thresh"

echo Adjust socket buffer window size to MAX 8MB
adb shell "echo 8388608 > /proc/sys/net/core/wmem_default"
adb shell "cat /proc/sys/net/core/wmem_default"