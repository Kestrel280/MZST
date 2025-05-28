#include <EEPROM.h>
#include "../_include/Module.h"
#include "../_include/Eeprom_Helpers.h"

void setup() {
  Serial.begin(921600);
  EEPROM.begin(EEPROM_SIZE);
  Serial.printf("Flashing EEPROM...\n");

  memset(&module, 0, sizeof(module)); // 0-out the entire struct. (There are ways to do this implicitly, but I want to be explicit)
  
  // cpp documentation suggests that strncpy with count > sizeof(src) is ub; but the given examples do exactly that and seem to work
  strncpy((module).networkSsid,      "Network SSID",     sizeof(module.networkSsid));
  strncpy((module).networkPassword,  "Network Password", sizeof(module.networkPassword));
  strncpy((module).serverIp,         "192.168.1.1",      sizeof(module.serverIp));
  module.serverPort = (unsigned short) 5000;
  module.moduleId = (unsigned short) 0;

  wipeEeprom();
  writeEeprom((char*)&module, sizeof(module), 0);
  
  Serial.printf("Flashed EEPROM\n");

  Serial.printf("Verify that EEPROM was flashed correctly and then reload board with correct program\n");
  sleep(2);
  dumpEeprom();
}

void loop() { }