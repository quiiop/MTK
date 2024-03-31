#include "wifi_netif.h"
#include "gl_init.h"
#include "gl_os.h"
#include "gl_upperlayer.h"
#include "lwip/ip_addr.h"
#include "lwip/netifapi.h"
#if IP_NAPT
#include "lwip/ip4_napt_forward.h"
#endif /* #if IP_NAPT */
#include "netif/etharp.h"
#include <lwip/snmp.h>
#include <lwip/sockets.h>
#include <lwip/stats.h>
#include <lwip/tcpip.h>
#include "lwip/dhcp.h"
#include "lwip/dhcp6.h"
#include "lwip/ethip6.h"
#include "tcpip_wrapper.h"
#include "get_profile_string.h"
#include "wifi_api_ex.h"
#include "wifi_event_gen4m.h"

#ifdef MTK_NVDM_ENABLE
#include "nvdm.h"
#endif /* #ifdef MTK_NVDM_ENABLE */

#define QUERY_IP_READY_TIMEOUT 3000
static bool tcpip_enable;
struct netif sta_netif;
struct netif ap_netif;
static int g_lwip_eapol_rx_socket = -1;
static int g_lwip_eapol_rx_socket_second = -1;
uint8_t rMac[6];
uint8_t apMac[6];
uint8_t rIpMode = REQ_IP_MODE_DHCP;
static SemaphoreHandle_t ip_ready;
uint8_t opmode;

int lwip_get_netif_name(enum ENUM_NETIF_TYPE netif_type, char *name)
{
    struct netif *netif = NULL;

    if (name == NULL)
        return 0;

    for (netif = netif_list; netif != NULL; netif = netif->next) {
        if (netif_type == NETIF_TYPE_AP &&
            netif->name[0] == IFNAME10 &&
            netif->name[1] == IFNAME11) {
            name[0] = IFNAME10;
            name[1] = IFNAME11;
            if (netif->num < 10) {
                name[2] = '0' + netif->num;
                name[3] = '\0';
            } else if (netif->num < 100) {
                name[2] = '0' + (netif->num / 10);
                name[3] = '0' + (netif->num % 10);
                name[4] = '\0';
            } else {/* netif->num < 255 */
                name[2] = '0' + (netif->num / 100);
                name[3] = '0' + ((netif->num % 100) / 10);
                name[4] = '0' + (netif->num % 10);
                name[5] = '\0';
            }
            LOG_FUNC("Name %s\r\n", name);
            return 1;
        } else if (netif_type == NETIF_TYPE_STA &&
                   netif->name[0] == IFNAME00 &&
                   netif->name[1] == IFNAME01) {
            name[0] = IFNAME00;
            name[1] = IFNAME01;
            if (netif->num < 10) {
                name[2] = '0' + netif->num;
                name[3] = '\0';
            } else if (netif->num < 100) {
                name[2] = '0' + (netif->num / 10);
                name[3] = '0' + (netif->num % 10);
                name[4] = '\0';
            } else {/* netif->num < 255 */
                name[2] = '0' + (netif->num / 100);
                name[3] = '0' + ((netif->num % 100) / 10);
                name[4] = '0' + (netif->num % 10);
                name[5] = '\0';
            }
            LOG_FUNC("Name %s\r\n", name);
            return 1;
        } else if (netif_type == NETIF_TYPE_LOOPBACK &&
                   netif->name[0] == IFNAME20 &&
                   netif->name[1] == IFNAME21) {
            name[0] = IFNAME20;
            name[1] = IFNAME21;
            if (netif->num < 10) {
                name[2] = '0' + netif->num;
                name[3] = '\0';
            } else if (netif->num < 100) {
                name[2] = '0' + (netif->num / 10);
                name[3] = '0' + (netif->num % 10);
                name[4] = '\0';
            } else {/* netif->num < 255 */
                name[2] = '0' + (netif->num / 100);
                name[3] = '0' + ((netif->num % 100) / 10);
                name[4] = '0' + (netif->num % 10);
                name[5] = '\0';
            }
            LOG_FUNC("Name %s\r\n", name);
            return 1;
        }
    }

    return 0;
}

