#ifndef UTILS_H
#define UTILS_H

#include "Hardware.h"

#define currentTimeAbs() esp_timer_get_time()
#define currentTime() (currentTimeAbs() - timestampLastResetUs)
#define writeLed(pcolor) ledcWrite(LED_RED_PIN, (pcolor)->r); ledcWrite(LED_GREEN_PIN, (pcolor)->g); ledcWrite(LED_BLUE_PIN, (pcolor)->b)

#endif
