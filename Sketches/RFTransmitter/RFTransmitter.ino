#include <string>
#include <sstream>
#include <queue>
#include <EEPROM.h>
#include <WiFi.h>

#include "../_include/TransmitterModule.h"
#include "../_include/Eeprom_Helpers.h"
#include "../_include/TimerSyncModule.h"

unsigned short id = 5;            // ID of the transmitter. TODO Move this to EEPROM

struct OutboundMessage {
  short type;
  short id;
  uint32_t data;
};

// Prototypes
OutboundMessage createOutboundMessage(char type, uint32_t data);
void processIncomingMessage(std::string msg);
void sendMessage(OutboundMessage msg);

// Hardware constants
const int transmitPin = 27;

// Constants (TODO should maybe be moved to EEPROM)
const char* HOST = "192.168.1.111";
const uint16_t PORT = 5000;

// Message types
const char MTYPE_REGISTER       = 3;    // Registration code for transmitters
const char MTYPE_ACK            = 111;

// Other globals
TransmitterModule module;
WiFiClient socket;
std::queue<OutboundMessage> outboundMessageQueue;

void setup() {
  Serial.begin(921600);
  EEPROM.begin(EEPROM_SIZE);
  readEeprom((char*)&module, 0, sizeof(TransmitterModule)); // Load EEPROM
  pinMode(transmitPin, OUTPUT);

  // Connect to wifi
  WiFi.begin(module.networkSsid, module.networkPassword);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf(".");
    vTaskDelay(250);
  }
  Serial.printf("\nWiFi connected with IP: %s\n", WiFi.localIP().toString().c_str());

  // Connect to socket
  Serial.printf("Trying to connect to socket at host %s:%d", HOST, PORT);
  while(!socket.connect(HOST, PORT)) {
    Serial.printf(".");
    vTaskDelay(250);
  }

  // Register to server
  sendMessage(createOutboundMessage(MTYPE_REGISTER, 0));
  Serial.printf("\nRegistered to server\n");
}

void loop() {
  while(!outboundMessageQueue.empty()) {
    OutboundMessage msg = outboundMessageQueue.front();
    Serial.printf("Sending message to server with type: %d\n", msg.type);
    sendMessage(msg);
    outboundMessageQueue.pop();
  }

  while (socket.available() > 0) {
    std::string line = std::string(socket.readStringUntil('\n').c_str());
    processIncomingMessage(line);
  }
}

void processIncomingMessage(std::string msg) {
  Serial.printf("Received message from server: %s\n", msg.c_str());
  //for (char c : msg) { Serial.printf("%x ", c); }

  std::stringstream iss(msg);
  std::string word;

  std::getline(iss, word, ' '); // Read the first word of the message into 'word'

  if ((word == "RESTART") || (word == "SHUTDOWN")) {
    Serial.printf("Received %s message! Restarting!", word.c_str());
    esp_restart();
  }

  if (word == "REQ_ACK") { // Server is requesting an ACK; send it, then continue
    outboundMessageQueue.push(createOutboundMessage(MTYPE_ACK, 0));
    std::getline(iss, word, ' ');
  }

  if (word == "TRANSMIT") {
    transmit();
  }
}

OutboundMessage createOutboundMessage(char type, unsigned long data) {
  OutboundMessage msg;
  msg.type = type;
  msg.id = id;
  msg.data = data;
  return msg;
}

void sendMessage(OutboundMessage msg) {
  socket.write_P((char*)(&msg), sizeof(OutboundMessage));
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
    delay(rfPulseIntervalMs); // Delay for the pulse duration
  }
  Serial.printf(" Done\n");
}