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

int64_t dbg_time;

// Constants
const int64_t UNTOUCH_TIMEOUT_US = 1000000;

typedef enum _State {
  INIT_BootStart,             // State set immediately upon module boot
  INIT_WaitingWifi,           // While waiting for wifi connection
  INIT_WaitingServer,         // While waiting for server connection
  INIT_Complete,              // Connected to server, awaiting instructions
  READYRUN_StartNode,         // Starting node for a course. When it's touched, server will move into RUN state
  READYRUN_NotPartOfCourse,   // In ready/run mode: the node IS NOT part of the active course
  READYRUN_NoTriggersDone,    // In ready/run mode: the node is part of the active course, but has not yet been triggered a single time
  READYRUN_NextUp,            // In ready/run mode: the node is part of the active course, and is the next node which must be triggered
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
int64_t currentTimeAbs();
int64_t currentTime();
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
NetworkModule networkModule;
WiFiClient serverSocket;
WiFiClient adminSocket;
WiFiServer adminSocketListener;
std::queue<OutboundMessage> outboundMessageQueue;

// State globals
int64_t timestampLastResetUs = 0;   // Timestamp when the node received the most recent RF broadcast
int64_t timeLastTouchUs = 0;        // Timestamp of last time node was touched
State state;                        // Current State of the node
StateData* stateData = nullptr;     // ... Data of the current state...
bool pendingServerUntouch = false;  // Whether the server has sent us an UNTOUCH message that we haven't yet processed
bool inTouchCooldown = false;       // Whether we're in cooldown from the previous touch


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
  dumpEeprom();
  readEeprom((char*)&networkModule, 0, sizeof(NetworkModule)); // Load EEPROM

  // Connect to wifi
  setState(INIT_WaitingWifi);
  WiFi.begin(networkModule.networkSsid, networkModule.networkPassword);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf(".");
    vTaskDelay(250);
  }
  Serial.printf("\nWiFi connected with IP: %s\n", WiFi.localIP().toString().c_str());

  sleep(2); // Pause a couple seconds... gives us time to restart/update the server if we just reset from a SHUTDOWN message

  // Connect to socket
  setState(INIT_WaitingServer);
  Serial.printf("Trying to connect to server socket at host %s:%d", networkModule.serverIp, networkModule.serverPort);
  adminSocketListener = WiFiServer(ADMIN_PORT);
  adminSocketListener.begin();
  while(!serverSocket.connect(networkModule.serverIp, networkModule.serverPort)) {
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
  Serial.printf("\nConnected to socket at host %s:%d\n", networkModule.serverIp, networkModule.serverPort);

  adminSocketListener.close();

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

inline int countSetBits(int inp) {
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
  if (!inTouchCooldown) {
    inTouchCooldown = true;
    outboundMessageQueue.push(createOutboundMessage(MTYPE_TOUCHED, currentTime()));
    if (stateData->colorOnTouch) {
      writeLed(*(stateData->colorOnTouch));
    }
    timeLastTouchUs = currentTimeAbs();
  }
}

/* ************************* */
/*    Messaging Functions    */
/* ************************* */

void processIncomingMessage(std::string msg) {
  Serial.printf("Received message from server: %s\n", msg.c_str());
  //for (char c : msg) { Serial.printf("%x ", c); }

  std::stringstream iss(msg);
  std::string command, value;

  std::getline(iss, command, ' '); // Read the first word of the message into 'command'

  if ((command == "RESTART") || (command == "SHUTDOWN")) {
    Serial.printf("Received %s message! Restarting!", command.c_str());
    esp_restart();
  }

  if (command == "REQ_ACK") { // Server is requesting an ACK; send it, then continue
    outboundMessageQueue.push(createOutboundMessage(MTYPE_ACK, currentTime()));
    std::getline(iss, command, ' ');
  }

  if (command == "UNTOUCH") {
    pendingServerUntouch = true;
  }

  if (command == "SET_STATE") {
    std::getline(iss, value, ' '); // Second word tells us what state to set
    setState(parseStateName(value));
  }

  if (command == "SET_EEPROM_VALUE") {
    std::getline(iss, command, ' '); // Second word tells us what value to update
    std::getline(iss, value, ' '); // Third word tells us what the new value is

    if      (command == "NETWORKSSID")      strncpy((networkModule).networkSsid, value.c_str(), sizeof(networkModule.networkSsid));
    else if (command == "NETWORKPASSWORD")  strncpy((networkModule).networkPassword, value.c_str(), sizeof(networkModule.networkPassword));
    else if (command == "SERVERIP")         strncpy((networkModule).serverIp, value.c_str(), sizeof(networkModule.serverIp));
    else if (command == "SERVERPORT")       networkModule.serverPort = (unsigned int) atoi(value.c_str());
    else if (command == "MODULEID")         networkModule.moduleId = (unsigned int) atoi(value.c_str());

    writeEeprom((char*)&networkModule, sizeof(NetworkModule), 0);
  }
}

OutboundMessage createOutboundMessage(char type, unsigned long data) {
  OutboundMessage msg;
  msg.type = type;
  msg.id = networkModule.moduleId;
  msg.data = data;
  return msg;
}

void sendMessage(OutboundMessage msg) {
  serverSocket.write_P((char*)(&msg), sizeof(OutboundMessage));
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
  uint32_t _touchVal, touchIncrement, touchThreshold;
  _touchVal = touchRead(tpPin);
  touchIncrement = _touchVal / 8;
  touchThreshold = _touchVal + touchIncrement;
  sendMessage(createOutboundMessage(MTYPE_REGISTER, touchThreshold));
  touchAttachInterrupt(tpPin, &touchpadCallback, touchIncrement);
  Serial.printf("Baseline tp val = %d; set threshold to %d\n", _touchVal, touchThreshold);

  while (true) {
    // Send messages
    while(!outboundMessageQueue.empty()) {
      OutboundMessage msg = outboundMessageQueue.front();
      Serial.printf("Sending message to server with type: %d\n", msg.type);
      sendMessage(msg);
      outboundMessageQueue.pop();
    }

    // Receive messages
    while (serverSocket.available() > 0) {
      std::string line = std::string(serverSocket.readStringUntil('\n').c_str());
      processIncomingMessage(line);
    }

    // Reset to untouch color iff server has sent us UNTOUCH and we're ready
    //  We're ready if UNTOUCH_TIMEOUT has passed since last touch and we're not currently touching
    if (inTouchCooldown) {
      inTouchCooldown = !(((currentTimeAbs() - timeLastTouchUs) > UNTOUCH_TIMEOUT_US) && (touchRead(tpPin) < (touchThreshold - touchIncrement / 2)));
    }
    // Unset the pendingServerUntouch and inTouchCooldown flags
    if (pendingServerUntouch && !inTouchCooldown) {
      vTaskDelay((TickType_t) (100 / portTICK_PERIOD_MS)); // ~100ms delay before unsetting flags, to avoid re-touch on release. TODO do this better
      inTouchCooldown = false;
      pendingServerUntouch = false;
      writeLed(*(stateData->colorIdle));
    }
  }
}

void rfListen(void* param) {
  Serial.printf("rfListen running on core %d\n", xPortGetCoreID());

  const int LOOP_TIME_US = 1000; // How long it takes to run one 
  unsigned int buf = 0b0; // Bit buffer received on the RF pin. Left boundary is old, right boundary is new
  int matchedBits;

  unsigned int debugMask;

  dbg_time = currentTimeAbs();

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
      outboundMessageQueue.push(createOutboundMessage(MTYPE_TIMESTAMPRESET, matchedBits));
      timestampLastResetUs = currentTimeAbs();
      Serial.printf("Received timestamp-reset key (%d / %d matched bits, %d required) \n", matchedBits, 32, rfKeyRequiredMatches);
    };

    dbg_time = currentTimeAbs(); // this call just happens to put us at pretty much exactly 1ms for this loop, so. leaving it in :-)
    esp_rom_delay_us(rfPulseIntervalUs + TRANSMIT_LOOP_TIME_US); // (busy wait) Delay for the pulse duration + account for transmitter lag
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
Color colorPurple     = Color(100,   0, 100);
Color colorCyan       = Color(  0, 100, 100);
Color colorDimOrange  = Color(255,  35,   0);
Color colorWhite      = Color(255, 255, 255);
//                                                  Standby     On-touch
StateData SD_INIT_BootStart             = StateData(&colorCyan,         nullptr);
StateData SD_INIT_WaitingWifi           = StateData(&colorRed,          nullptr);
StateData SD_INIT_WaitingServer         = StateData(&colorBlue,         nullptr);
StateData SD_INIT_Complete              = StateData(&colorGreen,        &colorWhite);
StateData SD_READYRUN_StartNode         = StateData(&colorPurple,       &colorWhite);
StateData SD_READYRUN_NotPartOfCourse   = StateData(&colorOff,          &colorRed);
StateData SD_READYRUN_NoTriggersDone    = StateData(&colorRed,          &colorWhite); // TODO when nextUp is hooked up, change on-touch to red
StateData SD_READYRUN_NextUp            = StateData(&colorBlue,         &colorWhite);
StateData SD_READYRUN_SomeTriggersDone  = StateData(&colorDimOrange,    &colorWhite); // TODO when nextUp is hooked up, change on-touch to red
StateData SD_READYRUN_AllTriggersDone   = StateData(&colorGreen,        &colorWhite); // TODO when nextUp is hooked up, change on-touch to red
StateData SD_DEFINE_SelectedNode        = StateData(&colorWhite,        nullptr);
StateData SD_DEFINE_NotInCourse         = StateData(&colorRed,          &colorBlue);
StateData SD_DEFINE_InCourse            = StateData(&colorDimOrange,    &colorWhite);
StateData SD_FINISHED_SuccessfulRun     = StateData(&colorGreen,        nullptr);
StateData SD_FINISHED_UnsuccessfulRun   = StateData(&colorOff,          nullptr);

State parseStateName(std::string stateName) {
  if      (stateName == "READYRUN_StartNode")         return READYRUN_StartNode;
  else if (stateName == "READYRUN_NotPartOfCourse")   return READYRUN_NotPartOfCourse;
  else if (stateName == "READYRUN_NoTriggersDone")    return READYRUN_NoTriggersDone;
  else if (stateName == "READYRUN_NextUp")            return READYRUN_NextUp;
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
  writeLed(*(stateData->colorIdle));
}

/* ************************* */
/*         Utilities         */
/* ************************* */

inline int64_t currentTimeAbs() {
  return esp_timer_get_time();
}
inline int64_t currentTime() {
  return currentTimeAbs() - timestampLastResetUs;
}

void writeLed(Color color) {
  ledcWrite(ledRedPin, color.r);
  ledcWrite(ledGreenPin, color.g);
  ledcWrite(ledBluePin, color.b);
}