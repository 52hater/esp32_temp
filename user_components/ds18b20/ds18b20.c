#include "ds18b20.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"

// DS18B20 명령어
#define DS18B20_SKIP_ROM           0xCC
#define DS18B20_CONVERT_T          0x44
#define DS18B20_READ_SCRATCHPAD    0xBE

static const gpio_num_t DS18B20_GPIO = HW_DS18B20_GPIO;

// 1-Wire 통신 구현 (비트 단위 통신)
static void write_bit(int bit)
{
    gpio_set_direction(DS18B20_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(DS18B20_GPIO, 0);
    ets_delay_us(5);
    
    if (bit) {
        gpio_set_level(DS18B20_GPIO, 1);
    }
    ets_delay_us(60);
    gpio_set_level(DS18B20_GPIO, 1);
    ets_delay_us(1);
}

static int read_bit(void)
{
    int bit;
    gpio_set_direction(DS18B20_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(DS18B20_GPIO, 0);
    ets_delay_us(2);
    
    gpio_set_direction(DS18B20_GPIO, GPIO_MODE_INPUT);
    ets_delay_us(10);
    bit = gpio_get_level(DS18B20_GPIO);
    ets_delay_us(50);
    
    return bit;
}

// 바이트 단위 읽기/쓰기
static void write_byte(uint8_t data)
{
    for (int i = 0; i < 8; i++) {
        write_bit(data & 1);
        data >>= 1;
    }
}

static uint8_t read_byte(void)
{
    uint8_t data = 0;
    for (int i = 0; i < 8; i++) {
        data >>= 1;
        if (read_bit()) {
            data |= 0x80;
        }
    }
    return data;
}

// 센서 리셋 및 존재 확인 (통신 준비 상태를 만듦)
static esp_err_t reset(void)
{
    gpio_set_direction(DS18B20_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(DS18B20_GPIO, 0);
    ets_delay_us(480);
    
    gpio_set_direction(DS18B20_GPIO, GPIO_MODE_INPUT);
    ets_delay_us(70);
    
    int presence = gpio_get_level(DS18B20_GPIO);
    ets_delay_us(410);
    
    return presence ? ESP_FAIL : ESP_OK;
}

esp_err_t ds18b20_init(void)
{
    gpio_reset_pin(DS18B20_GPIO);
    return ESP_OK;
}

// 온도 측정, 섭씨로 변환
double ds18b20_get_temp(void)
{
    if (reset() != ESP_OK) {
        return -999.0f;  // 에러 값 반환
    }

    // 온도 변환 시작
    write_byte(DS18B20_SKIP_ROM);
    write_byte(DS18B20_CONVERT_T);
    
    ets_delay_us(750000);  // 변환 대기 (750ms)
    
    if (reset() != ESP_OK) {
        return -999.0f;
    }
    
    // 온도값 읽기
    write_byte(DS18B20_SKIP_ROM);
    write_byte(DS18B20_READ_SCRATCHPAD);
    
    uint8_t temp_l = read_byte();
    uint8_t temp_h = read_byte();
    
    int16_t temp = (temp_h << 8) | temp_l;
    return temp * 0.0625;  // 온도 변환 (섭씨)
}