#ifndef BLE_H
#define BLE_H

#include "def.h"

#ifndef CONFIG_WIFI

#include <stdbool.h>

#define MAX_SSID_LEN_SET     MAX_SSID_LENGTH
#define MAX_PASSWORD_LEN_SET MAX_PASSWORD_LENGTH

typedef struct
{
    char ssid[MAX_SSID_LEN_SET];
    char password[MAX_PASSWORD_LEN_SET];
    bool credentials_received;
} wifi_credentials_t;

esp_err_t ble_prov_init(void);
esp_err_t ble_prov_get_credentials(wifi_credentials_t *credentials);
esp_err_t ble_prov_start(void);
esp_err_t ble_prov_stop(void);

#endif

#endif