struct netif *netif_find_by_type(enum ENUM_NETIF_TYPE netif_type)
{
    char name[6] = {0};

    if (lwip_get_netif_name(netif_type, (char *)name) == 1) {
        LWIP_DEBUGF(NETIF_DEBUG,
                    ("netif_find_by_type: %c%c\n", name[0], name[1]));
        return netif_find(name);
    }

    return NULL;
}

void lwip_set_ipmode(uint8_t ipMode)
{
    LOG_FUNC("set ip mode %x", ipMode);
    rIpMode = ipMode;
}

int8_t lwip_get_ipmode(void)
{
    LOG_FUNC("get ip mode %x", rIpMode);
    return rIpMode;
}

int lwip_net_ready(void)
{
    int ret = pdTRUE;
    uint32_t to = QUERY_IP_READY_TIMEOUT;

    to = portMAX_DELAY;

    if (rIpMode == REQ_IP_MODE_DHCP) {
        ret = xSemaphoreTake(ip_ready, to / portTICK_PERIOD_MS);
        if (ret == pdTRUE) {
            /* only block get ip */
            xSemaphoreGive(ip_ready);
            LOG_FUNC("ip ready");
        } else
            LOG_FUNC("ip not ready yet");
    }

    return ret;
}

#if IP_NAPT
#if MTK_NVDM_ENABLE
static u32_t wifi_dhcpd_endian_switch(u32_t ip)
{
    u32_t swapped;
    swapped = ((ip >> 24) & 0xff) | /* move byte 3 to byte 0 */
              ((ip >> 8) & 0xff00) | /* move byte 2 to byte 1 */
              ((ip << 8) & 0xff0000) | /* move byte 1 to byte 2 */
              ((ip << 24) & 0xff000000); /* byte 0 to byte 3 */
    return swapped;
}
#endif /* #if MTK_NVDM_ENABLE */

static void wifi_dhcpd_domain_change(void)
{
#if MTK_NVDM_ENABLE
    char buf[128] = {0};
    u32_t ip_to_add;
    const ip4_addr_t *ip_unchanged = netif_ip4_addr(&ap_netif);
    const ip4_addr_t *ip_netmask = netif_ip4_netmask(&ap_netif);
    ip4_addr_t ip_changed;
    char *pStr;

    ip_to_add = (0xffffffff ^ wifi_dhcpd_endian_switch(ip_netmask->addr)) + 1;
    ip_changed.addr = wifi_dhcpd_endian_switch(ip_unchanged->addr) + ip_to_add;
    ip_changed.addr = wifi_dhcpd_endian_switch(ip_changed.addr);
    pStr = inet_ntoa(ip_changed);
    kalStrnCpy(buf, pStr, IP4ADDR_STRLEN_MAX);
    /* prevent coverity issues */
    buf[IP4ADDR_STRLEN_MAX] = '\0';

    if (nvdm_write_data_item("hapd", "IpAddr", NVDM_DATA_ITEM_TYPE_STRING,
                             (uint8_t *)buf, kalStrLen(buf)) != NVDM_STATUS_OK) {
        return;
    }
    if (nvdm_write_data_item("hapd", "IpGateway", NVDM_DATA_ITEM_TYPE_STRING,
                             (uint8_t *)buf, kalStrLen(buf)) != NVDM_STATUS_OK) {
        return;
    }
    if (nvdm_write_data_item("hapd", "IpDns1", NVDM_DATA_ITEM_TYPE_STRING,
                             (uint8_t *)buf, kalStrLen(buf)) != NVDM_STATUS_OK) {
        return;
    }
#endif /* #if MTK_NVDM_ENABLE */
}

void mtk_dhcpd_domain_reload(void)
{
#if MTK_MINISUPP_ENABLE
    wifi_event_notification_wait(WIFI_PORT_AP,
                                 WIFI_EVENT_ID_IOT_RELOAD_CONFIGURATION, NULL, 0);
#endif /* #if MTK_MINISUPP_ENABLE */
}
#endif /* #if IP_NAPT */

