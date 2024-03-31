#!/system/bin/sh
echo $0" reset DHCP"

#DHCP_SERVER_APP_PATH=`cat mtk_ini.ini | grep DHCP_SERVER_APP_PATH | cut -f2 -d'='`
DHCP_SERVER_APP_PATH="ndc"
DHCP_SERVER_NDC_LEASES_FILE="/data/misc/dhcp/dnsmasq.leases"
if [[ "$DHCP_SERVER_APP_PATH" == *"busybox udhcpd"* ]]; then
	killall busybox
elif [[ "$DHCP_SERVER_APP_PATH" == *"udhcpd"* ]]; then
	killall udhcpd
elif [[ "$DHCP_SERVER_APP_PATH" == *"ndc"* ]]; then
	ndc tether stop
	[ -f $DHCP_SERVER_NDC_LEASES_FILE ] && rm -rf $DHCP_SERVER_NDC_LEASES_FILE
elif [[ "$DHCP_SERVER_APP_PATH" == *"dnsmasq"* ]]; then
	killall dnsmasq
else
	echo "need reset cmd in mtk_dhcp_reset.sh !"
fi
#DHCP_CLIENT_APP_PATH=`cat mtk_ini.ini | grep DHCP_CLIENT_APP_PATH | cut -f2 -d'='`
DHCP_CLIENT_APP_PATH="dhcpcd"
DHCP_CLIENT_LOG="/data/local/dhcpcd_log.txt"
if [[ "$DHCP_CLIENT_APP_PATH" == *"busybox udhcpc"* ]]; then
	killall busybox
elif [[ "$DHCP_CLIENT_APP_PATH" == *"udhcpc"* ]]; then
	killall udhcpc
elif [[ "$DHCP_CLIENT_APP_PATH" == *"dhcpcd"* ]]; then
	killall dhcpcd
	rm -rf $DHCP_CLIENT_LOG
else
	echo "need reset cmd in mtk_dhcp_reset.sh !"
fi
