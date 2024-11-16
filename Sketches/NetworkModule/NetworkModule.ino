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

const int CAPSENS_PIN = 33;
const touch_pad_t TPPIN = TOUCH_PAD_NUM8;

NetworkModule module;
WiFiClient client;
std::queue<String> outboundMessageQueue;

const char* HOST = "192.168.1.100";
const uint16_t PORT = 5000;
uint16_t tpBaseline;
uint16_t tpThresh;

bool touched = false;

void touchpadCallback(void* arg) {
  noInterrupts();
  if (!touched) {
    touched = true;
    outboundMessageQueue.push("TRIGGERED");
  }
}

void setup() {
  // Setup
  Serial.begin(SERIAL_BAUDRATE);
  EEPROM.begin(EEPROM_SIZE);
  sleep(2);
  Serial.printf("--- initialized ---\n");
  dumpEeprom();

  // Touchpad initialization
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
  while(!client.connect(HOST, PORT)) {
    Serial.printf(".");
    delay(50);
  }
  Serial.printf("\nConnected to socket at host %s:%d\n", HOST, PORT);

  // Register interrupts for touchpad
  touch_pad_isr_register(touchpadCallback, NULL);
  ESP_ERROR_CHECK(touch_pad_intr_enable());
}

void loop() {
  uint16_t val;
  ESP_ERROR_CHECK(touch_pad_read(TPPIN, &val));
  //Serial.printf("Touchpad val is %d (thresh = %d)\n", val, tpThresh);
  delay(250);

  while(!outboundMessageQueue.empty()) {
    Serial.printf("Sending message to server: %s\n", outboundMessageQueue.front());
    client.println(outboundMessageQueue.front());
    outboundMessageQueue.pop();
  }

  while (client.available() > 0) {
    String line = client.readStringUntil('\n');
    processIncomingMessage(line);
  }
}

void processIncomingMessage(String msg) {
  Serial.printf("Received message from server: %s\n", msg);
  //for (char c : msg) {
  //  Serial.printf("%x ", c);
  //}
  //Serial.println();
  if (msg == "RESET") {
    touched = false;
  }
}