#if LWIP_IPV6
static void ipv6_ready_callback(struct netif *netif)
{
    int i;
#if MTK_MINISUPP_ENABLE
    int last = -1;
#endif

    if (!netif_is_up(netif)) {
        LOG_FUNC("ipv6_ready_callback interface down");
        return;
    }

    LOG_FUNC("************************");
    for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++)
    {
        // skip ANY-type addr
        if ( ip_addr_isany_val( netif->ip6_addr[i] ) ) {
            //LOG_FUNC("addr any %d", i);
            continue;
        }

        // skip addr in invalid state
        if ( netif_ip6_addr_state(netif, i) != IP6_ADDR_VALID &&
             netif_ip6_addr_state(netif, i) != IP6_ADDR_PREFERRED ) {
            LOG_FUNC("state %d skipped", netif_ip6_addr_state(netif, i));
            continue;
        }

#if MTK_MINISUPP_ENABLE
        // record the last valid addr
        last = i;
        //LOG_FUNC("last valid %d\n", i);
#endif
        LOG_FUNC("IPv6 addr: %s%%%d",
                 inet6_ntoa(netif->ip6_addr[i]),
                 ip6_addr_zone(ip_2_ip6(&netif->ip6_addr[i])) );
    }
    LOG_FUNC("************************");

#if MTK_MINISUPP_ENABLE
    if (last != -1)
    {
        uint8_t port = kalStrCmp(netif->name, "st") == 0 ?
                                 WIFI_PORT_STA : WIFI_PORT_AP;
        char *ip_addr = inet6_ntoa(netif->ip6_addr[last]);

        if (last == 0)
            LOG_FUNC( "Inform link local IPv6 addr: %s%%%d",
                      ip_addr,
                      ip6_addr_zone(ip_2_ip6(&netif->ip6_addr[i])) );
        else
            LOG_FUNC( "Inform IPv6 addr: %s", ip_addr );

        LOG_FUNC("notify");
        wifi_api_event_trigger(port,
                               WIFI_EVENT_IOT_IPV6_ADDR_READY,
                               (uint8_t *)ip_addr,
                               strlen(ip_addr));
    }
#endif
}
#endif /* LWIP_IPV6 */

void ip_ready_callback(struct netif *netif)
{
#if MTK_MINISUPP_ENABLE
    char *ip_addr = NULL;
#endif /* #if MTK_MINISUPP_ENABLE */

    if (netif_is_up(netif) && !ip_addr_isany_val(netif->ip_addr)) {
        if (NULL != inet_ntoa(netif->ip_addr)) {
            LOG_FUNC("************************");
            LOG_FUNC("DHCP got IP:%s",
                     inet_ntoa(netif->ip_addr));
            LOG_FUNC("************************");
#if IP_NAPT
            kalReleasePrivilegeCH(g_prGlueInfo);
            ip4_napt_iface_binding(&ap_netif, netif);
#endif /* #if IP_NAPT */
            xSemaphoreGive(ip_ready);
            {
                uint8_t enable;
                wifi_config_get_arp_offload(&enable);
                wifi_config_set_arp_offload(enable);
            }
#if MTK_MINISUPP_ENABLE
            ip_addr = inet_ntoa(netif->ip_addr);
            wifi_api_event_trigger(WIFI_PORT_STA,
                                   WIFI_EVENT_IOT_IPV4_ADDR_READY, (uint8_t *)ip_addr, strlen(ip_addr));
#endif /* #if MTK_MINISUPP_ENABLE */
        } else {
            LOG_E(common, "DHCP got Failed");
        }
    }

#if LWIP_IPV6
    ipv6_ready_callback(netif);
#endif

#if IP_NAPT
    if (netif_is_link_up(netif) &&
        ip4_addr_netcmp(netif_ip4_addr(&ap_netif), netif_ip4_addr(netif), netif_ip4_netmask(netif))) {
        wifi_dhcpd_domain_change();
        mtk_tcpip_set_domain_changed(TRUE);
    }
#endif /* #if IP_NAPT */
}


