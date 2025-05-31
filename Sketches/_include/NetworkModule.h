#ifndef NETWORKMODULE_H
#define NETWORKMODULE_H

#include <string>
#include "Module.h"

// Message handler for Network Module-specific messages
void processModuleSpecificMessage(std::string iss);

// Hardware definitions and constants
#define SERIAL_BAUDRATE 921600
#define TP_PIN TOUCH_PAD_NUM7
#define RF_D0 42
#define RF_D1 41
#define RF_D2 40
#define RF_D3 39
#define RF_D4 38
#define RF_D5 37
#define RF_D6 36
#define RF_D7 35
#define RF_VT 45
#define LED_RED_PIN 2
#define LED_GREEN_PIN 3
#define LED_BLUE_PIN 4
#define AUDIO_PIN 1
#define AUDIO_PIN_EN 5

void* __color = nullptr;
#define writeLed(pcolor) if(pcolor) { ledcWrite(LED_RED_PIN, (pcolor)->r); ledcWrite(LED_GREEN_PIN, (pcolor)->g); ledcWrite(LED_BLUE_PIN, (pcolor)->b); __color = (void*)pcolor; }

#endif
