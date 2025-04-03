#ifndef CONFIG_WIFI

#include "ble.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_common_api.h"
#include "esp_gatts_api.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <string.h>

static const char *TAG = "ble";

#define PROFILE_NUM      1
#define PROFILE_APP_IDX  0
#define ESP_APP_ID       0x55
#define DEVICE_NAME      "ESP_WIFI_PROV"
#define GATTS_NUM_HANDLE 4

#define WIFI_CREDS_RECEIVED_BIT BIT0

static uint8_t adv_config_done = 0;
static uint16_t profile_handle_table[GATTS_NUM_HANDLE];
static EventGroupHandle_t ble_event_group;
static wifi_credentials_t wifi_creds;
static esp_gatt_if_t global_gatts_if = 0;

static esp_bt_uuid_t service_uuid = {
    .len  = ESP_UUID_LEN_128,
    .uuid = {
        .uuid128 = {0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10,
                    0x00, 0x00, 0xFF, 0x00, 0x00, 0x00},
    }};

static esp_bt_uuid_t char_uuid = {
    .len  = ESP_UUID_LEN_128,
    .uuid = {
        .uuid128 = {0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10,
                    0x00, 0x00, 0xFF, 0x01, 0x00, 0x00},
    }};

static esp_gatt_srvc_id_t service_id = {
    .id =
        {
            .inst_id = 0,
            .uuid =
                {
                    .len = ESP_UUID_LEN_128,
                },
        },
    .is_primary = true,
};