int lwip_tcpip_deinit(void)
{
#if LWIP_DHCP
    dhcp_stop(&sta_netif);
    dhcp_cleanup(&sta_netif);
#endif /* #if LWIP_DHCP */
    netif_set_down(&sta_netif);
    netif_remove(&sta_netif);

    netif_set_down(&ap_netif);
    netif_remove(&ap_netif);
    return 0;
}

int lwip_tcpip_init(void)
{
#if CFG_STATIC_MEM_ALLOC
    ip_info_t _sta_ip_info, _ap_ip_info;
    ip_info_t *sta_ip_info = &_sta_ip_info;
    ip_info_t *ap_ip_info = &_ap_ip_info;
#else /* #if CFG_STATIC_MEM_ALLOC */
    ip_info_t *sta_ip_info = kmalloc(sizeof(ip_info_t));
    ip_info_t *ap_ip_info = kmalloc(sizeof(ip_info_t));
#endif /* #if CFG_STATIC_MEM_ALLOC */
#if CFG_ENABLE_WIFI_DIRECT
    ip4_addr_t dns1_ip, dns2_ip;
#endif /* #if CFG_ENABLE_WIFI_DIRECT */

    if (!sta_ip_info || !ap_ip_info) {
        DBGLOG(INIT, ERROR, "[gen4m_netif]: alloc mem(%zu) fail!\r\n",
               sizeof(ip_info_t));
        return -1;
    }
    uint8_t mode[10] = {0};
    uint8_t tmp_ip[4] = {0};
    int ret = 0;

    ret = get_mode_from_nvdm((char *)mode);

    if (ret == 0 && strnicmp("static", (char *)mode, strlen("static")) == 0) {
        rIpMode = REQ_IP_MODE_STATIC;
    }
    get_ip_from_nvdm("IpAddr", tmp_ip);
    IP4_ADDR(&sta_ip_info->ip, tmp_ip[0], tmp_ip[1], tmp_ip[2], tmp_ip[3]);

    get_ip_from_nvdm("IpNetmask", tmp_ip);
    IP4_ADDR(&sta_ip_info->netmask, tmp_ip[0], tmp_ip[1], tmp_ip[2], tmp_ip[3]);

    get_ip_from_nvdm("IpGateway", tmp_ip);
    IP4_ADDR(&sta_ip_info->gw, tmp_ip[0], tmp_ip[1], tmp_ip[2], tmp_ip[3]);

    rMac[0] = 0x00;
    rMac[1] = 0x11;
    rMac[2] = 0x22;
    rMac[3] = 0x33;
    rMac[4] = 0x44;
    rMac[5] = 0x55;

    tcpip_stack_init(LWIP_STA_MODE, sta_ip_info, &rMac[0]);

#if CFG_ENABLE_WIFI_DIRECT
    if (!get_ip_from_nvdm_ap("IpAddr", tmp_ip)) {
        IP4_ADDR(&ap_ip_info->ip,
                 tmp_ip[0], tmp_ip[1], tmp_ip[2], tmp_ip[3]);
    } else
        inet_aton(AP_DEFAULT_IPADDR, &ap_ip_info->ip);

    if (!get_ip_from_nvdm_ap("IpNetmask", tmp_ip)) {
        IP4_ADDR(&ap_ip_info->netmask,
                 tmp_ip[0], tmp_ip[1], tmp_ip[2], tmp_ip[3]);
    } else
        inet_aton(AP_DEFAULT_MASK, &ap_ip_info->netmask);

    if (!get_ip_from_nvdm_ap("IpGateway", tmp_ip)) {
        IP4_ADDR(&ap_ip_info->gw,
                 tmp_ip[0], tmp_ip[1], tmp_ip[2], tmp_ip[3]);
    } else
        inet_aton(AP_DEFAULT_GW, &ap_ip_info->gw);

    if (!get_ip_from_nvdm_ap("IpDns1", tmp_ip)) {
        IP4_ADDR(&dns1_ip,
                 tmp_ip[0], tmp_ip[1], tmp_ip[2], tmp_ip[3]);
    } else
        inet_aton(PRIMARY_DNS, &dns1_ip);

    if (!get_ip_from_nvdm_ap("IpDns2", tmp_ip)) {
        IP4_ADDR(&dns2_ip,
                 tmp_ip[0], tmp_ip[1], tmp_ip[2], tmp_ip[3]);
    } else
        inet_aton(SECONDARY_DNS, &dns2_ip);

    apMac[0] = 0x00;
    apMac[1] = 0x22;
    apMac[2] = 0x79;
    apMac[3] = 0x33;
    apMac[4] = 0x79;
    apMac[5] = 0x33;

    tcpip_stack_init(LWIP_AP_MODE, ap_ip_info, &apMac[0]);
#endif /* #if CFG_ENABLE_WIFI_DIRECT */

    return 0;
}

