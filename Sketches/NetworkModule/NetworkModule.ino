#include <string>
#include <sstream>
#include <queue>
#include <EEPROM.h>
#include <WiFi.h>
#include "../_include/NetworkModule.h"
#include "../_include/Eeprom_Helpers.h"
#include "../_include/TimerSyncModule.h"

#include "esp_system.h"
#include "esp32-hal-touch.h"

#define SERIAL_BAUDRATE 921600
#define NOOP __asm__("nop\n\t");

unsigned short id = 5;            // ID of the node. TODO Move this to EEPROM

typedef enum _State {
  INIT_BootStart,             // State set immediately upon module boot
  INIT_WaitingWifi,           // While waiting for wifi connection
  INIT_WaitingServer,         // While waiting for server connection
  INIT_Complete,              // Connected to server, awaiting instructions
  READYRUN_StartNode,         // Starting node for a course. When it's touched, server will move into RUN state
  READYRUN_NotPartOfCourse,   // In ready/run mode: the node IS NOT part of the active course
  READYRUN_NoTriggersDone,    // In ready/run mode: the node is part of the active course, but has not yet been triggered a single time
  READYRUN_SomeTriggersDone,  // In ready/run mode: the node is part of the active course, and has been triggered, but the node re-appears later in the course so it will need to be triggered again
  READYRUN_AllTriggersDone,   // In ready/run mode: the node IS part of the active course, and has been triggered, and does not appear later in the course
  DEFINE_SelectedNode,        // In edit mode: the most-recently selected node
  DEFINE_NotInCourse,         // In edit mode: a node which has not been added to the course, but is able to be added
  DEFINE_InCourse,            // In edit mode: a node which has been added to the course, and is able to be added again
  FINISHED_SuccessfulRun,     // In finished mode: the run was successful
  FINISHED_UnsuccessfulRun    // In finished mode: the run was unsuccessful
} State;

// Defined types
struct Color {
  char r;
  char g;
  char b;
  Color(char r, char g, char b) {
    this->r = r;
    this->g = g;
    this->b = b;
  }
};

struct StateData {
  Color* colorIdle;
  Color* colorOnTouch; // If null, non-receptive to touch
  StateData(Color* colorIdle, Color* colorOnTouch) {
    this->colorIdle = colorIdle;
    this->colorOnTouch = colorOnTouch;
  }
};

struct OutboundMessage {
  short type;
  short id;
  uint32_t data;
};

// Prototypes
OutboundMessage createOutboundMessage(char type, uint32_t data);
State parseStateName(std::string stateName);
void setState(State newState);
void processIncomingMessage(std::string msg);
void touchpadCallback(void* param);
void sendMessage(OutboundMessage msg);
void rfListen(void* param);
void messageLoop(void* param);
uint32_t currentTimeAbs();
uint32_t currentTime();
void writeLed(Color color);

// Hardware constants
const touch_pad_t tpPin = TOUCH_PAD_NUM7;
const int rfReceivePin = D9;
const int speakerPin = D0;
const int ledRedPin = D2;
const int ledGreenPin = D3;
const int ledBluePin = D1;

// Tasks
TaskHandle_t messageLoopTask;
TaskHandle_t rfListenerTask;

// Message types
const char MTYPE_REGISTER       = 2;
const char MTYPE_TOUCHED        = 100;
const char MTYPE_ERROR          = 33;
const char MTYPE_TIMESTAMPRESET = 66;
const char MTYPE_ACK            = 111;

// Other globals
NetworkModule module;
WiFiClient socket;
std::queue<OutboundMessage> outboundMessageQueue;

// Constants (TODO should maybe be moved to EEPROM)
const char* HOST = "192.168.1.111";
const uint16_t PORT = 5000;

// State globals
bool touched = false;             // Helper variable to track if the node has been triggered. Must be reset by server
uint32_t timestampLastReset = 0;  // Timestamp when the node received the most recent RF broadcast
State state;                      // Current State of the node
StateData* stateData = nullptr;   // ... Data of the current state...

