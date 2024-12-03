#include <queue>
#include <EEPROM.h>
#include <WiFi.h>
#include "../_include/NetworkModule.h"
#include "../_include/Eeprom_Helpers.h"

#include "soc/touch_sensor_channel.h"
#include "driver/touch_sensor.h"
#include "driver/touch_sensor_common.h"
#include "hal/touch_sensor_types.h"

#define SERIAL_BAUDRATE 921600
#define EEPROM_SIZE 512
#define NOOP __asm__("nop\n\t");

// Defined types
typedef struct {
    short type;
    short id;
    unsigned long timestamp;
} OutboundMessage;

// Prototypes
OutboundMessage createOutboundMessage(char type);
void processIncomingMessage(String msg);
void touchpadCallback(void* param);
void sendMessage(OutboundMessage msg);
void rfListen(void* param);
void messageLoop(void* param);

// Hardware constants
const touch_pad_t TPPIN = TOUCH_PAD_NUM7;

// Tasks
TaskHandle_t messageLoopTask;
TaskHandle_t rfListenerTask;

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
  uint32_t tpBaseline;
  uint32_t tpThresh;
  ESP_ERROR_CHECK(touch_pad_init());
  touch_pad_config(TPPIN);
  touch_pad_set_voltage(TOUCH_PAD_HIGH_VOLTAGE_THRESHOLD, TOUCH_PAD_LOW_VOLTAGE_THRESHOLD, TOUCH_PAD_ATTEN_VOLTAGE_THRESHOLD);
  touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
  touch_pad_fsm_start();
  sleep(1);
  touch_pad_read_raw_data(TPPIN, &tpBaseline);
  tpThresh = tpBaseline * 4 / 3;
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
  touch_pad_isr_register(touchpadCallback, NULL, TOUCH_PAD_INTR_MASK_ACTIVE);
  ESP_ERROR_CHECK(touch_pad_intr_enable(TOUCH_PAD_INTR_MASK_ACTIVE));

  Serial.printf("starting tasks\n");
  // Start message loop on core 0
  xTaskCreatePinnedToCore(
    messageLoop,      /* Task function. */
    "Message_Loop",   /* name of task. */
    4096,             /* Stack size of task */
    NULL,             /* parameter of the task */
    0,                /* priority of the task */
    &messageLoopTask, /* Task handle to keep track of created task */
    0);               /* pin task to core 0 */
  Serial.printf("started message loop\n");

  // Start RF receiver on core 1
  xTaskCreatePinnedToCore(
    rfListen,         /* Task function. */
    "RF_Listen",      /* name of task. */
    4096,             /* Stack size of task */
    NULL,             /* parameter of the task */
    0,                /* priority of the task */
    &rfListenerTask,  /* Task handle to keep track of created task */
    1);               /* pin task to core 1 */
  Serial.printf("started rf receiver\n");
  sleep(2);
}

void loop() {

}

/* ************************* */
/*            ISRs           */
/* ************************* */

void touchpadCallback(void* param) {
  if (!touched) {
    touched = true;
    outboundMessageQueue.push(createOutboundMessage(MTYPE_TOUCHED));
  }
}

/* ************************* */
/*    Messaging Functions    */
/* ************************* */

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

/* ************************* */
/*           Tasks           */
/* ************************* */

void messageLoop(void* param) {
  Serial.printf("Message loop running on core %d\n", xPortGetCoreID());
  
  while (true) {
    delay(50);

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
}

void rfListen(void* param) {
  Serial.printf("rfListen running on core %d\n", xPortGetCoreID());
  while(true) {
    NOOP;
  }
}
