#include <stdio.h>

#include <freertos/FreeRTOS.h>
#include <sdkconfig.h>
#include "esp_event.h"
#include "esp_log.h"

#include "NVSUtil.h"
#include "Secrets.h"

extern bool startWifi();

static const char* TAG = "main";

void nvsSetupDefaults() {
    ESP_LOGI(TAG, "Populating NVS with default values");
    nvsSetStr("NTWK_SSID",      NTWK_SSID);
    nvsSetStr("NTWK_PSWD",      NTWK_PSWD);
    nvsSetStr("SERVER_IP",      SERVER_IP);
    nvsSetInt("SERVER_PORT",    5000);
    nvsSetInt("MODULE_ID",      9999);
    nvsSetInt("__MZST_MODULE",  1);
    nvsCommit();
}

void app_main(void) {
    int i = 0;
    uint32_t int_out;
    char* str_out;
    ESP_LOGV(TAG, "app_main entry");

    // Start event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Load NVS and populate it with defaults if necessary
    nvsInit();
    if (!nvsGetInt("__MZST_MODULE", &int_out)) nvsSetupDefaults();

    // Connect to WiFi
    startWifi();

    while (1) {
        nvsGetInt("SERVER_PORT", &int_out);
        nvsGetStr("SERVER_IP", &str_out);
        ESP_LOGI(TAG, "i = %d | NVS SERVER_PORT = %lu | NVS SERVER_IP = %s", i++, int_out, str_out);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}