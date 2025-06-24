#include <stdio.h>

#include "esp_log.h"

static const char* TAG = "MZST_CommonModule";

void processCommandCommon(char* cmd) {
    ESP_LOGI(TAG, "TRNS module processing cmd %s", cmd);
}