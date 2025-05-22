#ifndef NETWORKMODULE_H
#define NETWORKMODULE_H

#include <string>
#include "Module.h"

// Message handler for Network Module-specific messages
void processModuleSpecificMessage(std::string iss);

// Hardware definitions and constants
#define SERIAL_BAUDRATE 921600
#define TP_PIN TOUCH_PAD_NUM7
#define RF_RECEIVE_PIN D9
#define LED_RED_PIN D2
#define LED_GREEN_PIN D3
#define LED_BLUE_PIN D1
#define SPEAKER_PIN D0

#define writeLed(pcolor) ledcWrite(LED_RED_PIN, (pcolor)->r); ledcWrite(LED_GREEN_PIN, (pcolor)->g); ledcWrite(LED_BLUE_PIN, (pcolor)->b)

#endif
