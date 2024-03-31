#!/system/bin/sh

DHCP_CLIENT_LOG="/data/local/dhcpcd_log.txt"

ip_addr=""

if [ -f $DHCP_CLIENT_LOG ]; then
ip_addr=`cat $DHCP_CLIENT_LOG | grep acknowledged | awk '{print $5}'`
[ "$ip_addr" == "" ] && \
ip_addr=`cat $DHCP_CLIENT_LOG | grep offered | awk '{print $5}'`
fi

echo $ip_addr

