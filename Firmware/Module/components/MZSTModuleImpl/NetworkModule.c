#include <stdio.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "driver/ledc.h"
#include "driver/touch_sensor.h"

#include "MZSTModuleImpl.h"
#include "Server.h"

#define LEDC_TIMER_RESOLUTION LEDC_TIMER_8_BIT
#define SPEAKER_PIN GPIO_NUM_1
#define SPEAKER_PIN_EN GPIO_NUM_5
#define LED_PIN_R 3
#define LED_PIN_G 2
#define LED_PIN_B 4
#define TOUCHPAD_PIN TOUCH_PAD_NUM7

extern uint16_t mid;    // main.c, loaded from NVS
static const int ledcCountsPerCycle = (2 << (LEDC_TIMER_RESOLUTION - 1));
static const char* TAG = "MZST_NtwkModule";
int ctype = CTYPE_NODE; // Exported global variable

void touchpadIsr();

void initMzstModule() {

    ESP_LOGI(TAG, "initMzstModule() entry");

    /* Speaker configuration */
    gpio_config_t speaker_config = {};
    speaker_config.intr_type = GPIO_INTR_DISABLE;
    speaker_config.mode = GPIO_MODE_OUTPUT;
    speaker_config.pin_bit_mask = SPEAKER_PIN | SPEAKER_PIN_EN; // TODO will need to separate these eventually
    speaker_config.pull_down_en = 0;
    speaker_config.pull_up_en = 0;
    ESP_ERROR_CHECK(gpio_config(&speaker_config));
    gpio_set_level(SPEAKER_PIN, 0);
    gpio_set_level(SPEAKER_PIN_EN, 0);

    /* LEDC Configuration */
    ledc_timer_config_t ledc_timer = {              // Single timer for all LEDs
        .speed_mode         = LEDC_LOW_SPEED_MODE,
        .duty_resolution    = LEDC_TIMER_RESOLUTION,
        .timer_num          = LEDC_TIMER_0,
        .freq_hz            = 4000,
        .clk_cfg            = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {          // Template for instantiating channels for each LED
        .speed_mode         = LEDC_LOW_SPEED_MODE,
        .channel            = LEDC_CHANNEL_0,
        .timer_sel          = LEDC_TIMER_0,
        .intr_type          = LEDC_INTR_DISABLE,
        .gpio_num           = 0,
        .duty               = 0,                    // PWM duty cycle corresponds to channel intensity, start at 0
        .hpoint             = 0
    };

    ledc_channel.channel = LEDC_CHANNEL_0;
    ledc_channel.gpio_num = LED_PIN_R;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    ledc_channel.channel = LEDC_CHANNEL_1;
    ledc_channel.gpio_num = LED_PIN_G;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    
    ledc_channel.channel = LEDC_CHANNEL_2;
    ledc_channel.gpio_num = LED_PIN_B;
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    /* Touchpad configuration */
    uint32_t tpval;
    ESP_ERROR_CHECK(touch_pad_init());
    ESP_ERROR_CHECK(touch_pad_config(TOUCHPAD_PIN));
    ESP_ERROR_CHECK(touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER));
    ESP_ERROR_CHECK(touch_pad_fsm_start());
    ESP_ERROR_CHECK(touch_pad_read_raw_data(TOUCHPAD_PIN, &tpval));
    ESP_ERROR_CHECK(touch_pad_set_thresh(TOUCHPAD_PIN, tpval * 9 / 8));
    ESP_ERROR_CHECK(touch_pad_isr_register(touchpadIsr, NULL, TOUCH_PAD_INTR_MASK_ALL));
    ESP_ERROR_CHECK(touch_pad_intr_enable(TOUCH_PAD_INTR_MASK_ACTIVE | TOUCH_PAD_INTR_MASK_TIMEOUT));
    ESP_LOGI(TAG, "Touchpad configured with threshold %lu", tpval * 9 / 8);

    return;
}

void processCommand(char* cmd) {
    /* Nominally entered by Server's messageLoop()
    cmd should be a SINGLE, null-terminated command
        (note that the server sends newline-terminated commands;
        but by the time the command reaches this function,
        the newline should have been replaced with a null byte)
    */
    char* token;
    ESP_LOGI(TAG, "NTWK module processing cmd %s", cmd);

    // No while-loop to process commands: input is already a well-formed single command
    // strtok() extracts the first space-separated token from the command
    //  The space is replaced with a null byte, so token is a usable null-terminated string
    //  The value of cmd is advanced to point to the start of the NEXT token
    token = strtok(cmd, " ");

    if (strcmp(token, "TEST") == 0) {
        ESP_LOGI(TAG, "TEST received");
    }

    serverSend(MTYPE_ACK, 123, 0);
    uint32_t tpval;
    ESP_ERROR_CHECK(touch_pad_read_raw_data(TOUCHPAD_PIN, &tpval));
    ESP_LOGI(TAG, "tpval: %lu", tpval);
}

void setColor(int r, int g, int b) {
    // TODO possibly check for initMzstModule() called first

    ESP_LOGI(TAG, "setColor r = %d, g = %d, b = %d", r, g, b);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, r * 255 / ledcCountsPerCycle);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, g * 255 / ledcCountsPerCycle);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, b * 255 / ledcCountsPerCycle);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2);

    return;
}

void touchpadIsr() {
    serverSend(MTYPE_TOUCHED, mid, 123); // TODO serverSend can, in theory, block; might want a trySend() or something
}