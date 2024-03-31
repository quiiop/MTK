REM Since Android P removes WPS UI, we should try to provide a script to do wps certification

set wpa_cli="wpa_cli -p /data/vendor/misc/wifi/sockets -s /data/vendor/misc/wifi/sockets"

adb shell %wpa_cli% -iwlan0 set config_methods virtual_display virtual_push_button
adb shell %wpa_cli% -iwlan0 mac_rand_scan all enable=0
adb shell ifconfig wlan0 %DUT_IP_ADDRESS% netmask %DUT_IP_NETMASK%

:test_menu

for /f "tokens=2" %%i in ('tasklist /V ^| findstr WPS-DUT') do (taskkill /FI "pid eq %%i")
adb shell %wpa_cli% -iwlan0 remove_network all
adb shell "killall dhcpcd"

color 07
cls
@echo ##################################
@echo #                                #
@echo #          WPS STA Test          #
@echo #                                #
@echo ##################################

echo.

echo Please input the number you want to do :
echo  (1) Do WPS PBC test
echo  (2) Do WPS PIN test
echo  (3) Exit
@set /p test_choice=Your choice is :

echo.

if %test_choice%==1 (
	echo ============== WPS PBC Test =============
	adb shell %wpa_cli% -iwlan0 wps_pbc

	echo Now turn on PBC settings on AP and check connected or not.....
    echo.
	start "WPS-DUT" adb shell %wpa_cli% -iwlan0
	PAUSE
	adb shell "%BIN_DIR%/dhcpcd --noarp --nogateway wlan0"
	PAUSE
	goto test_menu
)

if %test_choice%==2 (
	echo ============== WPS PIN Test =============
	echo Please enter the pin on your ap router

	adb shell %wpa_cli% -iwlan0 wps_pin any
	echo.
	echo.
	start "WPS-DUT" adb shell %wpa_cli% -iwlan0
	PAUSE
	adb shell "%BIN_DIR%/dhcpcd --noarp --nogateway wlan0"
	PAUSE
	goto test_menu
)

if %test_choice%==3 (
	echo Bye bye~
	goto end
)

echo Bad choice, type carefully please.
PAUSE
goto test_menu

:end
exit
