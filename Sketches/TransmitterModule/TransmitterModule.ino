/* XIAO_ESP32S3 | esp32 by Espressif Systems, v3.2.0*/

#include <string>
#include <sstream>
#include <queue>

#include <EEPROM.h>
#include <WiFi.h>

#include "../_include/TransmitterModule.h"
#include "../_include/Server.h"
#include "../_include/Eeprom_Helpers.h"

// Prototypes
void debugButtonCallback();
void messageLoop(void* param);
void dumbLoop(void* param);

// Globals
char stamp = 0b0;                             // Most recent RF "Stamp" broadcast
TaskHandle_t messageLoopTask;                 // Handle on message loop task
TaskHandle_t dumbLoopTask;                    // Handle on dumb loop task
uint64_t timeLastTransmitUs;

/* ****************************************
          Program entrypoint
  Hardware setup
  Connect to wifi
  Connect to server/Receive admin-msgs
  ...
  ***************************************** */
void setup() {
  /* ************** */
  /* Hardware Setup */
  /* ************** */
  Serial.begin(SERIAL_BAUDRATE);
  EEPROM.begin(EEPROM_SIZE);
  pinMode(PUSHBUTTON, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(USER_LED, OUTPUT);
  pinMode(RF_D0, OUTPUT);
  pinMode(RF_D1, OUTPUT);
  pinMode(RF_D2, OUTPUT);
  pinMode(RF_D3, OUTPUT);
  pinMode(RF_D4, OUTPUT);
  pinMode(RF_D5, OUTPUT);
  pinMode(RF_D6, OUTPUT);
  pinMode(RF_D7, OUTPUT);
  pinMode(RF_TE, OUTPUT);
  while(!Serial);
  dumpEeprom();
  readEeprom((char*)&module, 0, sizeof(Module)); // Initialize module by loading from EEPROM

  /* *************** */
  /* Connect to wifi */
  /* *************** */
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
  adminSocketListener.close();
  sendMessage(createOutboundMessage(MTYPE_REGISTER_TRNS, 0));
  Serial.printf("\nConnected & registered to server at %s:%d\n", module.serverIp, module.serverPort);

  /* ******************* */
  /* Register interrupts */
  /* ******************* */
  attachInterrupt(PUSHBUTTON, debugButtonCallback, RISING);

  /* ****************** */
  /* Start master loops */
  /* ****************** */
  Serial.printf("Setup complete, starting tasks\n");

  xTaskCreatePinnedToCore(        // Message loop on core 0
    messageLoop,        /* Task function. */
    "Message_Loop",     /* name of task. */
    16384,              /* Stack size of task */
    NULL,               /* parameter of the task */
    0,                  /* priority of the task */
    &messageLoopTask,   /* Task handle to keep track of created task */
    0);                 /* pin task to core 0 */

    xTaskCreatePinnedToCore(        // Dumb loop on core 1
    dumbLoop,           /* Task function. */
    "Dumb_Loop",        /* name of task. */
    16384,              /* Stack size of task */
    NULL,               /* parameter of the task */
    0,                  /* priority of the task */
    &dumbLoopTask,      /* Task handle to keep track of created task */
    1);                 /* pin task to core 1 */
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
void debugButtonCallback() {
  transmit();
  return;
}

/* ****************************************
          Loops/Thread fns
  ***************************************** */
void dumbLoop(void* param) {
  while (true) {};
}

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
    
    if (timeLastTransmitUs > 0) {
      uint64_t _curtimeus = currentTimeAbs();
      if ((_curtimeus - timeLastTransmitUs) > TRANSMIT_DURATION_US) {
        digitalWrite(RF_TE, LOW);
        timeLastTransmitUs = 0;
        Serial.printf("...stopped transmitting at %d\n", _curtimeus);
      }
    }
  }
  Serial.printf("unreachable code in messageloop!\n");
}

/* ****************************************
          Misc
  ***************************************** */
void processModuleSpecificMessage(std::string msg) {
  std::stringstream iss(msg);
  std::string command, value;

  std::getline(iss, command, ' '); // Read the first word of the message stream into 'command'
  
  if (command == "TRANSMIT") {
    transmit();
  }
}

void transmit() {
  // TODO write directly to GPIO register?
  timeLastTransmitUs = currentTimeAbs(); // TODO utterly baffling bug, this always results in timeLastTransmitUs = 0xFF00 (65280). Thought maybe it was a caching thing but 'volatile' and tricks didn't seem to solve it. Check disasm, it's a little weird (compiler introduced new timeLastTransmit symbol worth investigating)
  digitalWrite(RF_D0, stamp & 0x80);
  digitalWrite(RF_D1, stamp & 0x40);
  digitalWrite(RF_D2, stamp & 0x20);
  digitalWrite(RF_D3, stamp & 0x10);
  digitalWrite(RF_D4, stamp & 0x08);
  digitalWrite(RF_D5, stamp & 0x04);
  digitalWrite(RF_D6, stamp & 0x02);
  digitalWrite(RF_D7, stamp & 0x01);
  digitalWrite(RF_TE, HIGH);
  Serial.printf("Broadcasting stamp 0x%x at timestamp %d...\n", ++stamp, timeLastTransmitUs);
}