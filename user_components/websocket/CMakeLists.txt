idf_component_register(SRCS "web.c" "web_server.c" "web_client.c"
  INCLUDE_DIRS "include"
  EMBED_FILES "html/error.html"
              "html/favicon.ico"
              "html/main.js"
              "html/root.html"
  REQUIRES sqlite json esp_wifi nvs_flash sdcard esp_common ble config)
