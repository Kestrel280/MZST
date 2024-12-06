#include <queue>
#include <EEPROM.h>
#include <WiFi.h>
#include "../_include/NetworkModule.h"
#include "../_include/Eeprom_Helpers.h"
#include "../_include/TimerSyncModule.h"

#include "esp32-hal-touch.h"

#define SERIAL_BAUDRATE 921600
#define EEPROM_SIZE 512
#define NOOP __asm__("nop\n\t");

// Defined types
typedef struct {
    short type;
    short id;
    unsigned long ts;
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
const int rfReceivePin = D9;
const int speakerPin = D0;

// Tasks
TaskHandle_t messageLoopTask;
TaskHandle_t rfListenerTask;

// Message types
const char MTYPE_REGISTER       = 2;
const char MTYPE_TOUCHED        = 100;
const char MTYPE_ERROR          = 33;
const char MTYPE_TIMESTAMPRESET = 66;

// State globals
unsigned short id = 2;
bool touched = false;
unsigned long timestampLastReset = 0; 

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
  pinMode(rfReceivePin, INPUT);
  sleep(2);
  Serial.printf("--- initialized ---\n");
  Serial.printf("  PACKET SIZE: %d\n", sizeof(OutboundMessage));
  dumpEeprom();

  sleep(1);

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

  sendMessage(createOutboundMessage(MTYPE_REGISTER));

  Serial.printf("starting tasks\n");

  // Start message loop on core 0
  xTaskCreatePinnedToCore(
    messageLoop,      /* Task function. */
    "Message_Loop",   /* name of task. */
    16384,            /* Stack size of task */
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
/*     TimeSync Functions    */
/* ************************* */

bool circularBufferMatchesKey(int* buffer, int* key, int startIdx, int length, int allowableMisses = 0) {
  int misses = 0;
  for (int i = 0; i < length; i++) {
    if (buffer[(startIdx + i) % length] != key[i]) {
      misses++;
    }

    if (misses > allowableMisses) { return false; }
  } // End for loop

  return true;
}

/* ************************* */
/*            ISRs           */
/* ************************* */

void touchpadCallback() {
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
  msg.ts = millis() - timestampLastReset;
  return msg;
}

void sendMessage(OutboundMessage msg) {
  socket.write_P((char*)(&msg), sizeof(OutboundMessage));
}

/* ************************* */
/*           Tasks           */
/* ************************* */

void messageLoop(void* param) {

  touch_value_t _touchVal;
  _touchVal = touchRead(TPPIN);
  touchAttachInterrupt(TPPIN, &touchpadCallback, _touchVal * 6 / 5);
  Serial.printf("Baseline tp val = %d; set threshold to %d\n", _touchVal, _touchVal * 8 / 7);

  Serial.printf("Message loop running on core %d\n", xPortGetCoreID());

  while (true) {
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
    
    delay(250);
  }
}

void rfListen(void* param) {
  Serial.printf("rfListen running on core %d\n", xPortGetCoreID());

  int buffer[rfKeyLength]; // Circular buffer to receive incoming signals
  int bufferIdx;
  int j;
  for (j = 0; j < rfKeyLength; j++) {
    buffer[j] = -1;
  }
  
  while(true) {

    buffer[bufferIdx] = digitalRead(rfReceivePin);
    
    // Check if buffer, starting from NEXT value, matches the key
    // If it does, reset timestamp and send a timestamp-reset message to server
    // note: buffer overflows are checked in circularBufferMatchesKey(), don't need to check here
    if (circularBufferMatchesKey(buffer, rfKey, bufferIdx + 1, rfKeyLength, rfKeyAllowableMisses)) {
      outboundMessageQueue.push(createOutboundMessage(MTYPE_TIMESTAMPRESET));
      timestampLastReset = millis();
      Serial.printf("Received timestamp-reset key\n");
    }

    bufferIdx = (bufferIdx + 1) % rfKeyLength;

    //Serial.printf("%d", buffer[bufferIdx]);
    //if (!bufferIdx) { Serial.printf("\n"); }

    delay(rfPulseIntervalMs);
  }
}
