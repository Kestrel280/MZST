/* ESP32S3 Dev Module | esp32 by Espressif Systems, v3.2.0*/

#include "esp_system.h"
#include "esp32-hal-touch.h"

#include "../_include/NetworkModule.h"
#include "../_include/Server.h"
#include "../_include/Eeprom_Helpers.h"
#include "../_include/Utils.h"

#include "State.h"

// Constants
const int64_t UNTOUCH_TIMEOUT_US = 1000000;

// Prototypes
void touchpadCallback(void* param) __attribute__((interrupt_handler));
void rfPacketCallback(void* param) __attribute__((interrupt_handler));
void messageLoop(void* param);

// Globals
TaskHandle_t messageLoopTask;                       // Handle on message-loop thread
TaskHandle_t feedbackLoopTask;                      // Handle on feedback-loop thread
State state;                                        // Current State of the node, updated via setState()
StateData* stateData = nullptr;                     // State data associated with current state; always updated along with state
int64_t timestampLastResetUs = 0;                   // Timestamp when the node received the most recent RF broadcast (in microseconds)
int64_t timeLastTouchUs = 0;                        // Timestamp of last time node was touched (in microseconds)
bool pendingServerUntouch = false;                  // Whether the server has sent us an UNTOUCH message that we haven't yet processed
bool inTouchCooldown = false;                       // Whether we're in cooldown from the previous touch
uint32_t touchIncrement, touchThreshold;            // Touchpad values

/* ****************************************
          Program entrypoint
  Hardware setup
  Connect to wifi
  Connect to server/Receive admin-msgs
  Register interrupts
  Start thread fns/loops
  ***************************************** */
void setup() {
  /* ************** */
  /* Hardware Setup */
  /* ************** */
  Serial.begin(SERIAL_BAUDRATE);
  EEPROM.begin(EEPROM_SIZE);
  pinMode(RF_D0, INPUT_PULLDOWN);
  pinMode(RF_D1, INPUT_PULLDOWN);
  pinMode(RF_D2, INPUT_PULLDOWN);
  pinMode(RF_D3, INPUT_PULLDOWN);
  pinMode(RF_D4, INPUT_PULLDOWN);
  pinMode(RF_D5, INPUT_PULLDOWN);
  pinMode(RF_D6, INPUT_PULLDOWN);
  pinMode(RF_D7, INPUT_PULLDOWN);
  pinMode(RF_VT, INPUT_PULLDOWN);
  digitalWrite(AUDIO_PIN, 0);
  digitalWrite(AUDIO_PIN_EN, 0);
  ledcAttach(LED_RED_PIN, 5000, 8);
  ledcAttach(LED_GREEN_PIN, 5000, 8);
  ledcAttach(LED_BLUE_PIN, 5000, 8);
  while(!Serial);
  dumpEeprom();
  readEeprom((char*)&module, 0, sizeof(module)); // Initialize module by loading from EEPROM
  
  /* *************** */
  /* Connect to wifi */
  /* *************** */
  setState(INIT_WaitingWifi);
  WiFi.begin(module.networkSsid, module.networkPassword);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf(".");
    vTaskDelay(250);
  }
  Serial.printf("\nWiFi connected with IP: %s\n", WiFi.localIP().toString().c_str());
  sleep(2); // Pause a couple seconds... gives us time to restart/update the server if we just reset from a SHUTDOWN message

  /* ***************** */
  /* Connect to server */
  /* ***************** */
  // While connecting, if we receive an admin connection, accept it and process whatever message it has for us
  setState(INIT_WaitingServer);
  Serial.printf("Trying to connect to server socket at host %s:%d", module.serverIp, module.serverPort);
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
  adminSocketListener.close();
  sendMessage(createOutboundMessage(MTYPE_REGISTER_NODE, touchThreshold));
  Serial.printf("\nConnected & registered to server at %s:%d\n", module.serverIp, module.serverPort);
  
  /* ******************* */
  /* Register interrupts */
  /* ******************* */
  // Touchpad interrupt
  // ESP32 vs ESP32s3 differences: 
  //    Touch values are normally typedef'd to touch_val_t, which is uint16_t on ESP32, uint32_t on ESP32s2/s3
  //    touchPadInterrupt() behaves differently on ESP32 ('threshold' argument is a true threshold for ESP32, but on ESP32s2/s3 it's an INCREMENT value)
  // Using uint32_t explicitly, caused some headaches in the past
  uint32_t _touchVal;
  _touchVal = touchRead(TP_PIN);
  touchIncrement = _touchVal / 8;
  touchThreshold = _touchVal + touchIncrement;
  touchAttachInterrupt(TP_PIN, &touchpadCallback, touchIncrement);
  Serial.printf("Baseline tp val = %d; set threshold to %d\n", _touchVal, touchThreshold);

  // RF packet interrupt
  attachInterrupt(RF_VT, rfPacketCallback, RISING);

  /* ****************** */
  /* Start master loops */
  /* ****************** */
  Serial.printf("Setup complete, starting tasks\n");
  setState(INIT_Complete);

  xTaskCreatePinnedToCore(        // Message loop on core 0
    messageLoop,        /* Task function. */
    "Message_Loop",     /* name of task. */
    16384,              /* Stack size of task */
    NULL,               /* parameter of the task */
    0,                  /* priority of the task */
    &messageLoopTask,   /* Task handle to keep track of created task */
    0);                 /* pin task to core 0 */

  xTaskCreatePinnedToCore(        // Feedback loop on core 1
    feedbackLoop,       /* Task function. */
    "Feedback_Loop",    /* name of task. */
    16384,              /* Stack size of task */
    NULL,               /* parameter of the task */
    0,                  /* priority of the task */
    &feedbackLoopTask,  /* Task handle to keep track of created task */
    1);                 /* pin task to core 1 */
  return;
}

