idf_component_register(SRCS "main.c" "WiFi.c"
                    INCLUDE_DIRS "."
                    REQUIRES NVSUtil esp_wifi esp_event esp_netif)