void setup() {

  // Hardware setup
  Serial.begin(SERIAL_BAUDRATE);
  EEPROM.begin(EEPROM_SIZE);
  pinMode(rfReceivePin, INPUT);
  pinMode(speakerPin, OUTPUT);
  ledcAttach(ledRedPin, 5000, 8);
  ledcAttach(ledGreenPin, 5000, 8);
  ledcAttach(ledBluePin, 5000, 8);
  digitalWrite(speakerPin, HIGH);
  readEeprom((char*)&module, 0, sizeof(NetworkModule)); // Load EEPROM

  // Connect to wifi
  setState(INIT_WaitingWifi);
  WiFi.begin(module.networkSsid, module.networkPassword);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf(".");
    vTaskDelay(250);
  }
  Serial.printf("\nWiFi connected with IP: %s\n", WiFi.localIP().toString().c_str());


  // Connect to socket
  setState(INIT_WaitingServer);
  Serial.printf("Trying to connect to socket at host %s:%d", HOST, PORT);
  while(!socket.connect(HOST, PORT)) {
    Serial.printf(".");
    vTaskDelay(250);
  }
  Serial.printf("\nConnected to socket at host %s:%d\n", HOST, PORT);

  Serial.printf("starting tasks\n");
  setState(INIT_Complete);
  xTaskCreatePinnedToCore(        // Message loop on core 0
    messageLoop,      /* Task function. */
    "Message_Loop",   /* name of task. */
    16384,            /* Stack size of task */
    NULL,             /* parameter of the task */
    0,                /* priority of the task */
    &messageLoopTask, /* Task handle to keep track of created task */
    0);               /* pin task to core 0 */
  Serial.printf("started message loop\n");

  xTaskCreatePinnedToCore(        // RF receiver loop on core 1
    rfListen,         /* Task function. */
    "RF_Listen",      /* name of task. */
    16384,            /* Stack size of task */
    NULL,             /* parameter of the task */
    0,                /* priority of the task */
    &rfListenerTask,  /* Task handle to keep track of created task */
    1);               /* pin task to core 1 */
  Serial.printf("started rf receiver\n");
  sleep(1);
}

void loop() {
  vTaskDelete(NULL);
}

/* ************************* */
/*     TimeSync Functions    */
/* ************************* */

int countSetBits(int inp) {
  int count = 0;
  while (inp) {
    inp = inp & (inp - 1);
    count++;
  }
  return count;
}

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
    outboundMessageQueue.push(createOutboundMessage(MTYPE_TOUCHED, currentTime()));
    if (stateData->colorOnTouch) {
      writeLed(*(stateData->colorOnTouch));
    }
  }
}

