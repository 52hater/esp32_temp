#include <string.h>
#include "sdkconfig.h"
#include <time.h>
#include <sys/time.h>
#include "esp_system.h"
#include "esp_mac.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "nvs_flash.h"
#include "esp_netif_sntp.h"
#include "lwip/ip_addr.h"
#include "esp_sntp.h"
#include "rtc.h"

static const char *TAG = "rtc";

static void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "SNTP 시간 동기화 완료");
}

static void obtain_time(void)
{
    ESP_LOGI(TAG, "SNTP 초기화 및 시작");
    
    // 기본 설정으로 SNTP 초기화
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    config.sync_cb = time_sync_notification_cb;
    
    esp_netif_sntp_init(&config);
    
    // 시간이 설정될 때까지 대기
    const int retry_count = 5;
    int retry = 0;
    time_t now = 0;
    struct tm timeinfo = { 0 };
    
    ESP_LOGI(TAG, "시스템 시간 설정을 기다리는 중...");
    // 2초 이내에 SNTP 서버의 응답이 없고 재시도 횟수 초과시 ESP_ERR_TIMEOUT 반환, 동기화 완료시 ESP_OK 반환
    while (esp_netif_sntp_sync_wait(2000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT && ++retry < retry_count) {
        ESP_LOGI(TAG, "시스템 시간 설정을 기다리는 중... (%d/%d)", retry, retry_count);
    }
    
    // 동기화 된 시간이 유효한지 확인
    time(&now);
    localtime_r(&now, &timeinfo);
    
    //한국 시간으로 설정 (GMT+9)
    setenv("TZ", "KST-9", 1);
    tzset();
    
    char strftime_buf[64];
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    ESP_LOGI(TAG, "현재 한국 시간: %s", strftime_buf);
    
    esp_netif_sntp_deinit();
}

esp_err_t sntp_rtc_init(void)
{
    /* NVS 초기화 */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    /* TCP/IP 스택 초기화 */
    // ESP_ERROR_CHECK(esp_netif_init());
    // ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // 와이파이 연결 먼저
    // 시간 설정
    obtain_time();
    
    /* 현재 시간 확인 */
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    // 확인된 현재시간이 2025년 이전이면 sntp가 잘못설정되어 있는 것이므로 ESP_FAIL
    // 최대 재시도 횟수만큼 재시도(5)
    const int retry_count = 5;
    int retry = 0;
    while (timeinfo.tm_year < (2025 - 1900) && retry < retry_count) 
    {
        retry++;
        ESP_LOGE(TAG, "시간 설정 실패, 재시도 중 (%d/%d)", retry + 1, retry_count);
        vTaskDelay(pdMS_TO_TICKS(500)); // 0.5초 대기 후 재시도

        // 재시도 : 다시 시간 동기화 시도
        obtain_time(); // 시간을 다시 동기화 하는 함수 호출

        // 재시도 후 시간 확인
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    if (timeinfo.tm_year < (2025 - 1900)) 
    {
        ESP_LOGE(TAG, "시간 설정 실패. 최대 재시도 횟수 초과.");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "시간 동기화 성공");
    return ESP_OK;
}

char* rtc_get_time(char *time_str, size_t size)
{
    time_t now;
    struct tm timeinfo;
    
    time(&now);
    localtime_r(&now, &timeinfo);
    
    strftime(time_str, size, "%Y-%m-%d %H:%M:%S", &timeinfo);
    return time_str;
}

time_t rtc_get_timestamp(void)
{
    time_t now;
    time(&now);
    return now;
}
