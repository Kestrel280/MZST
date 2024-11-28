#include <queue>
#include <EEPROM.h>
#include <WiFi.h>
#include "../_include/NetworkModule.h"
#include "../_include/Eeprom_Helpers.h"

#include "soc/touch_sensor_channel.h"
#include "driver/touch_sensor.h"
#include "driver/touch_sensor_common.h"
#include "hal/touch_sensor_types.h"

#define SERIAL_BAUDRATE 115200
#define EEPROM_SIZE 512

// Defined types
typedef struct {
    short type;
    short id;
    unsigned long timestamp;
} OutboundMessage;

// Prototypes
OutboundMessage createOutboundMessage(char type);
void processIncomingMessage(String msg);
void touchpadCallback(void* arg);
void sendMessage(OutboundMessage msg);

// Hardware constants
const int CAPSENS_PIN = 33;
const touch_pad_t TPPIN = TOUCH_PAD_NUM8;

// Message types
const char MTYPE_TOUCHED  = 100;
const char MTYPE_ERROR    = 33;

// State globals
unsigned short id = 2;
bool touched = false;

// Other globals
NetworkModule module;
WiFiClient socket;
std::queue<OutboundMessage> outboundMessageQueue;

// Constants (TODO should maybe be moved to EEPROM)
const char* HOST = "192.168.1.109";
const uint16_t PORT = 5000;

void setup() {
  // Setup
  Serial.begin(SERIAL_BAUDRATE);
  EEPROM.begin(EEPROM_SIZE);
  sleep(2);
  Serial.printf("--- initialized ---\n");
  Serial.printf("  PACKET SIZE: %d\n", sizeof(OutboundMessage));
  dumpEeprom();

  // Touchpad initialization
  uint16_t tpBaseline;
  uint16_t tpThresh;
  ESP_ERROR_CHECK(touch_pad_init());
  touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
  touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
  touch_pad_config(TPPIN, 0);
  touch_pad_read(TPPIN, &tpBaseline);
  tpThresh = tpBaseline * 2 / 3;
  ESP_ERROR_CHECK(touch_pad_set_thresh(TPPIN, tpThresh));
  Serial.printf("Baseline tp val = %d; set threshold to %d\n", tpBaseline, tpThresh);

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
  while(!socket.connect(HOST, PORT)) {
    Serial.printf(".");
    delay(50);
  }
  Serial.printf("\nConnected to socket at host %s:%d\n", HOST, PORT);

  // Register interrupts for touchpad
  touch_pad_isr_register(touchpadCallback, NULL);
  ESP_ERROR_CHECK(touch_pad_intr_enable());
}

void loop() {
  delay(250);

  while(!outboundMessageQueue.empty()) {
    OutboundMessage msg = outboundMessageQueue.front();
    Serial.printf("Sending message to server with type: %d\n", msg.type);
    sendMessage(msg);
    outboundMessageQueue.pop();
  }

  while (socket.available() > 0) {
    String line = socket.readStringUntil('\n');
    processIncomingMessage(line);
  }
}

/* Function definitions */

void touchpadCallback(void* arg) {
  if (!touched) {
    touched = true;
    outboundMessageQueue.push(createOutboundMessage(MTYPE_TOUCHED));
  }
}

void processIncomingMessage(String msg) {
  Serial.printf("Received message from server: %s\n", msg.c_str());
  //for (char c : msg) { Serial.printf("%x ", c); }

  if (msg == "RESET") {
    touched = false;
  }
}

OutboundMessage createOutboundMessage(char type) {
  OutboundMessage msg;
  msg.type = type;
  msg.id = id;
  msg.timestamp = millis();
  return msg;
}

void sendMessage(OutboundMessage msg) {
  socket.write_P((char*)(&msg), sizeof(OutboundMessage));
}
