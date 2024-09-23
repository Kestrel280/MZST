#include <EEPROM.h>
#include "../_include/NetworkModule.h"
#include "../_include/_Networks.h"

NetworkModule _SamHome = {
  .networkSsid = __SAM_NETWORK_SSID,
  .networkPassword = __SAM_NETWORK_PASSWORD
};

void setup() {
  Serial.printf("");
}

void loop() {
  // put your main code here, to run repeatedly:

}