/* ************************* */
/*    Messaging Functions    */
/* ************************* */

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
    outboundMessageQueue.push(createOutboundMessage(MTYPE_ACK, currentTime()));
    std::getline(iss, word, ' ');
  }

  if (word == "UNTOUCH") {
    touched = false;
    writeLed(*(stateData->colorIdle));
  }

  if (word == "SET_STATE") {
    std::getline(iss, word, ' '); // Second word tells us 
    setState(parseStateName(word));
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

/* ************************* */
/*           Tasks           */
/* ************************* */

void messageLoop(void* param) {
  Serial.printf("Message loop running on core %d\n", xPortGetCoreID());
  
  // Normally aliased to touch_val_t
  //   which is uint16_t on ESP32, uint32_t on ESP32s2/s3
  // Explicitly use uint32_t because the behavior of touchPadInterrupt() is different for both anyway
  // ('threshold' argument is a true threshold for ESP32, but on ESP32s2/s3 it's an INCREMENT value)
  uint32_t _touchVal, _touchIncrement;
  _touchVal = touchRead(tpPin);
  _touchIncrement = _touchVal / 8;
  sendMessage(createOutboundMessage(MTYPE_REGISTER, _touchVal + _touchIncrement));
  touchAttachInterrupt(tpPin, &touchpadCallback, _touchIncrement);
  Serial.printf("Baseline tp val = %d; set threshold to %d\n", _touchVal, _touchVal + _touchIncrement);

  while (true) {
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
    
    //vTaskDelay((TickType_t) (200 / portTICK_PERIOD_MS)); // ~200ms delay to satisfy the scheduler
  }
}

void rfListen(void* param) {
  Serial.printf("rfListen running on core %d\n", xPortGetCoreID());

  unsigned int buf = 0b0; // Bit buffer received on the RF pin. Left boundary is old, right boundary is new
  int matchedBits;

  unsigned int debugMask;

  while(true) {

    digitalWrite(speakerPin, HIGH);

    // Flush the oldest value and create a slot for the new value
    buf = buf << 1;

    // Read the pin and store it as the final bit
    //  (value of digitalRead is either 0 or 1, so a plain OR will stick it at the right boundary)
    buf = buf | digitalRead(rfReceivePin);
    matchedBits = countSetBits(~(buf ^ rfKey));

    // Debug: print the buf
    //debugMask = 0b10000000000000000000000000000000;
    //for (int i = 0; i < 32; i++) {
    //  Serial.printf("%d", (buf & debugMask) > 0);
    //  debugMask = debugMask >> 1;
    //}
    //Serial.printf(" (%d / %d)", matchedBits, rfKeyRequiredMatches);
    //Serial.printf("\n");
    
    // Check if buffer matches the key to acceptable tolerance
    if (matchedBits >= rfKeyRequiredMatches ) {
      outboundMessageQueue.push(createOutboundMessage(MTYPE_TIMESTAMPRESET, currentTime()));
      timestampLastReset = currentTimeAbs();
      Serial.printf("Received timestamp-reset key (%d / %d matched bits, %d required) \n", matchedBits, 32, rfKeyRequiredMatches);
    };

    delay(rfPulseIntervalMs);
  }
}

/* ************************* */
/*           State           */
/* ************************* */
// ... ugh

// Colors
Color colorOff        = Color(  0,   0,   0);
Color colorRed        = Color(255,   0,   0);
Color colorGreen      = Color(  0, 255,   0);
Color colorBlue       = Color(  0,   0, 255);
Color colorCyan       = Color(  0, 150, 150);
Color colorDimOrange  = Color(255,  35,   0);
Color colorWhite      = Color(255, 255, 255);
//                                                  Standby     On-touch
StateData SD_INIT_BootStart             = StateData(&colorCyan,         nullptr);
StateData SD_INIT_WaitingWifi           = StateData(&colorRed,          nullptr);
StateData SD_INIT_WaitingServer         = StateData(&colorBlue,         nullptr);
StateData SD_INIT_Complete              = StateData(&colorGreen,        &colorWhite);
StateData SD_READYRUN_StartNode         = StateData(&colorBlue,         &colorWhite);
StateData SD_READYRUN_NotPartOfCourse   = StateData(&colorOff,          &colorRed);
StateData SD_READYRUN_NoTriggersDone    = StateData(&colorRed,          &colorWhite);
StateData SD_READYRUN_NextUp            = StateData(&colorDimOrange,    &colorWhite);
StateData SD_READYRUN_SomeTriggersDone  = StateData(&colorDimOrange,    &colorWhite);
StateData SD_READYRUN_AllTriggersDone   = StateData(&colorGreen,        &colorWhite);
StateData SD_DEFINE_SelectedNode        = StateData(&colorWhite,        nullptr);
StateData SD_DEFINE_NotInCourse         = StateData(&colorRed,          &colorWhite);
StateData SD_DEFINE_InCourse            = StateData(&colorDimOrange,    &colorWhite);
StateData SD_FINISHED_SuccessfulRun     = StateData(&colorGreen,        nullptr);
StateData SD_FINISHED_UnsuccessfulRun   = StateData(&colorOff,          nullptr);

State parseStateName(std::string stateName) {
  if      (stateName == "READYRUN_StartNode")         return READYRUN_StartNode;
  else if (stateName == "READYRUN_NotPartOfCourse")   return READYRUN_NotPartOfCourse;
  else if (stateName == "READYRUN_NoTriggersDone")    return READYRUN_NoTriggersDone;
  else if (stateName == "READYRUN_SomeTriggersDone")  return READYRUN_SomeTriggersDone;
  else if (stateName == "READYRUN_AllTriggersDone")   return READYRUN_AllTriggersDone;
  else if (stateName == "DEFINE_SelectedNode")        return DEFINE_SelectedNode;
  else if (stateName == "DEFINE_NotInCourse")         return DEFINE_NotInCourse;
  else if (stateName == "DEFINE_InCourse")            return DEFINE_InCourse;
  else if (stateName == "FINISHED_SuccessfulRun")     return FINISHED_SuccessfulRun;
  else if (stateName == "FINISHED_UnsuccessfulRun")   return FINISHED_UnsuccessfulRun;
}

void setState(State newState) {
  switch(newState) {
    case INIT_WaitingWifi:          stateData = &SD_INIT_WaitingWifi;          break;   
    case INIT_WaitingServer:        stateData = &SD_INIT_WaitingServer;        break;   
    case INIT_Complete:             stateData = &SD_INIT_Complete;             break;       
    case READYRUN_StartNode:        stateData = &SD_READYRUN_StartNode;        break;
    case READYRUN_NotPartOfCourse:  stateData = &SD_READYRUN_NotPartOfCourse;  break;
    case READYRUN_NoTriggersDone:   stateData = &SD_READYRUN_NoTriggersDone;   break;
    case READYRUN_SomeTriggersDone: stateData = &SD_READYRUN_SomeTriggersDone; break;
    case READYRUN_AllTriggersDone:  stateData = &SD_READYRUN_AllTriggersDone;  break;
    case DEFINE_SelectedNode:       stateData = &SD_DEFINE_SelectedNode;       break;
    case DEFINE_NotInCourse:        stateData = &SD_DEFINE_NotInCourse;        break;
    case DEFINE_InCourse:           stateData = &SD_DEFINE_InCourse;           break;
    case FINISHED_SuccessfulRun:    stateData = &SD_FINISHED_SuccessfulRun;    break;
    case FINISHED_UnsuccessfulRun:  stateData = &SD_FINISHED_UnsuccessfulRun;  break;
  }
  Serial.printf("SetState (previous %d, new %d)\n", state, newState);
  state = newState;
  touched = false;
  writeLed(*(stateData->colorIdle));
}

/* ************************* */
/*         Utilities         */
/* ************************* */

// TODO make inline
uint32_t currentTimeAbs() {
  return millis();
}
uint32_t currentTime() {
  return currentTimeAbs() - timestampLastReset;
}

void writeLed(Color color) {
  ledcWrite(ledRedPin, color.r);
  ledcWrite(ledGreenPin, color.g);
  ledcWrite(ledBluePin, color.b);
}