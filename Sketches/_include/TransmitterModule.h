#ifndef TRANSMITTERMODULE_H
#define TRANSMITTERMODULE_H

#include "Module.h"

// Message handler for Transmitter Module-specific messages
void processModuleSpecificMessage(std::string msg);

#define SERIAL_BAUDRATE 921600
#define TRANSMIT_PIN 27

#endif
