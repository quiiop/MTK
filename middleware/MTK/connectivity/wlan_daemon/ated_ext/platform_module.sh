#!/bin/sh
#

dram_fun()
{
	echo "DRAM function working"
	cpfile="/tmp/testcopy"
	while true; do
		for index in 0 1 2 3 4 5 6 7 8 9 10
		do
			if [ -e "/dev/ram"$index ]; then
				cp "/dev/ram"$index ${cpfile}
			fi
		done
	done
}

usb3_fun()
{
	echo "USB3 function workingt"
	if [ -f "/media/sda/usb_test" ]; then
		usbfile="/media/sda/usb_test"	
	elif [ -f "/media/sda1/usb_test" ]; then
		usbfile="/media/sda1/usb_test"	
	else
		echo "USB copy file not found!!"
		exit 0
	fi
	while true; do
	    cp ${usbfile} ${usbfile}"_copy"
	done
}

usb2_fun()
{
	echo "USB2 function working"
	if [ -f "/media/sda/usb_test" ]; then
		usbfile="/media/sda/usb_test"	
	elif [ -f "/media/sda1/usb_test" ]; then
		usbfile="/media/sda1/usb_test"	
	else
		echo "USB copy file not found!!"
		exit 0
	fi
	while true; do
	    cp ${usbfile} ${usbfile}"_copy"
	done

}

sata_fun()
{
	echo "SATA function working"
	if [ -f "/media/sda/sata_test" ]; then
		satafile="/media/sda/sata_test"	
	elif [ -f "/media/sda1/sata_test" ]; then
		satafile="/media/sda1/sata_test"	
	else
		echo "SATA copy file not found!!"
		exit 0
	fi
	while true; do
	    cp ${satafile} ${satafile}"_copy"
	done

}

sdxc_fun()
{
	echo "SDXC function working"
	if [ -f "/media/mmcblk/sdxc_test" ]; then
		sdxcfile="/media/mmcblk/sdxc_test"	
	elif [ -f "/media/mmcblk0/sdxc_test" ]; then
		sdxcfile="/media/mmcblk0/sdxc_test"	
	elif [ -f "/media/mmcblk1/sdxc_test" ]; then
		sdxcfile="/media/mmcblk1/sdxc_test"	
	else
		echo "SDXC copy file not found!!"
		exit 0
	fi
	while true; do
	    cp ${sdxcfile} ${sdxcfile}"_copy"
	done
}

nand_fun()
{
	echo "NAND de-sense test"
	cpfile="/tmp/testcopy"
	while true; do
		for index in 0 1 2 3 4 5 6 7 8 9 10
		do
			if [ -e "/dev/mtdblock"$index ]; then
				cp "/dev/mtdblock"$index ${cpfile}
			fi
		done
	done
}

pcie_fun()
{
	echo "PCIe function working"
	ifconfig eth0 up
	brctl addif br0 eth0
	sleep 5
	iperf_client1
}

ephy_fun()
{
	echo "EPHY function working"
	iperf_client1
}

rgmii_fun()
{
	echo "RGMII function workinga"
	iperf_client1
}

sgmii_fun()
{
	echo "SGMII function workingt"
	iperf_client1
}

iperf_client1()
{
	iperf -c 10.10.10.10 -d -t 99999 &
}

iperf_client2()
{
	iperf -c 10.10.10.20 -d -t 99999 &
}

all_fun()
{
	echo "All functions workingt"
	usb3_fun
	usb2_fun
	sata_fun
	sdxc_fun
	nand_fun
	iperf_client1
	iperf_client2
}

stop_fun()
{
	echo "stop all functions"
	killall -SIGKILL iperf
	killall -SIGKILL cp
	killall -SIGKILL fun_busy.sh
}

main_function()
{
# free cache
echo 3 > /proc/sys/vm/drop_caches

if [ "$function" == "DRAM" ]; then
	dram_fun
elif [ "$function" == "USB3" ]; then
	usb3_fun
elif [ "$function" == "USB2" ]; then
	usb2_fun
elif [ "$function" == "SATA" ]; then
	sata_fun
elif [ "$function" == "SDXC" ]; then
	sdxc_fun
elif [ "$function" == "NAND" ]; then
	nand_fun
elif [ "$function" == "PCIE" ]; then
	pcie_fun
elif [ "$function" == "EPHY" ]; then
	ephy_fun
elif [ "$function" == "RGMII" ]; then
	rgmii_fun
elif [ "$function" == "SGMII" ]; then
	sgmii_fun
elif [ "$function" == "ALL" ]; then
	all_fun
elif [ "$function" == "STOP" ]; then
	stop_fun
else
	echo "Interface "$function" not supported!!"
fi
}

function=$1
main_function

