#include <stdio.h>

#include "esp_log.h"

#include "MZSTModuleImpl.h"
#include "Server.h"

static const char* TAG = "MZST_TrnsModule";
int ctype = CTYPE_TRNS; // Exported global variable

void processCommand(char* cmd) {
    ESP_LOGI(TAG, "TRNS module processing cmd %s", cmd);
    serverSend(MTYPE_ACK, 123, 0);
}