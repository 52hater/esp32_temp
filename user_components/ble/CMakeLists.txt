idf_component_register(
    SRCS "ble.c"
    INCLUDE_DIRS "inc"
    PRIV_INCLUDE_DIRS "${IDF_PATH}/components/bt/include/esp32/include"
    REQUIRES bt esp_http_server nvs_flash main
)