WLAN_ATTR_TEXT_IN_MEM_RX
err_t mtk_netif_input(struct netif *inp, struct pbuf *p)
{
    uint8_t *payload = p->payload;

    DBGLOG(RX, TRACE, "[gen4m_netif]->\n");
    if ((payload[12] & 0x0F) != 0x08) {
#if LWIP_IPV6
        if (payload[12] != 0x86)
#endif /* #if LWIP_IPV6 */
        {
            DBGLOG(RX, TRACE, "[gen4m_netif] drop non ip pkt\r\n");
            return ERR_VAL;
        }
    } else if ((payload[12] == 0x88) && (payload[13] == 0x8E)) {
        struct sockaddr_in    to;
        ssize_t               len;

        if ((p->len > 12) && (g_lwip_eapol_rx_socket >= 0) && (g_lwip_eapol_rx_socket_second >= 0)) {
            /*dual interface*/
            DBGLOG(RX, TRACE, "<<Dual interface RX EAPOL (Len=%d)>>\n", p->len);

            to.sin_family      = PF_INET;
            to.sin_addr.s_addr = htonl((127 << 24) | 1);

            if ((inp->name[0] == IFNAME10) && (inp->name[1] == IFNAME11)) {
                to.sin_port = htons(66);
                LOG_FUNC("send to AP socket: %d\n",  g_lwip_eapol_rx_socket);
                len = lwip_sendto(g_lwip_eapol_rx_socket, p->payload, p->len, 0, (struct sockaddr *)&to, sizeof(to));
            } else {
                to.sin_port = htons(76);
                LOG_FUNC("send to STA socket: %d\n", g_lwip_eapol_rx_socket_second);
                len = lwip_sendto(g_lwip_eapol_rx_socket_second, p->payload, p->len, 0, (struct sockaddr *)&to, sizeof(to));
            }

            if (len != p->len)
                LOG_E(lwip, "Dual interface eapol-rx relay sendto failed!\n");
        } else if ((p->len > 12) && (g_lwip_eapol_rx_socket >= 0)) {
            /*4-way habdshake packet type*/
            DBGLOG(RX, TRACE, "<<RX EAPOL (Len=%d)>>\n", p->len);

            to.sin_family = PF_INET;
            to.sin_addr.s_addr = htonl((127 << 24) | 1);
            to.sin_port = htons(66);
            DBGLOG(RX, TRACE, "send to g_lwip_eapol_rx_socket\n");
            len = lwip_sendto(g_lwip_eapol_rx_socket, p->payload,
                              p->len, 0, (struct sockaddr *)&to,
                              sizeof(to));
            if (len != p->len)
                DBGLOG(RX, ERROR,
                       "eapol-rx relay sendto failed!\n");
        }
        return ERR_INPROGRESS;
    }
    return tcpip_input(p, inp);
}

#if CFG_ENABLE_WIFI_DIRECT
static uint32_t ip_number_to_big_endian(uint32_t ip_number)
{
    uint8_t *byte = (uint8_t *)&ip_number;

    return (uint32_t)
           ((byte[0] << 24) | (byte[1] << 16) | (byte[2] << 8) | byte[3]);
}

