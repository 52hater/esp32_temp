#include "sdcard.h"

static const char *mount_point = "/sdcard";
static const char *TAG         = "sdcard";
static sdmmc_card_t *card      = NULL;

void sdcard_init(void)
{
    esp_err_t ret;
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files              = 5,
        .allocation_unit_size   = 16 * 1024};

    ESP_LOGI(TAG, "Initializing SD card");

    sdmmc_host_t host        = SDSPI_HOST_DEFAULT();

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
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs               = PIN_NUM_CS;
    slot_config.host_id               = host.slot;

    ESP_LOGI(TAG, "Mounting filesystem");

    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config,
                                  &mount_config, &card);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(
                TAG,
                "Failed to mount filesystem. "
                "If you want the card to be formatted, set the "
                "CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        }
        else
        {
            ESP_LOGE(TAG,
                     "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.",
                     esp_err_to_name(ret));
        }
        return;
    }
    ESP_LOGI(TAG, "Filesystem mounted");

    sdmmc_card_print_info(stdout, card);

}

const char* get_mount_point(void)
{
    return mount_point;
}