// loop() is automatically entered after setup() returns
// We're managing our own loops, so just kill this thread
void loop() {
  vTaskDelete(NULL);
  /* unreachable */
  Serial.printf("---Unreachable code in loop()---");
  return;
}

/* ****************************************
          ISRs
  ***************************************** */
// ISR for when touchpad is triggered
void touchpadCallback() {
  if (!inTouchCooldown) {
    inTouchCooldown = true;
    outboundMessageQueue.push(createOutboundMessage(MTYPE_TOUCHED, currentTimeAbs() - timeLastTouchUs));
    writeLed(stateData->colorOnTouch);
    timeLastTouchUs = currentTimeAbs();
  }
  return;
}

// ISR for when RF_VT goes high (RF packet received)
void rfPacketCallback() {
  char stamp = 0b0;
  // TODO read directly from GPIO register?
  stamp = \
    digitalRead(RF_D0) << 7 |\
    digitalRead(RF_D1) << 6 |\
    digitalRead(RF_D2) << 5 |\
    digitalRead(RF_D3) << 4 |\
    digitalRead(RF_D4) << 3 |\
    digitalRead(RF_D5) << 2 |\
    digitalRead(RF_D6) << 1 |\
    digitalRead(RF_D7);
  Serial.printf("RF received stamp: 0x%x\n", stamp);

  // TODO this is just debug
  writeLed(stateData->colorRf);
  return;
}

/* ****************************************
          Loops/Thread fns
  ***************************************** */
// Master message loop
void messageLoop(void* param) {
  Serial.printf("Message loop running on core %d\n", xPortGetCoreID());
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

  }
  /* unreachable */
  return;
}

// Master feedback-submodule loop
void feedbackLoop(void* param) {
  Serial.printf("Feedback loop running on core %d\n", xPortGetCoreID());
  while (true) {
    // Reset to untouch color iff server has sent us UNTOUCH and we're ready
    //  We're ready if UNTOUCH_TIMEOUT has passed since last touch and we're not currently touching
    if (inTouchCooldown) {
      inTouchCooldown = !(((currentTimeAbs() - timeLastTouchUs) > UNTOUCH_TIMEOUT_US) && (touchRead(TP_PIN) < (touchThreshold - touchIncrement / 2)));
    }
    // Unset the pendingServerUntouch and inTouchCooldown flags
    if (pendingServerUntouch && !inTouchCooldown) {
      vTaskDelay((TickType_t) (100 / portTICK_PERIOD_MS)); // ~100ms delay before unsetting flags, to avoid re-touch on release. TODO do this better
      inTouchCooldown = false;c:\Users\downe\Documents\Repos\MZST\Sketches\_include\Module.h
      pendingServerUntouch = false;
      writeLed(stateData->colorIdle);
    }
  vTaskDelay(250);
  }
}

/* ****************************************
          Misc
  ***************************************** */
void processModuleSpecificMessage(std::string msg) {
  std::stringstream iss(msg);
  std::string command, value;

  std::getline(iss, command, ' '); // Read the first word of the message stream into 'command'

  if (command == "UNTOUCH") {
    pendingServerUntouch = true;
  }

  if (command == "SET_STATE") {
    std::getline(iss, value, ' '); // Second word tells us what state to set
    setState(parseStateName(value));
  }

  return;
}