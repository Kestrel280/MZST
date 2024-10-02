#include <EEPROM.h>
#include "../_include/NetworkModule.h"
#include "../_include/Eeprom_Helpers.h"

#define SERIAL_BAUDRATE 115200
#define EEPROM_SIZE 512

void setup() {
  Serial.begin(SERIAL_BAUDRATE);
  EEPROM.begin(EEPROM_SIZE);

  sleep(2);

  Serial.printf("--- initialized ---\n");

  dumpEeprom();

  NetworkModule module;
  readEeprom((char*)&module, 0, sizeof(NetworkModule));
  Serial.printf("Read SSID from EEPROM: %s\n", module.networkSsid);
  Serial.printf("Read Password from EEPROM: %s\n", module.networkPassword);

  // Connect to wifi
  //while (!connectToWifi()) {};
}

void loop() {
  sleep(1);
}

bool connectToWifi() {
  //Wifi.begin()
  return false;
}