static void ip_number_to_string(uint32_t ip_number,
                                char ip_string[IP4ADDR_STRLEN_MAX])
{
    int32_t i4Ret = 0;

    i4Ret = snprintf(ip_string,
                     IP4ADDR_STRLEN_MAX,
                     "%d.%d.%d.%d",
                     (uint8_t)((ip_number >> 24) & 0xFF),
                     (uint8_t)((ip_number >> 16) & 0xFF),
                     (uint8_t)((ip_number >> 8) & 0xFF),
                     (uint8_t)((ip_number >> 0) & 0xFF));

    if (i4Ret < 0)
        DBGLOG(INIT, ERROR, "snprintf fail\n");
}

static void dhcpd_set_ip_pool(const ip4_addr_t *ap_ip_addr,
                              const ip4_addr_t *ap_net_mask,
                              char ip_pool_start[IP4ADDR_STRLEN_MAX],
                              char ip_pool_end[IP4ADDR_STRLEN_MAX])
{
    uint32_t ap_ip_number =
        ip_number_to_big_endian(ip4_addr_get_u32(ap_ip_addr));
    uint32_t ap_mask_number =
        ip_number_to_big_endian(ip4_addr_get_u32(ap_net_mask));
    uint32_t ip_range_min = ap_ip_number & ap_mask_number;
    uint32_t ip_range_max = ip_range_min | (~ap_mask_number);

    if ((ip_range_max - ap_ip_number) > (ap_ip_number - ip_range_min)) {
        ip_number_to_string(ap_ip_number + 1, ip_pool_start);
        ip_number_to_string(ip_range_max - 1, ip_pool_end);
    } else {
        ip_number_to_string(ip_range_min + 1, ip_pool_start);
        ip_number_to_string(ap_ip_number - 1, ip_pool_end);
    }
}

void wifi_dhcpd_init(dhcpd_settings_t *dhcpd_settings)
{
    ip_info_t _ap_ip_info;
    ip_info_t *ap_ip_info = &_ap_ip_info;
    uint8_t tmp_ip[4] = {0};
    ip4_addr_t dns1_ip, dns2_ip;
    char *pStr = NULL;

    if (!get_ip_from_nvdm_ap("IpAddr", tmp_ip)) {
        IP4_ADDR(&ap_ip_info->ip,
                 tmp_ip[0], tmp_ip[1], tmp_ip[2], tmp_ip[3]);
    } else
        inet_aton(AP_DEFAULT_IPADDR, &ap_ip_info->ip);

    if (!get_ip_from_nvdm_ap("IpNetmask", tmp_ip)) {
        IP4_ADDR(&ap_ip_info->netmask,
                 tmp_ip[0], tmp_ip[1], tmp_ip[2], tmp_ip[3]);
    } else
        inet_aton(AP_DEFAULT_MASK, &ap_ip_info->netmask);

    if (!get_ip_from_nvdm_ap("IpGateway", tmp_ip)) {
        IP4_ADDR(&ap_ip_info->gw,
                 tmp_ip[0], tmp_ip[1], tmp_ip[2], tmp_ip[3]);
    } else
        inet_aton(AP_DEFAULT_GW, &ap_ip_info->gw);

    if (!get_ip_from_nvdm_ap("IpDns1", tmp_ip)) {
        IP4_ADDR(&dns1_ip,
                 tmp_ip[0], tmp_ip[1], tmp_ip[2], tmp_ip[3]);
    } else
        inet_aton(PRIMARY_DNS, &dns1_ip);

    if (!get_ip_from_nvdm_ap("IpDns2", tmp_ip)) {
        IP4_ADDR(&dns2_ip,
                 tmp_ip[0], tmp_ip[1], tmp_ip[2], tmp_ip[3]);
    } else
        inet_aton(SECONDARY_DNS, &dns2_ip);

    pStr = ip4addr_ntoa(&ap_ip_info->ip);
    if (pStr != NULL)
        kalStrnCpy(dhcpd_settings->dhcpd_server_address,
                   pStr, IP4ADDR_STRLEN_MAX - 1);

    pStr = ip4addr_ntoa(&ap_ip_info->netmask);
    if (pStr != NULL)
        kalStrnCpy(dhcpd_settings->dhcpd_netmask,
                   pStr, IP4ADDR_STRLEN_MAX - 1);

    pStr = ip4addr_ntoa(&ap_ip_info->gw);
    if (pStr != NULL)
        kalStrnCpy(dhcpd_settings->dhcpd_gateway,
                   pStr, IP4ADDR_STRLEN_MAX - 1);

    pStr = ip4addr_ntoa(&dns1_ip);
    if (pStr != NULL)
        kalStrnCpy(dhcpd_settings->dhcpd_primary_dns,
                   pStr, IP4ADDR_STRLEN_MAX - 1);

    pStr = ip4addr_ntoa(&dns2_ip);
    if (pStr != NULL)
        kalStrnCpy(dhcpd_settings->dhcpd_secondary_dns,
                   pStr, IP4ADDR_STRLEN_MAX - 1);

    dhcpd_set_ip_pool(&ap_ip_info->ip,
                      &ap_ip_info->netmask,
                      dhcpd_settings->dhcpd_ip_pool_start,
                      dhcpd_settings->dhcpd_ip_pool_end);

    netif_set_addr(&ap_netif,
                   &ap_ip_info->ip, &ap_ip_info->netmask, &ap_ip_info->gw);
}
#endif /* #if CFG_ENABLE_WIFI_DIRECT */

