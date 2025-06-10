#include <sdkconfig.h>
#include <freertos/FreeRTOS.h>
#include <esp_log.h>

#include <stdio.h>

static const char* TAG = "main";

void app_main(void)
{
    int i = 0;
    ESP_LOGV(TAG, "app_main entry");
    while (1) {
        ESP_LOGI(TAG, "%d", i++);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}