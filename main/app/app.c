#include "app.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "web.h"
#include "time.h"

static const char *TAG = "sdcard";

void app_init(void)
{
}

// 태스크 수행시간 : 최소 1000ms ~ 최대 4000ms
void sqlite(void *pvParameter) // xTaskCreate(&sqlite, "SQLITE3", 1024 * 6, NULL, 5, NULL); > 4번째 인자 NULL 전달(사용하고 있지 않으므로)
{
    // 무한 루프로 지속적으로 온도 측정 및 저장 작업 수행
    while (1) 
    {
        // 온도 읽기
        double temp = ds18b20_get_temp();
        
        // 현재 시간 가져오기
        char time_str[64];
        rtc_get_time(time_str, sizeof(time_str));
        
        // 온도 센서 오류 발생 시
        if (temp == -999.0f) {
            // 로깅 (현재시간과 함께)
            ESP_LOGE(TAG, "%s, 온도 센서 읽기 실패", time_str);
            
            // 오류를 DB에 기록하기 위한 경로 설정
            char db_path[32];
            snprintf(db_path, sizeof(db_path), "%s/temp.db", get_mount_point());
            
            sqlite3 *db;
            
            // DB 열기 재시도 로직
            int db_retry = 0;  // 재시도 카운터
            const int db_retry_count = 5;  // 최대 재시도 횟수
            int ret;  // 반환값 저장 변수
            
            // DB 열기 최대 3회 시도
            while (db_retry < db_retry_count) {
                ret = db_open(db_path, &db);  // DB 열기 시도
                if (ret == SQLITE_OK) {  // 성공 시 루프 종료
                    break;
                }
                
                // 실패 시 로그 출력
                ESP_LOGW(TAG, "DB 열기 실패 (시도 %d/%d)", db_retry+1, db_retry_count);
                db_retry++;  // 재시도 카운터 증가
                
                // 아직 재시도 기회가 남았으면 500ms 대기 후 재시도
                if (db_retry < db_retry_count) {
                    vTaskDelay(pdMS_TO_TICKS(500));
                }
            }
            
            // 최대 시도 후에도 DB 열기 실패 시
            if (ret != SQLITE_OK) {
                ESP_LOGE(TAG, "DB 열기 실패: 최대 재시도 횟수 초과");
                // 0.5초 후 다시 시도 (오류 발생 시 짧은 대기)
                vTaskDelay(pdMS_TO_TICKS(500));
                continue;  // 현재 루프 나머지 부분 건너뛰기
            }
            
            // 오류 로그 테이블 생성
            ret = db_exec(db, "CREATE TABLE IF NOT EXISTS error_log ("
                        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                        "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP, "
                        "error_type TEXT, "
                        "message TEXT);");
            
            // 테이블 생성에 실패하면 로그 출력 후 루프 재시작
            if (ret != SQLITE_OK) {
                ESP_LOGE(TAG, "오류 로그 테이블 생성 실패: %s", sqlite3_errmsg(db));
                sqlite3_close(db);
                // 0.5초 후 다시 시도 (오류 발생 시 짧은 대기)
                vTaskDelay(pdMS_TO_TICKS(500));
                continue;
            }
            
            // 오류 정보 삽입 (실패 시 로그만 출력하고 계속 진행)
            char *sql = "INSERT INTO error_log (error_type, message) VALUES ('SENSOR_ERROR', '온도 센서 읽기 실패');";
            ret = db_exec(db, sql);
            if (ret != SQLITE_OK) {
                ESP_LOGE(TAG, "오류 정보 기록 실패: %s", sqlite3_errmsg(db));
            }
            
            // DB 연결 닫기
            sqlite3_close(db);
            
            // 1초 후 다시 시도 (오류 발생 시 짧은 대기)
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;  // 현재 루프 나머지 부분 건너뛰기
        }
        
        // 온도 센서 정상 작동 시 로그 출력
        ESP_LOGI(TAG, "현재 시간: %s, 온도: %.2f", time_str, temp);

        // DB 파일 경로 설정
        char db_path[32];
        snprintf(db_path, sizeof(db_path), "%s/temp.db", get_mount_point());
        ESP_LOGI(TAG, "Database Path: %s", db_path);

        // SQLite DB 변수 선언
        sqlite3 *db;

        // DB 열기 재시도 로직
        int db_retry = 0;
        int db_retry_count = 3;
        int ret;
        
        // DB 열기 최대 3회 시도, 재시도 간격 500ms
        while (db_retry < db_retry_count) {
            ret = db_open(db_path, &db);
            if (ret == SQLITE_OK) {
                break;  // 성공 시 루프 종료
            }
            
            // 실패 시 로그 출력 및 카운터 증가
            ESP_LOGW(TAG, "DB 열기 실패 (시도 %d/%d)", db_retry+1, db_retry_count);
            db_retry++;
            
            // 아직 재시도 기회가 남았으면 500ms 대기 후 재시도
            if (db_retry < db_retry_count) {
                vTaskDelay(pdMS_TO_TICKS(500));
            }
        }
        
        // 최대 시도 후에도 DB 열기 실패 시
        if (ret != SQLITE_OK) {
            ESP_LOGE(TAG, "DB 열기 실패: 최대 재시도 횟수 초과");
            // 1초 후 다시 시도 (오류 발생 시 짧은 대기)
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;  // 현재 루프 나머지 부분 건너뛰기
        }
        
        // temperature 테이블 생성
        ret = db_exec(db, "CREATE TABLE IF NOT EXISTS temperature ("
                     "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                     "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP, "
                     "temperature FLOAT);");
        
        // 테이블 생성 실패 시
        if (ret != SQLITE_OK) {
            ESP_LOGE(TAG, "테이블 생성 실패: %s", sqlite3_errmsg(db));
            sqlite3_close(db);  // DB 연결 닫기
            // 1초 후 다시 시도 (오류 발생 시 짧은 대기)
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;  // 현재 루프 나머지 부분 건너뛰기
        }

        // 온도 데이터 삽입을 위한 SQL 문 준비
        sqlite3_stmt *stmt;
        const char *sql = "INSERT INTO temperature (temperature) VALUES (?);";

        // SQL 문 준비
        ret = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
        if (ret != SQLITE_OK) {
            // 준비 실패 시 오류 로그 출력
            ESP_LOGE(TAG, "SQL 문 준비 실패: %s", sqlite3_errmsg(db));
            sqlite3_close(db);  // DB 연결 닫기
            // 1초 후 다시 시도 (오류 발생 시 짧은 대기)
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;  // 현재 루프 나머지 부분 건너뛰기
        }

        // 온도 값을 SQL 문의 ? 위치에 바인딩
        sqlite3_bind_double(stmt, 1, temp);

        // SQL 실행 재시도 로직
        int sql_retry = 0;
        int sql_retry_count = 5;
        
        // SQL 실행 최대 3회 시도, 재시도 간격 500ms
        while (sql_retry < sql_retry_count) {
            ret = sqlite3_step(stmt);  // SQL 실행
            if (ret == SQLITE_DONE) {  // 성공적으로 완료됨
                ESP_LOGI(TAG, "온도 데이터 저장 성공: %.2f°C", temp);
                break;  // 성공 시 루프 종료
            }
            
            // 실패 시 로그 출력 및 카운터 증가
            ESP_LOGW(TAG, "SQL 실행 실패 (시도 %d/%d): %s", 
                sql_retry+1, sql_retry_count, sqlite3_errmsg(db));
            sql_retry++;
            
            // 아직 재시도 기회가 남았으면 500ms 대기 후 재시도
            if (sql_retry < sql_retry_count) {
                vTaskDelay(pdMS_TO_TICKS(500));
            }
        }
        
        // 최대 시도 후에도 SQL 실행 실패 시 로그 출력
        if (ret != SQLITE_DONE) {
            ESP_LOGE(TAG, "SQL 실행 실패: 최대 재시도 횟수 초과");
        }

        // 리소스 정리: SQL 문 및 DB 연결 닫기
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        
        // 정상 처리 후 1초 대기
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
void app_main(void)
{
    // websocket_init이 와이파이 연결까지 수행
    websocket_init();
    
    // 와이파이 연결이후에 sntp 받아와야 되니까 그 뒤에
    sntp_rtc_init();

    // 시간 동기화 후 SQLite 태스크 시작
    // 태스크실행할함수, 태스크이름, 태스크할당스택크기(바이트), 태스크함수에전달할파라미터, 태스크우선순위,태스크핸들을저장할변수
    xTaskCreate(&sqlite, "SQLITE3", 1024 * 6, NULL, 5, NULL);
}