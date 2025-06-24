#include <stdio.h>

#include <freertos/FreeRTOS.h>
#include <sdkconfig.h>
#include "esp_event.h"
#include "esp_log.h"
#include "esp_sntp.h" /* gettimeofday */

#include "NVSUtil.h"
#include "Server.h"
#include "Secrets.h"
#include "MZSTModuleImpl.h"

extern bool startWifi();

static const char* TAG = "MZST_main";
uint16_t mid;                   // Module id, read from NVS
TaskHandle_t messageLoopTask;   // Handle on message-loop thread
TaskHandle_t feedbackLoopTask;  // Handle on feedback submodule-loop thread
char* serverIp;                 // This ptr is passed to the Server component, so it must have permanent lifetime and the data must not be modified. Simple enough -- we only do one read from the NVS
extern int ctype;               // Specified in Module component implementation file, which is selected by MZST_MODULE_TYPE config option

void nvsSetupDefaults() {
    ESP_LOGI(TAG, "Populating NVS with default values");
    nvsSetStr(NVS_NTWK_SSID_KEY,    NTWK_SSID);
    nvsSetStr(NVS_NTWK_PSWD_KEY,    NTWK_PSWD);
    nvsSetStr(NVS_SERVER_IP_KEY,    SERVER_IP);
    nvsSetInt(NVS_SERVER_PORT_KEY,  5000);
    nvsSetInt(NVS_MODULE_ID_KEY,    9999);
    nvsSetInt(NVS_VERSION_KEY,      1);
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
    if (!nvsGetInt(NVS_VERSION_KEY, &int_out)) nvsSetupDefaults();

    // Initialize module-specific items (mostly hardware/pin setup)
    initMzstModule();

    // Connect to WiFi
    startWifi();

    // Connect to server
    nvsGetInt(NVS_MODULE_ID_KEY, &int_out);
    mid = (uint16_t)int_out;
    nvsGetInt(NVS_SERVER_PORT_KEY, &int_out);
    nvsGetStr(NVS_SERVER_IP_KEY, &serverIp);
    ESP_LOGI(TAG, "Connecting to server at ip %s...", serverIp);
    serverConnect(serverIp, (uint16_t)int_out, ctype, mid);

    // Start message loop with server
    xTaskCreatePinnedToCore(        // Message loop on core 0
        serverMessageLoop,      /* Task function. */
        "Message_Loop",         /* name of task. */
        16384,                  /* Stack size of task */
        &processCommandCommon,  /* parameter of the task */
        0,                      /* priority of the task */
        &messageLoopTask,       /* Task handle to keep track of created task */
        0);                     /* pin task to core 0 */

    // Start feedback submodule loop
    xTaskCreatePinnedToCore(        // Feedback loop on core 1
        feedbackLoop,           /* Task function. */
        "Feedback_Loop",        /* name of task. */
        16384,                  /* Stack size of task */
        NULL,                   /* parameter of the task */
        0,                      /* priority of the task */
        &feedbackLoopTask,      /* Task handle to keep track of created task */
        1);                     /* pin task to core 1 */

    // Register with server
    serverSend(MTYPE_REGISTER_NODE, mid, 1234);

    struct timeval tv_now;
    while (1) {
        gettimeofday(&tv_now, NULL);
        ESP_LOGI(TAG, "i = %5d | NVS SERVER_PORT = %lu | MODULE_ID = %d | NVS SERVER_IP = %s | TIME = %5lli.%6lli", i++, int_out, mid, serverIp, (int64_t)tv_now.tv_sec, (int64_t)tv_now.tv_usec);
        vTaskDelay(2500 / portTICK_PERIOD_MS);
        setColor((i % 3) == 0 ? 255 : 0, ((i+1) % 3) == 0 ? 255 : 0, ((i+2) % 3) == 0 ? 255 : 0);
    }
}