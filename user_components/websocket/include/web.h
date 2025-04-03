#ifndef WEB_H
#define WEB_H

#include "def.h"

// #ifdef CUSTOM_IP
// #define IP4_IP_CUSTOM      IP4_IP_SET
// #define IP4_GATEWAY_CUSTOM IP4_GATEWAY_SET
// #define IP4_NETMASK_CUSTOM IP4_NETMASK_SET
// #define IP4_DNS_CUSTOM     IP4_DNS_SET
// #endif

// #ifdef CUSTOM_mDNS
// #define CONFIG_MDNS_HOSTNAME mDNS_HOST_NAME
// #endif

#ifdef CONFIG_WIFI
#define CONFIG_ESP_WIFI_SSID     WIFI_SSID
#define CONFIG_ESP_WIFI_PASSWORD WIFI_PASSWORD
#endif

void websocket_init(void);
#endif