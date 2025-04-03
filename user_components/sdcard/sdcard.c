#include "sdcard.h"

static const char *mount_point = "/sdcard";
static const char *TAG         = "sdcard";
static sdmmc_card_t *card      = NULL;

void sdcard_init(void)
{
    esp_err_t ret;
    const int retry_count = 5;
    int retry = 0;
    
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files              = 5,
        .allocation_unit_size   = 16 * 1024
    };

    ESP_LOGI(TAG, "SD 카드 초기화 중");

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();

    spi_bus_config_t bus_cfg = {
        .mosi_io_num     = PIN_NUM_MOSI,
        .miso_io_num     = PIN_NUM_MISO,
        .sclk_io_num     = PIN_NUM_CLK,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = 4000,
    };

    ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "버스 초기화 실패: %s", esp_err_to_name(ret));
        return;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs               = PIN_NUM_CS;
    slot_config.host_id               = host.slot;

    ESP_LOGI(TAG, "파일 시스템 마운트 중");

    while (retry < retry_count) {
        ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config,
                                      &mount_config, &card);
        
        if (ret == ESP_OK) {
            break; // 성공시 반복 종료
        }
        
        ESP_LOGW(TAG, "SD 카드 마운트 실패 (시도 %d/%d): %s", 
            retry+1, retry_count, esp_err_to_name(ret));
        
        retry++;
        if (retry < retry_count) {
            vTaskDelay(pdMS_TO_TICKS(500)); // 0.5초 대기 후 재시도
        }
    }

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "파일 시스템 마운트 실패");
        }
        else
        {
            ESP_LOGE(TAG, "SD 카드 초기화 실패 (%s)",
                esp_err_to_name(ret));
        }
        return;
    }
    
    ESP_LOGI(TAG, "파일 시스템 마운트 완료");
    sdmmc_card_print_info(stdout, card);
}

const char* get_mount_point(void)
{
    return mount_point;
}