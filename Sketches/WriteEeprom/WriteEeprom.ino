#include <EEPROM.h>
#include "../_include/NetworkModule.h"
#include "../_include/_Networks.h"
#include "../_include/Eeprom_Helpers.h"

NetworkModule _SamHome = {
  .networkSsid = __SAM_NETWORK_SSID,
  .networkPassword = __SAM_NETWORK_PASSWORD
};

void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  Serial.printf("Flashing EEPROM...\n");

  /* ------------------------ */
  /* ------------------------ */
  auto dataToFlash = _SamHome;
  /* ------------------------ */
  /* ------------------------ */

  int i;
  for (i = 0; (i < sizeof(dataToFlash)) && (i < EEPROM_SIZE); i++) { EEPROM.write(i, *((char*)&dataToFlash + i)); }
  for (; i < EEPROM_SIZE; i++) { EEPROM.write(i, 0); }
  EEPROM.commit();
  
  Serial.printf("Flashed EEPROM\n");

  dumpEeprom();

  Serial.printf("Verify that EEPROM was flashed correctly and then reload board with correct program\n");
}

void loop() { }