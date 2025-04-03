set(srcs "main.c" "hw/hw.c" "app/app.c" "rtc/rtc.c")

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS "." "hw" "app" "rtc"
                       REQUIRES sdcard ds18b20 sqlite websocket ble config bt
                       WHOLE_ARCHIVE)
