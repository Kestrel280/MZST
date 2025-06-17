#include <stdio.h>

#include <freertos/FreeRTOS.h>
#include <sdkconfig.h>
#include "esp_event.h"
#include "esp_log.h"
#include "esp_sntp.h" /* gettimeofday */

#include "NVSUtil.h"
#include "Server.h"
#include "Secrets.h"

extern bool startWifi();

static const char* TAG = "MZST_main";
uint16_t mid;                   // Module id, read from NVS
TaskHandle_t messageLoopTask;   // Handle on message-loop thread
char* serverIp;                 // This ptr is passed to the Server component, so it must have permanent lifetime and the data must not be modified. Simple enough -- we only do one read from the NVS, and one (primary, non-reconnecting) call to serverConnect()

/* TODO these should be extern, declared in a module-specific implementation */
void processCommand(char* cmd) {
    ESP_LOGI(TAG, "processing cmd %s", cmd);
    serverSend(MTYPE_ACK, mid, 0);
}
int ctype = CTYPE_NODE;

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

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    // Load NVS and populate it with defaults if necessary
    nvsInit();
    if (!nvsGetInt("__MZST_MODULE", &int_out)) nvsSetupDefaults();

    // Connect to WiFi
    startWifi();

    // Connect to server
    nvsGetInt("MODULE_ID", &int_out);
    mid = (uint16_t)int_out;
    nvsGetInt("SERVER_PORT", &int_out);
    nvsGetStr("SERVER_IP", &serverIp);
    ESP_LOGI(TAG, "Connecting to server at ip %s...", serverIp);
    serverConnect(serverIp, (uint16_t)int_out, ctype, mid);

    // Start message loop with server
    xTaskCreatePinnedToCore(        // Message loop on core 0
        serverMessageLoop,  /* Task function. */
        "Message_Loop",     /* name of task. */
        16384,              /* Stack size of task */
        &processCommand,    /* parameter of the task */
        0,                  /* priority of the task */
        &messageLoopTask,   /* Task handle to keep track of created task */
        0);                 /* pin task to core 0 */

    // Register with server
    serverSend(MTYPE_REGISTER_NODE, mid, 1234);

    struct timeval tv_now;
    while (1) {
        gettimeofday(&tv_now, NULL);
        ESP_LOGI(TAG, "i = %5d | NVS SERVER_PORT = %lu | MODULE_ID = %d | NVS SERVER_IP = %s | TIME = %5lli.%6lli", i++, int_out, mid, serverIp, (int64_t)tv_now.tv_sec, (int64_t)tv_now.tv_usec);
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}