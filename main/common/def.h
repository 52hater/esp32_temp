#ifndef DEF_H
#define DEF_H

#include "esp_err.h"
#include "esp_log.h"
#include "sdkconfig.h"

// DS18B20 PIN SET
#define DS18B20_DAT GPIO_NUM_4

// SDCARD SET
#define SDCARD_MISO GPIO_NUM_13
#define SDCARD_MOSI GPIO_NUM_11
#define SDCARD_CLK  GPIO_NUM_12
#define SDCARD_CS   GPIO_NUM_10

#define MOUNT_POINT_SET "/sdcard"

// NTP TIMEZONE SET
#define TIMEZONE_NTP "KST-9"
#define SERVER_NTP   "time.windows.com"

// CONFIG_WIFI가 정의되어 있으면 : 하드코딩 된 SSID와 비번사용
// IF NOT USE BLE(Enter directly)
// 아래의 CONFIG_WIFI 정의를 주석 해제 / 주석 으로 모드변경 > 현재 주석 > BLE 프로비저닝 사용
// #define CONFIG_WIFI
#ifdef CONFIG_WIFI
#define WIFI_SSID     "SSID"
#define WIFI_PASSWORD "PASSWORD"
#endif

// CONFIG_WIFI가 정의되어있지 않으면 BLE 프로비저닝 사용 (MAX_SSID_LENTH, MAX_PASSWORD_LENGTH 정의)
// WIFI SET
#ifndef CONFIG_WIFI
#define MAX_SSID_LENGTH     32
#define MAX_PASSWORD_LENGTH 64
#endif

// #define CUSTOM_IP
#ifdef CUSTOM_IP
#define IP4_IP_SET      {192, 168, 0, 3}
#define IP4_GATEWAY_SET {192, 168, 0, 1}
#define IP4_NETMASK_SET {255, 255, 255, 0}
#define IP4_DNS_SET     {8, 8, 8, 8}
#endif

// #define CUSTOM_mDNS
#ifdef CUSTOM_mDNS
#define mDNS_HOST_NAME "sems"
#endif

// DB SET
#define DB_NAME "temp.db"
#define SAVE_TIME_INTERVAL 60000

#endif