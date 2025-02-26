#ifndef RTC_H
#define RTC_H

#include <time.h>
#include <sys/time.h>
#include "esp_err.h"

// SNTP 서비스를 초기화하고 RTC 시간 업데이트
// 성공시 ESP_OK, 실패시 ESP_FAIL 
esp_err_t sntp_rtc_init(void);

// 현재 시각을 한국 시간 문자열로 반환
// 버퍼, 버퍼 크기
char* rtc_get_time(char *time_str, size_t size);

// 현재 시간 반환
time_t rtc_get_timestamp(void);

#endif // RTC_H
