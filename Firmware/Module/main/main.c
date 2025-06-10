#include <sdkconfig.h>
#include <freertos/FreeRTOS.h>
#include <esp_log.h>
#include "NVSUtil.h"
#include "Secrets.h"

#include <stdio.h>

static const char* TAG = "main";

void app_main(void) {
    int i = 0;
    uint32_t int_out;
    char* str_out;
    ESP_LOGV(TAG, "app_main entry");

    // Check if NVS is populated
    nvsInit();

    if (!nvsGetInt("__MZST_MODULE", &int_out)) {
        ESP_LOGI(TAG, "Module does not have data on NVS, populating with defaults from Secrets.h");
        nvsSetStr("NTWK_SSID",      NTWK_SSID);
        nvsSetStr("NTWK_PSWD",      NTWK_PSWD);
        nvsSetStr("SERVER_IP",      SERVER_IP);
        nvsSetInt("SERVER_PORT",    5000);
        nvsSetInt("MODULE_ID",      9999);
        nvsSetInt("__MZST_MODULE",  1);
        nvsCommit();
    }

    while (1) {
        nvsGetInt("SERVER_PORT", &int_out);
        nvsGetStr("SERVER_IP", &str_out);
        ESP_LOGI(TAG, "i = %d | NVS SERVER_PORT = %lu | NVS SERVER_IP = %s", i++, int_out, str_out);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}