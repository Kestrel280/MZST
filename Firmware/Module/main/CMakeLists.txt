idf_component_register(SRCS "main.c" "WiFi.c"
                    INCLUDE_DIRS "."
                    REQUIRES AdminListener NVSUtil Server MZSTModuleImpl esp_wifi esp_event esp_netif lwip)