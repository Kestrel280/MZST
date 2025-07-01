#include <stdio.h>

#include <freertos/FreeRTOS.h>
#include "esp_log.h"
#include "driver/gpio.h"

#include "MZSTModuleImpl.h"
#include "Server.h"

#define RF_D0 GPIO_NUM_7
#define RF_D1 GPIO_NUM_44
#define RF_D2 GPIO_NUM_6
#define RF_D3 GPIO_NUM_5
#define RF_D4 GPIO_NUM_4
#define RF_D5 GPIO_NUM_3
#define RF_D6 GPIO_NUM_2
#define RF_D7 GPIO_NUM_1
#define RF_TE GPIO_NUM_43


static const char* TAG = "MZST_TrnsModule";
int ctype = CTYPE_TRNS; // Exported global variable

void initMzstModule() {

    ESP_LOGI(TAG, "initMzstModule() entry");

    /* Speaker configuration */
    gpio_config_t rf_pin_config = {};
    rf_pin_config.intr_type = GPIO_INTR_DISABLE;
    rf_pin_config.mode = GPIO_MODE_OUTPUT;
    rf_pin_config.pin_bit_mask = RF_D0 | RF_D1 | RF_D2 | RF_D3 | RF_D4 | RF_D5 | RF_D6 | RF_D7 | RF_TE;
    rf_pin_config.pull_down_en = 0;
    rf_pin_config.pull_up_en = 0;
    ESP_ERROR_CHECK(gpio_config(&rf_pin_config));

    return;
}

void processCommandSpecific(char* token) {
    /* Fallthrough for processCommandCommon
    Module-specific command handling
    strtok() has already been primed with the full command. Special cases and prefixes have already been handled
    Initial token passed as arg; strtok(NULL, " ") returns pointer to next token
    */

    ESP_LOGI(TAG, "processCommandSpecific with initial token [%s]", token);
}

void feedbackLoop() {
    while (1) {
        ESP_LOGI(TAG, "... feedback loop ...");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

void setColor(int r, int g, int b) {
    return;
}