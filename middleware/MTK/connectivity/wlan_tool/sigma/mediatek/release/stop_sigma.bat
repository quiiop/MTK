@echo off

:: ca/dut config
for /F "tokens=1,2 delims==" %%i IN (scripts/config.txt) do set %%i=%%j
for /F "tokens=1,2 delims==" %%i IN (%PROGRAM_DIR%/config.txt) do set %%i=%%j

:: kill ca/dut/pf
for /f "tokens=2" %%i in ('tasklist /V ^| findstr Control-Agent') do (taskkill /FI "pid eq %%i")
for /f "tokens=2" %%i in ('tasklist /V ^| findstr WFA-DUT') do (taskkill /FI "pid eq %%i")
for /f "tokens=2" %%i in ('tasklist /V ^| findstr PFService') do (taskkill /FI "pid eq %%i")
adb shell "killall hostapd"
if "%~1"=="force" goto :stop
choice /C YN /M "press Y to turn off wlan, press N to leave wlan in sigma mode"
if ERRORLEVEL 2 goto :eof
:stop
adb shell ps -A|findstr wpa_supplicant>nul || echo wlan already off && goto :eof
rem adb shell ls /data/misc/wifi/sockets/wpa_ctrl* | findstr /C:"No such file">nul || echo wlan is in normal mode && goto :eof
call scripts\wifi_ctrl.bat wifi_stop
echo turning wifi off, wait 2 seconds
ping 127.0.0.1 -n 2 > nul
goto :stop