static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp        = false,
    .include_name        = true,
    .include_txpower     = true,
    .min_interval        = 0x20,
    .max_interval        = 0x40,
    .appearance          = 0x00,
    .manufacturer_len    = 0,
    .p_manufacturer_data = NULL,
    .service_data_len    = 0,
    .p_service_data      = NULL,
    .service_uuid_len    = sizeof(service_uuid.uuid.uuid128),
    .p_service_uuid      = service_uuid.uuid.uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_ble_adv_data_t scan_rsp_data = {
    .set_scan_rsp        = true,
    .include_name        = true,
    .include_txpower     = true,
    .min_interval        = 0x20,
    .max_interval        = 0x40,
    .appearance          = 0x00,
    .manufacturer_len    = 0,
    .p_manufacturer_data = NULL,
    .service_data_len    = 0,
    .p_service_data      = NULL,
    .service_uuid_len    = 0,
    .p_service_uuid      = NULL,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_ble_adv_params_t adv_params = {
    .adv_int_min       = 0x20,
    .adv_int_max       = 0x40,
    .adv_type          = ADV_TYPE_IND,
    .own_addr_type     = BLE_ADDR_TYPE_PUBLIC,
    .channel_map       = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
                                        esp_gatt_if_t gatts_if,
                                        esp_ble_gatts_cb_param_t *param)
{
    ESP_LOGI(TAG, "GATTS event: %d", event);

    switch (event)
    {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(TAG, "GATTS register app ID: %x", param->reg.app_id);
        global_gatts_if = gatts_if;

        esp_ble_gap_set_device_name(DEVICE_NAME);
        esp_ble_gap_config_adv_data(&adv_data);
        esp_ble_gap_config_adv_data(&scan_rsp_data);

        memcpy(service_id.id.uuid.uuid.uuid128, service_uuid.uuid.uuid128,
               ESP_UUID_LEN_128);

        esp_ble_gatts_create_service(gatts_if, &service_id, GATTS_NUM_HANDLE);
        break;

    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(TAG, "CREATE service handle %d", param->create.service_handle);
        profile_handle_table[0] = param->create.service_handle;
        esp_ble_gatts_start_service(profile_handle_table[0]);

        esp_ble_gatts_add_char(profile_handle_table[0], &char_uuid,
                               ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                               ESP_GATT_CHAR_PROP_BIT_READ |
                                   ESP_GATT_CHAR_PROP_BIT_WRITE,
                               NULL, NULL);
        break;

    case ESP_GATTS_CONNECT_EVT:
        ESP_LOGI(TAG, "GATTS connection establish, conn_id = %d",
                 param->connect.conn_id);
        esp_ble_set_encryption(param->connect.remote_bda,
                               ESP_BLE_SEC_ENCRYPT_MITM);
        break;

    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(TAG, "GATTS disconnect");
        esp_ble_gap_start_advertising(&adv_params);
        break;

    case ESP_GATTS_WRITE_EVT:
        if (param->write.len > 0)
        {
            char data[param->write.len + 1];
            memcpy(data, param->write.value, param->write.len);
            data[param->write.len] = '\0';

            char *ssid     = strtok(data, ",");
            char *password = strtok(NULL, ",");

            if (ssid && password)
            {
                memset(&wifi_creds, 0, sizeof(wifi_credentials_t));
                strncpy(wifi_creds.ssid, ssid, sizeof(wifi_creds.ssid) - 1);
                strncpy(wifi_creds.password, password,
                        sizeof(wifi_creds.password) - 1);
                wifi_creds.credentials_received = true;

                ESP_LOGI(TAG, "Received SSID: %s", wifi_creds.ssid);
                ESP_LOGI(TAG, "Received Password: %s", wifi_creds.password);

                xEventGroupSetBits(ble_event_group, WIFI_CREDS_RECEIVED_BIT);
            }
        }
        break;

    default:
        break;
    }
}

static void gap_event_handler(esp_gap_ble_cb_event_t event,
                              esp_ble_gap_cb_param_t *param)
{
    ESP_LOGI(TAG, "GAP event: %d", event);

    switch (event)
    {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        adv_config_done &= (~ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT);
        if (adv_config_done == 0)
        {
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;

    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
        adv_config_done &= (~ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT);
        if (adv_config_done == 0)
        {
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;

    case ESP_GAP_BLE_SEC_REQ_EVT:
        ESP_LOGI(TAG, "SEC_REQ");
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
        break;

    case ESP_GAP_BLE_AUTH_CMPL_EVT:
        if (param->ble_security.auth_cmpl.success)
        {
            ESP_LOGI(TAG, "Authentication success");
        }
        else
        {
            ESP_LOGE(TAG, "Authentication failed, status: %d",
                     param->ble_security.auth_cmpl.fail_reason);
        }
        break;

    default:
        break;
    }
}

esp_err_t ble_prov_init(void)
{
    esp_err_t ret;
    const int retry_count = 5;
    int retry = 0;

    ble_event_group = xEventGroupCreate();

    // Release classic BT memory
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    // 컨트롤러 초기화, 재시도 로직 추가
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    
    while ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK && retry < retry_count) {
        ESP_LOGW(TAG, "Initialize controller failed (attempt %d/%d): %s", 
            retry+1, retry_count, esp_err_to_name(ret));
        vTaskDelay(pdMS_TO_TICKS(500)); // 0.5초 대기 후 재시도
        retry++;
    }
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Initialize controller failed after %d attempts: %s", 
            retry_count, esp_err_to_name(ret));
        return ret;
    }

    // Initialize Bluedroid
    ret = esp_bluedroid_init();
    if (ret)
    {
        ESP_LOGE(TAG, "init bluedroid failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bluedroid_enable();
    if (ret)
    {
        ESP_LOGE(TAG, "enable bluedroid failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Register callbacks
    ret = esp_ble_gatts_register_callback(gatts_profile_event_handler);
    if (ret)
    {
        ESP_LOGE(TAG, "register gatts callback failed: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret)
    {
        ESP_LOGE(TAG, "register gap callback failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Set security parameters
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;
    esp_ble_io_cap_t iocap      = ESP_IO_CAP_NONE;
    uint8_t key_size            = 16;
    uint8_t init_key            = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key             = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;

    ret = esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req,
                                         sizeof(auth_req));
    ret |= esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap,
                                          sizeof(iocap));
    ret |= esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size,
                                          sizeof(key_size));
    ret |= esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key,
                                          sizeof(init_key));
    ret |= esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key,
                                          sizeof(rsp_key));

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "set security param failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_ble_gatts_app_register(ESP_APP_ID);
    if (ret)
    {
        ESP_LOGE(TAG, "register app failed: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}

esp_err_t ble_prov_start(void)
{
    esp_err_t ret;

    ret = esp_ble_gap_set_device_name(DEVICE_NAME);
    if (ret)
    {
        ESP_LOGE(TAG, "set device name failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_ble_gap_config_adv_data(&adv_data);
    if (ret)
    {
        ESP_LOGE(TAG, "config adv data failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
    if (ret)
    {
        ESP_LOGE(TAG, "config scan response data failed: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}

// 이벤트 그룹을 사용하여 WiFi 자격 증명 수신을 대기
esp_err_t ble_prov_get_credentials(wifi_credentials_t *credentials)
{
    xEventGroupWaitBits(ble_event_group, WIFI_CREDS_RECEIVED_BIT, pdTRUE,
                        pdTRUE, portMAX_DELAY);

    memcpy(credentials, &wifi_creds, sizeof(wifi_credentials_t));
    return ESP_OK;
}

esp_err_t ble_prov_stop(void)
{
    esp_err_t ret;

    ret = esp_ble_gap_stop_advertising();
    if (ret)
    {
        ESP_LOGE(TAG, "stop advertising failed: %s", esp_err_to_name(ret));
        return ret;
    }

    if (global_gatts_if)
    {
        ret = esp_ble_gatts_stop_service(profile_handle_table[0]);
        if (ret)
        {
            ESP_LOGE(TAG, "stop service failed: %s", esp_err_to_name(ret));
            return ret;
        }
    }

    return ESP_OK;
}

#endif