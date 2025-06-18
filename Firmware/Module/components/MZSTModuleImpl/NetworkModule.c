#include <stdio.h>

#include "esp_log.h"

#include "MZSTModuleImpl.h"
#include "Server.h"

static const char* TAG = "MZST_NtwkModule";
int ctype = CTYPE_NODE; // Exported global variable

void processCommand(char* cmd) {
    ESP_LOGI(TAG, "NTWK module processing cmd %s", cmd);
    serverSend(MTYPE_ACK, 123, 0);
}