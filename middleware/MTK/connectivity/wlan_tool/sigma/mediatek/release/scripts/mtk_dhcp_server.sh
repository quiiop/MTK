#!/system/bin/sh

#DHCP_SERVER_APP_PATH=`cat mtk_ini.ini | grep DHCP_SERVER_APP_PATH | cut -f2 -d'='`
DHCP_SERVER_APP_PATH="ndc"
DHCP_SERVER_IP="192.168.49.1"
DHCPD_IP_POOL_S="192.168.49.100"
DHCPD_IP_POOL_E="192.168.49.200"
#DHCPD_LEASE_FILE_PATH=`cat mtk_ini.ini | grep DHCPD_LEASE_FILE_PATH | cut -f2 -d'='`
#DHCPD_CONFIG_FILE_PATH=`cat mtk_ini.ini | grep DHCPD_CONFIG_FILE_PATH | cut -f2 -d'='`

echo "dhcp server path: "$DHCP_SERVER_APP_PATH" interface: "$1

if [[ "$DHCP_SERVER_APP_PATH" == *"udhcpd"* ]]; then
	echo "interface $1" > $DHCPD_CONFIG_FILE_PATH.tmp
	cat $DHCPD_CONFIG_FILE_PATH >> $DHCPD_CONFIG_FILE_PATH.tmp
	touch $DHCPD_LEASE_FILE_PATH;sync;sync;echo \"\" > $DHCPD_LEASE_FILE_PATH; $DHCP_SERVER_APP_PATH -fS $DHCPD_CONFIG_FILE_PATH.tmp &
elif [[ "$DHCP_SERVER_APP_PATH" == *"dnsmasq"* ]]; then
	$DHCP_SERVER_APP_PATH --no-resolv --no-poll --pid-file --dhcp-range=$1,$1,1h
elif [[ "$DHCP_SERVER_APP_PATH" == *"dhcpd"* ]]; then
	touch $DHCPD_LEASE_FILE_PATH;sync;sync;echo \"\" > $DHCPD_LEASE_FILE_PATH; $DHCP_SERVER_APP_PATH -cf $DHCPD_CONFIG_FILE_PATH -lf $DHCPD_LEASE_FILE_PATH $1 &
elif [[ "$DHCP_SERVER_APP_PATH" == *"ndc"* ]]; then
	ifconfig $1 $DHCP_SERVER_IP up
	ip rule add pref 9999 from all fwmark 0x0/0xffff table main
	$DHCP_SERVER_APP_PATH tether start $DHCPD_IP_POOL_S $DHCPD_IP_POOL_E
else
	echo $DHCP_SERVER_APP_PATH need to add cmd!!!!
fi