// TODO: move this function to tcpip_init_done callback
static int wifi_netif_init(int mode, ip_info_t *ip_info, uint8_t *mac)
{
    struct netif *pNetif = NULL;

    DBGLOG(RX, TRACE, "[gen4m_netif] %s mac:%u\r\n", __func__, mac[1]);

    if (mode > LWIP_AP_MODE || ip_info == NULL) {
        DBGLOG(RX, ERROR, "init fail(mode:%d, ip_info:%p)\r\n", mode,
               ip_info);
        return -1;
    }

    if (mode == LWIP_STA_MODE) {
        pNetif = &sta_netif;
        pNetif->name[0] = IFNAME00;
        pNetif->name[1] = IFNAME01;
    } else if (mode == LWIP_AP_MODE) {
        pNetif = &ap_netif;
        pNetif->name[0] = IFNAME10;
        pNetif->name[1] = IFNAME11;
    }

    if (pNetif == NULL)
        return -1;

    if (mac != NULL) {
        DBGLOG(RX, TRACE, "set netif mac(%u) address\r\n", mac[1]);
        memcpy(pNetif->hwaddr, mac, NETIF_MAX_HWADDR_LEN);
        pNetif->hwaddr_len = NETIF_MAX_HWADDR_LEN;
    }

    netif_add(pNetif, &ip_info->ip, &ip_info->netmask,
              &ip_info->gw, NULL, wlan_if_init,
              (netif_input_fn)mtk_netif_input);
#if LWIP_IPV6
    pNetif->ip6_autoconfig_enabled = 1;
#endif /* #if LWIP_IPV6 */
    /* TODO: input function pointer improve */
    wlan_register_callback((wlan_netif_input_fn)mtk_netif_input,
                           pNetif, mode);

    if (mode == LWIP_AP_MODE) {
        if (opmode == WIFI_MODE_AP_ONLY || opmode == WIFI_MODE_REPEATER) {
            netif_set_default(&ap_netif);
            netif_set_link_up(&ap_netif);
        }
#ifdef MTK_SIGMA_ENABLE
        netif_set_down(&ap_netif);
#endif /* #ifdef MTK_SIGMA_ENABLE */
#ifdef IP_NAPT
        if (opmode == WIFI_MODE_REPEATER)
            mtk_tcpip_enable_napt_clean_entry_timer();
#endif /* #ifdef IP_NAPT */
    }
    /* TODO: improve default netif selection */
    if (mode == LWIP_STA_MODE) {
        if (opmode == WIFI_MODE_STA_ONLY || opmode == WIFI_MODE_REPEATER)
            netif_set_default(&sta_netif);
        netif_set_up(&sta_netif);

#if LWIP_DHCP
        if (rIpMode == REQ_IP_MODE_DHCP) {
            err_t err = ERR_OK;

            netif_set_status_callback(&sta_netif,
                                      ip_ready_callback);
#if LWIP_IPV6_DHCP6
            dhcp6_enable_stateless(&sta_netif);
#endif /* #if LWIP_IPV6_DHCP6 */
            err = dhcp_start(&sta_netif);
            if (err != ERR_OK)
                LOG_FUNC("start DHCP client failed (%d)\r\n",
                         err);
            if (!ip_ready)
                ip_ready = xSemaphoreCreateBinary();
        } else {
            dhcp_stop(&sta_netif);
        }
#endif /* #if LWIP_DHCP */
    }

    return 0;
}

