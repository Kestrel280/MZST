#include <stdio.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include "esp_log.h"
#include "esp_sntp.h" /* gettimeofday */
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
#define PUSH_BUTTON GPIO_NUM_9
#define DEBUG_LED GPIO_NUM_8

struct timeval timeLastTransmit;
static const char* TAG = "MZST_TrnsModule";
int ctype = CTYPE_TRNS; // Exported global variable
static char stamp = 0b0; // Data stamp, increments with every transmit

void transmit(char data);
static void IRAM_ATTR debugButtonCallback(void* args) {
    transmit(++stamp);
}

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

    /* Push button configuration */
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_set_intr_type(PUSH_BUTTON, GPIO_INTR_POSEDGE));
    ESP_ERROR_CHECK(gpio_isr_handler_add(PUSH_BUTTON, debugButtonCallback, NULL));
    ESP_ERROR_CHECK(gpio_input_enable(PUSH_BUTTON));

    return;
}

void processCommandSpecific(char* token) {
    /* Fallthrough for processCommandCommon
    Module-specific command handling
    strtok() has already been primed with the full command. Special cases and prefixes have already been handled
    Initial token passed as arg; strtok(NULL, " ") returns pointer to next token
    */
    ESP_LOGI(TAG, "processCommandSpecific with initial token [%s]", token);

    if (strcmp(token, "TRANSMIT") == 0) {
        transmit(++stamp);
        strtok(NULL, " ");
    }
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

void transmit(char data) {
    gettimeofday(&timeLastTransmit, NULL);
    gpio_set_level(RF_D0, (stamp & 0b10000000) != 0);
    gpio_set_level(RF_D1, (stamp & 0b01000000) != 0);
    gpio_set_level(RF_D2, (stamp & 0b00100000) != 0);
    gpio_set_level(RF_D3, (stamp & 0b00010000) != 0);
    gpio_set_level(RF_D4, (stamp & 0b00001000) != 0);
    gpio_set_level(RF_D5, (stamp & 0b00000100) != 0);
    gpio_set_level(RF_D6, (stamp & 0b00000010) != 0);
    gpio_set_level(RF_D7, (stamp & 0b00000001) != 0);
    gpio_set_level(RF_TE, 1);
    gpio_set_level(RF_TE, 0);
    ESP_EARLY_LOGI(TAG, "Transmitted stamp 0x%x at timestamp %6lli.%6lli\n", stamp, (int64_t)timeLastTransmit.tv_sec, (int64_t)timeLastTransmit.tv_usec);
}