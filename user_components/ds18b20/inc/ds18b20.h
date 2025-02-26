#ifndef DS18B20_H
#define DS18B20_H

#include <esp_err.h>

#define HW_DS18B20_GPIO GPIO_NUM_4

esp_err_t ds18b20_init(void);
double ds18b20_get_temp(void);

#endif