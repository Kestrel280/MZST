#include <string>
#include <sstream>
#include <queue>
#include <EEPROM.h>
#include <WiFi.h>

#include "../_include/TransmitterModule.h"
#include "../_include/Server.h"
#include "../_include/Eeprom_Helpers.h"
#include "../_include/TimerSyncModule.h"

// Hardware constants
const int transmitPin = 27;

void setup() {
  Serial.begin(SERIAL_BAUDRATE);
  EEPROM.begin(EEPROM_SIZE);
  dumpEeprom();
  readEeprom((char*)&module, 0, sizeof(Module)); // Initialize module by loading from EEPROM
  pinMode(transmitPin, OUTPUT);

  // Connect to wifi
  WiFi.begin(module.networkSsid, module.networkPassword);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf(".");
    vTaskDelay(250);
  }
  Serial.printf("\nWiFi connected with IP: %s\n", WiFi.localIP().toString().c_str());

  sleep(2); // Pause a couple seconds... gives us time to restart/update the server if we just reset from a SHUTDOWN message

  // Connect to server; while connecting, if we receive an admin connection, accept it and process whatever message it has for us
  Serial.printf("Trying to connect to socket at host %s:%d", module.serverIp, module.serverPort);
  adminSocketListener = WiFiServer(ADMIN_PORT);
  adminSocketListener.begin();
  while(!serverSocket.connect(module.serverIp, module.serverPort)) {
    Serial.printf(".");

    // Listen for admin connections 
    adminSocket = adminSocketListener.available();
    if (adminSocket) {
      Serial.printf("\nReceived admin-socket connection\n");
      while (adminSocket.connected()) {
        std::string line = std::string(adminSocket.readStringUntil('\n').c_str());
        processIncomingMessage(line);
      }
      adminSocket.stop();
      Serial.printf("\nDisconnected from admin-socket, continuing to attempt connection to server\n");
    }

    vTaskDelay(250);
  }
  Serial.printf("\nConnected to server at host %s:%d\n", module.serverIp, module.serverPort);
  adminSocketListener.close();
  sendMessage(createOutboundMessage(MTYPE_REGISTER_TRNS, 0));
  Serial.printf("\nRegistered to server\n");
}

void loop() {
  while(!outboundMessageQueue.empty()) {
    OutboundMessage msg = outboundMessageQueue.front();
    Serial.printf("Sending message to server with type: %d\n", msg.type);
    sendMessage(msg);
    outboundMessageQueue.pop();
  }

  while (serverSocket.available() > 0) {
    std::string line = std::string(serverSocket.readStringUntil('\n').c_str());
    processIncomingMessage(line);
  }
}

void processModuleSpecificMessage(std::string msg) {
  std::stringstream iss(msg);
  std::string command, value;

  std::getline(iss, command, ' '); // Read the first word of the message stream into 'command'
  
  if (command == "TRANSMIT") {
    transmit();
  }
}

void transmit() {
  unsigned int mask = 0b10000000000000000000000000000000;
  unsigned int val;
  Serial.printf("Broadcasting...");
  for (int i = 0; i < 32; i++) {
    val = (mask & rfKey) > 0;
    digitalWrite(transmitPin, val); // Output the current array value to the pin
    Serial.printf("%d", val);
    mask = mask >> 1;
    esp_rom_delay_us(rfPulseIntervalUs + RECEIVE_LOOP_TIME_US); // (busy wait) Delay for the pulse duration + account for receiver lag
  }
  digitalWrite(transmitPin, LOW);
  Serial.printf(" Done\n");
}