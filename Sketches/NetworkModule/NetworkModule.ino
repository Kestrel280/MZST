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

#include "Hardware.h"
#include "Message.h"
#include "State.h"
#include "Timing.h"
#include "Utils.h"


// Constants
const int64_t UNTOUCH_TIMEOUT_US = 1000000;

// Prototypes
void touchpadCallback(void* param);

// Globals
TaskHandle_t messageLoopTask;                       // Handle on message-loop thread
TaskHandle_t rfListenerTask;                        // Handle on RF-listen-loop thread
NetworkModule networkModule;                        // This network module object
WiFiClient serverSocket;                            // Socket for server communication
WiFiClient adminSocket;                             // Socket for pre-server-connection "admin" communications
WiFiServer adminSocketListener;                     // Listener socket for incoming "admin" comms
std::queue<OutboundMessage> outboundMessageQueue;   // Queue for outbound messages
State state;                                        // Current State of the node, updated via setState()
StateData* stateData = nullptr;                     // State data associated with current state; always updated along with state
int64_t dbg_time;                                   // ...
int64_t timestampLastResetUs = 0;                   // Timestamp when the node received the most recent RF broadcast (in microseconds)
int64_t timeLastTouchUs = 0;                        // Timestamp of last time node was touched (in microseconds)
bool pendingServerUntouch = false;                  // Whether the server has sent us an UNTOUCH message that we haven't yet processed
bool inTouchCooldown = false;                       // Whether we're in cooldown from the previous touch

// Program entrypoint
void setup() {
  // Hardware setup
  Serial.begin(SERIAL_BAUDRATE);
  EEPROM.begin(EEPROM_SIZE);
  pinMode(RF_RECEIVE_PIN, INPUT);
  pinMode(SPEAKER_PIN, OUTPUT);
  ledcAttach(LED_RED_PIN, 5000, 8);
  ledcAttach(LED_GREEN_PIN, 5000, 8);
  ledcAttach(LED_BLUE_PIN, 5000, 8);
  digitalWrite(SPEAKER_PIN, HIGH);
  dumpEeprom();
  readEeprom((char*)&networkModule, 0, sizeof(NetworkModule)); // Initialize networkModule by loading from EEPROM

  // Connect to wifi
  setState(INIT_WaitingWifi);
  WiFi.begin(networkModule.networkSsid, networkModule.networkPassword);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf(".");
    vTaskDelay(250);
  }
  Serial.printf("\nWiFi connected with IP: %s\n", WiFi.localIP().toString().c_str());

  sleep(2); // Pause a couple seconds... gives us time to restart/update the server if we just reset from a SHUTDOWN message

  // Connect to server; while connecting, if we receive an admin connection, accept it and process whatever message it has for us
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
  Serial.printf("\nConnected to server at host %s:%d\n", networkModule.serverIp, networkModule.serverPort);
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
  vTaskDelete(NULL); // Kill this thread; loop() should be dead
  Serial.printf("---Unreachable code in loop()---");
}

// ISR for when touchpad is triggered
void touchpadCallback() {
  if (!inTouchCooldown) {
    inTouchCooldown = true;
    outboundMessageQueue.push(createOutboundMessage(MTYPE_TOUCHED, currentTime()));
    if (stateData->colorOnTouch) {
      writeLed(stateData->colorOnTouch);
    }
    timeLastTouchUs = currentTimeAbs();
  }
}
