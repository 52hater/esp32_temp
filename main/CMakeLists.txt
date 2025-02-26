set(srcs "main.c" "hw/hw.c" "ap/ap.c" "rtc/rtc.c")

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS "." "hw" "common" "ap" "rtc"
                       REQUIRES sdcard ds18b20 sqlite websocket
                       WHOLE_ARCHIVE)
