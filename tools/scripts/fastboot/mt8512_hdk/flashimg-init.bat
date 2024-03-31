echo ====== DO NOT RELEASE DOWNLOAD KEY UNTIL FOLLOWING STEP SAY IT! ======

python fbtool.py
::Wait for reboot finish
ping 123.45.67.89 -n 1 -w 50 > nul

fastboot erase nand0

echo ====== YOU CAN RELEASE DOWNLOAD KEY NOW. ======

fastboot flash nand0 MBR_NAND
fastboot flash bl2 bl2.img
fastboot flash tee_a tee.img
fastboot flash boot_a boot.img
fastboot flash system_a system.ubi
fastboot flash userdata userdata.ubi

fastboot reboot

echo Finish downloading images!
pause