err_t mtk_freertos_wlan_output(struct netif *netif, struct pbuf *p,
                               const ip4_addr_t *ipaddr)
{
    DBGLOG(TX, TRACE, "[gen4m_netif]\r\n");
    return 0;
}

err_t wlan_if_init(struct netif *netif)
{
    DBGLOG(INIT, TRACE, "[gen4m_netif]\r\n");

    /* enable hostname if mDNS is enable*/
    // netif->hostname = "Demo STA";

    NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd, 100);

    netif->output = etharp_output;
#if LWIP_IPV6
    netif->output_ip6 = ethip6_output;
#endif /* #if LWIP_IPV6 */
    netif->linkoutput = (netif_linkoutput_fn)mtk_freertos_wlan_send;
    /* netif->output = mtk_freertos_wlan_output; */

    /* netif->hwaddr_len = HW_ADDR_LEN; */
    /* confirm need to change to 1460 */
    netif->mtu = 1500;

    netif->flags =
        NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_IGMP;
#if LWIP_IPV6
    netif->flags |= NETIF_FLAG_MLD6;
#endif /* #if LWIP_IPV6 */

    netif_set_up(netif);
    /* netif_set_link_up(netif); */
    return 0;
}

int8_t tcpip_stack_init(int mode, ip_info_t *ip_info, uint8_t *mac)
{
    DBGLOG(INIT, TRACE, "[gen4m_netif] mac:%u\r\n", mac[0]);

    if (tcpip_enable == false) {
        mtk_tcpip_init(NULL, NULL);
        tcpip_enable = true;
    }
    return wifi_netif_init(mode, ip_info, mac);
}
/*
For wpa_supplicant send L2 packet,
call this api in the file of l2_packet_freertos.c
*/
err_t mtk_l2_packet_send(void *packet, uint32_t len, struct netif *netif)
{
    struct pbuf *p;
    err_t ret = 0;

    p = pbuf_alloc(PBUF_RAW_TX, len, PBUF_POOL);
    if (p == NULL) {
        DBGLOG(TX, ERROR, "pbuf_alloc fail\n");
        return -1;
    }

    memcpy(p->payload, (uint8_t *)packet, len);
    ret = mtk_freertos_wlan_send(netif, p);
    pbuf_free(p);
    return ret;
}

void register_eapol_rx_socket(int eapol_rx_socket)
{
    if (eapol_rx_socket >= 0)
        g_lwip_eapol_rx_socket = eapol_rx_socket;
    g_lwip_eapol_rx_socket_second = -1;
}

void unregister_eapol_rx_socket(void)
{
    if (g_lwip_eapol_rx_socket >= 0)
        close(g_lwip_eapol_rx_socket);
    g_lwip_eapol_rx_socket = -1;
}

void register_eapol_rx_socket_dual_intf(int eapol_rx_socket,
                                        int eapol_rx_socket_second)
{
    if (eapol_rx_socket >= 0)
        g_lwip_eapol_rx_socket = eapol_rx_socket;

    if (eapol_rx_socket_second >= 0)
        g_lwip_eapol_rx_socket_second = eapol_rx_socket_second;
}

void unregister_eapol_rx_socket_dual_intf(int eapol_rx_socket,
                                          int eapol_rx_socket_second)
{
    if (eapol_rx_socket >= 0)
        close(eapol_rx_socket);
    g_lwip_eapol_rx_socket = -1;

    if (eapol_rx_socket_second >= 0)
        close(eapol_rx_socket_second);
    g_lwip_eapol_rx_socket_second = -1;
}
