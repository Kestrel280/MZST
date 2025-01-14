#include <EEPROM.h>
#include "../_include/NetworkModule.h"
#include "../_include/Eeprom_Helpers.h"

void setup() {
  Serial.begin(921600);
  EEPROM.begin(EEPROM_SIZE);
  Serial.printf("Flashing EEPROM...\n");

  NetworkModule networkModule;
  memset(&networkModule, 0, sizeof(NetworkModule)); // 0-out the entire struct. (There are ways to do this implicitly, but I want to be explicit)
  
  // cpp documentation suggests that strncpy with count > sizeof(src) is ub; but the given examples do exactly that and seem to work
  strncpy((networkModule).networkSsid,      "Network SSID",     sizeof(networkModule.networkSsid));
  strncpy((networkModule).networkPassword,  "Network Password", sizeof(networkModule.networkPassword));
  strncpy((networkModule).serverIp,         "192.168.1.1",      sizeof(networkModule.serverIp));
  networkModule.serverPort = (unsigned short) 5000;
  networkModule.moduleId = (unsigned short) 0;

  wipeEeprom();
  writeEeprom((char*)&networkModule, sizeof(NetworkModule), 0);
  
  Serial.printf("Flashed EEPROM\n");

  Serial.printf("Verify that EEPROM was flashed correctly and then reload board with correct program\n");
  sleep(2);
  dumpEeprom();
}

void loop() { }