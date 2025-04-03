/*
    sqlite for esp-idf

	This example code is in the Public Domain (or CC0 licensed, at your option.)
	Unless required by applicable law or agreed to in writing, this
	software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
	CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include "sdkconfig.h"
#include "ble.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/message_buffer.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "mdns.h"
#include "sdcard.h"
#include "web.h"

#include "websocket_server.h"

MessageBufferHandle_t xMessageBufferToClient;

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

static const char *TAG = "websocket";

static int s_retry_num = 0;

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		esp_wifi_connect();
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		if (s_retry_num < CONFIG_ESP_MAXIMUM_RETRY) {
			esp_wifi_connect();
			s_retry_num++;
			ESP_LOGI(TAG, "retry to connect to the AP");
		} else {
			xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
		}
		ESP_LOGI(TAG,"connect to the AP fail");
	} else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
		ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
		s_retry_num = 0;
		xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
	}
}

void wifi_init_sta(void)
{
	#ifndef CONFIG_WIFI
    ESP_ERROR_CHECK(ble_prov_init());
    ESP_ERROR_CHECK(ble_prov_start());

    wifi_credentials_t credentials;
    ESP_ERROR_CHECK(ble_prov_get_credentials(&credentials));
	#endif

	s_wifi_event_group = xEventGroupCreate();

	ESP_ERROR_CHECK(esp_netif_init());

	ESP_ERROR_CHECK(esp_event_loop_create_default());


	#ifndef CUSTOM_IP
    esp_netif_create_default_wifi_sta();
	#endif

	#ifdef CUSTOM_IP
    esp_netif_t *netif = esp_netif_create_default_wifi_sta();
    ESP_ERROR_CHECK(esp_netif_dhcpc_stop(netif));

    static const int ip[]      = IP4_IP_CUSTOM;
    static const int gateway[] = IP4_GATEWAY_CUSTOM;
    static const int netmask[] = IP4_NETMASK_CUSTOM;
    static const int dns[]     = IP4_DNS_CUSTOM;

    esp_netif_ip_info_t ip_info = {0};
    IP4_ADDR(&ip_info.ip, ip[0], ip[1], ip[2], ip[3]);
    IP4_ADDR(&ip_info.gw, gateway[0], gateway[1], gateway[2], gateway[3]);
    IP4_ADDR(&ip_info.netmask, netmask[0], netmask[1], netmask[2], netmask[3]);
    ESP_ERROR_CHECK(esp_netif_set_ip_info(netif, &ip_info));

    esp_netif_dns_info_t dns_info = {0};
    esp_ip4_addr_t dns_ip;
    IP4_ADDR(&dns_ip, dns[0], dns[1], dns[2], dns[3]);
    dns_info.ip.u_addr.ip4 = dns_ip;
    dns_info.ip.type       = IPADDR_TYPE_V4;
    ESP_ERROR_CHECK(
        esp_netif_set_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns_info));
#endif


	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	esp_event_handler_instance_t instance_any_id;
	esp_event_handler_instance_t instance_got_ip;
	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
		ESP_EVENT_ANY_ID,
		&event_handler,
		NULL,
		&instance_any_id));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
		IP_EVENT_STA_GOT_IP,
		&event_handler,
		NULL,
		&instance_got_ip));

	wifi_config_t wifi_config = {
		.sta = {
			// .ssid = CONFIG_ESP_WIFI_SSID,
			// .password = CONFIG_ESP_WIFI_PASSWORD,
			/* Setting a password implies station will connect to all security modes including WEP/WPA.
			 * However these modes are deprecated and not advisable to be used. Incase your Access point
			 * doesn't support WPA2, these mode can be enabled by commenting below line */
			.threshold.authmode = WIFI_AUTH_WPA2_PSK,

			.pmf_cfg = {
				.capable = true,
				.required = false
			},
		},
	};


	#ifndef CONFIG_WIFI
    strcpy((char *)wifi_config.sta.ssid, credentials.ssid);
    strcpy((char *)wifi_config.sta.password, credentials.password);
	#endif


	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
	ESP_ERROR_CHECK(esp_wifi_start() );

	ESP_LOGI(TAG, "wifi_init_sta finished.");

	/* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
	 * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
	EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

	/* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
	 * happened. */
	if (bits & WIFI_CONNECTED_BIT) {
		ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
	} else if (bits & WIFI_FAIL_BIT) {
		ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
	} else {
		ESP_LOGE(TAG, "UNEXPECTED EVENT");
	}

	/* The event will not be processed after unregister */
	ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
	ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
	vEventGroupDelete(s_wifi_event_group);
}

void initialise_mdns(void)
{
	//initialize mDNS
	ESP_ERROR_CHECK( mdns_init() );
	//set mDNS hostname (required if you want to advertise services)
	ESP_ERROR_CHECK( mdns_hostname_set(CONFIG_MDNS_HOSTNAME) );
	ESP_LOGI(TAG, "mdns hostname set to: [%s]", CONFIG_MDNS_HOSTNAME);

	//initialize service
	ESP_ERROR_CHECK( mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0) );
#if 0
	//set default mDNS instance name
	ESP_ERROR_CHECK( mdns_instance_name_set("ESP32 with mDNS") );
#endif
}

void client_task(void* pvParameters);
void server_task(void* pvParameters);

void websocket_init()
{
	// Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_LOGW(TAG, "NVS no free pages or new version found, erasing...");
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "NVS initialization failed: %s", esp_err_to_name(ret));
    }
    ESP_ERROR_CHECK(ret);

    wifi_init_sta();

    initialise_mdns();

    xMessageBufferToClient = xMessageBufferCreate(1024);
    configASSERT(xMessageBufferToClient);

    // Get the local IP address
    esp_netif_ip_info_t ip_info;
    ESP_ERROR_CHECK(esp_netif_get_ip_info(
        esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &ip_info));
    char cparam0[64];
    sprintf(cparam0, IPSTR, IP2STR(&ip_info.ip));
    // sprintf(cparam0, "%s.local", CONFIG_MDNS_HOSTNAME);

    ws_server_start();

    xTaskCreate(&server_task, "server_task", 1024 * 8, (void *)cparam0, 5,
                NULL);

    xTaskCreate(&client_task, "client_task", 1024 * 6,
                (void *)get_mount_point(), 5, NULL);

    vTaskDelay(100);
}
