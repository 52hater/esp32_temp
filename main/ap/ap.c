#include "ap.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "web.h"
#include "time.h"

static const char *TAG = "sdcard";

void ap_init(void)
{
}


void sqlite(void *pvParameter)
{
    while (1)
    {
        double temp = ds18b20_get_temp();

        // 현재 시간 가져오기 (로그에 현재 시간과 온도 출력 >> 로그 확인용도)
        char time_str[64];
        rtc_get_time(time_str, sizeof(time_str));
        ESP_LOGI(TAG, "현재 시간: %s, 온도: %.2f", time_str, temp);

        // SQLite DB에 데이터를 저장, 경로, 파일이름 등 // temp.db SQLite DB 파일에 저장, 파일내의 temperature 테이블에 온도와 시간 레코드
        char db_path[32];
        snprintf(db_path, sizeof(db_path), "%s/temp.db", get_mount_point());
        ESP_LOGI(TAG, "Database Path: %s", db_path);

        sqlite3 *db;
        sqlite3_initialize();

        int rc = db_open(db_path, &db);
        if (rc != SQLITE_OK)
        {
            ESP_LOGE(TAG, "Failed to open database");
            vTaskDelay(pdMS_TO_TICKS(1000)); // 오류 발생 시 1초 대기 후 재시도
            continue;
            //vTaskDelete(NULL);
        }
        ESP_LOGI(TAG, "open");
        vTaskDelay(pdMS_TO_TICKS(100));

        // temperature 테이블 생성 (테이블 존재하지 않으면)
        rc = db_exec(db, "CREATE TABLE IF NOT EXISTS temperature ("
                         "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                         "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP, "
                         "temperature FLOAT);");
        if (rc != SQLITE_OK)
        {
            ESP_LOGE(TAG, "Failed to create query");
            vTaskDelete(NULL);
        }

        sqlite3_stmt *stmt;
        // 데이터 삽입 - sqlite에서 시간은 자동으로 레코딩 시간이 삽입됨
        const char *sql = "INSERT INTO temperature (temperature) VALUES (?);";

        rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
        if (rc != SQLITE_OK)
        {
            ESP_LOGE(TAG, "Failed to prepare statement");
            vTaskDelete(NULL);
        }


        // 온도 값 바인딩 (온도 값을 첫 번째 ?자리에 바인딩) - 시간은 sqlite에서 자동삽입
        sqlite3_bind_double(stmt, 1, temp);

        // 준비 된 쿼리 실행 > db에 해당 레코드 삽입
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE)
        {
            ESP_LOGE(TAG, "Failed to execute statement");
            vTaskDelete(NULL);
        }

        // 쿼리 해제
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        
        // 현재 실행 중인 태스크를 5분 동안 대기 상태로 > 5분에 한번씩 저장 (루프 안돌고 태스크는 대기중)
        vTaskDelay(pdMS_TO_TICKS(600000)); // 10분 대기
    }
}

void ap_main(void)
{
    // 먼저 와이파이연결, 웹소켓 초기화 (websocket_init이 와이파이 연결까지 수행)
    websocket_init();
    
    // 와이파이 연결이후에 sntp 받아와야 되니까 그 뒤에
    sntp_rtc_init();

    // 시간 동기화 후 SQLite 태스크 시작
    // 태스크실행할함수, 태스크이름, 태스크할당스택크기(바이트), 태스크함수에전달할파라미터, 태스크우선순위,태스크핸들을저장할변수
    xTaskCreate(&sqlite, "SQLITE3", 1024 * 6, NULL, 5, NULL);
}