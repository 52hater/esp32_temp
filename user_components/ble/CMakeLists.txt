idf_component_register(
    SRCS "ble.c"
    INCLUDE_DIRS "inc"
    REQUIRES bt esp_http_server nvs_flash main config
)