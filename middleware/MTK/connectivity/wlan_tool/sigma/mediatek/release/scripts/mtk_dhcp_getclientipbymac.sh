#!/system/bin/sh

#if [ "$1" == "" ]; then
#	exit
#fi

CLIENT_MAC=$1

DHCP_LEASES_FILE="/data/misc/dhcp/dnsmasq.leases"

if [ "$CLIENT_MAC" != "" ]; then

ip_addr=""
[ -f $DHCP_LEASES_FILE ] && \
ip_addr=`cat $DHCP_LEASES_FILE | grep $CLIENT_MAC | awk '{print $3}'`

echo $ip_addr

else
# get first client mac ip
mac_addr=""
[ -f $DHCP_LEASES_FILE ] && \
mac_addr=`cat $DHCP_LEASES_FILE | awk 'NR==1{print $2" "$3}'`

echo $mac_addr
fi

