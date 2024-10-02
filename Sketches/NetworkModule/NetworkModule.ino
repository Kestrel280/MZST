#include <EEPROM.h>
#include <WiFi.h>
#include "../_include/NetworkModule.h"
#include "../_include/Eeprom_Helpers.h"

#define SERIAL_BAUDRATE 115200
#define EEPROM_SIZE 512

NetworkModule module;
WiFiClient client;

const char* HOST = "192.168.1.110";
const uint16_t PORT = 5000;

void setup() {
  // Setup
  Serial.begin(SERIAL_BAUDRATE);
  EEPROM.begin(EEPROM_SIZE);
  sleep(2);
  Serial.printf("--- initialized ---\n");
  dumpEeprom();

  // Load wifi network info
  readEeprom((char*)&module, 0, sizeof(NetworkModule));
  Serial.printf("Read SSID from EEPROM: %s\n", module.networkSsid);
  Serial.printf("Read Password from EEPROM: %s\n", module.networkPassword);

  // Connect to wifi
  WiFi.begin(module.networkSsid, module.networkPassword);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.printf(".");
  }
  Serial.printf("\nWiFi connected with IP: %s\n", WiFi.localIP().toString());

  // Connect to socket
  Serial.printf("Trying to connect to socket at host %s:%d", HOST, PORT);
  while(!client.connect(HOST, PORT)) {
    Serial.printf(".");
    delay(500);
  }
  Serial.printf("\nConnected to socket at host %s:%d\n", HOST, PORT);
}

void loop() {
  long rand;
  Serial.printf("Sending message to host... ");
  client.printf("%d\n", rand = random(100));
  Serial.printf("Sent message %d\n", rand);
  delay(5000);
}
