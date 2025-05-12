#include "Message.h"

// Bundles an outbound message into an OutboundMessage instance
OutboundMessage createOutboundMessage(char type, unsigned long data) {
  OutboundMessage msg;
  msg.type = type;
  msg.id = networkModule.moduleId;
  msg.data = data;
  return msg;
}

// Delivers an OutboundMessage to the server through global 'serverSocket'
// GLOBALS: serverSocket
void sendMessage(OutboundMessage msg) {
  serverSocket.write_P((char*)(&msg), sizeof(OutboundMessage));
}

// Handles incoming message and processes accordingly
// GLOBALS: outboundMessageQueue, pendingServerUntouch, networkModule, state, stateData
void processIncomingMessage(std::string msg) {
  Serial.printf("Received message from server: %s\n", msg.c_str());

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

// Master message loop. Intended to run in its own thread
void messageLoop(void* param) {
  Serial.printf("Message loop running on core %d\n", xPortGetCoreID());
  
  // Normally aliased to touch_val_t
  //   which is uint16_t on ESP32, uint32_t on ESP32s2/s3
  // Explicitly use uint32_t because the behavior of touchPadInterrupt() is different for both anyway
  // ('threshold' argument is a true threshold for ESP32, but on ESP32s2/s3 it's an INCREMENT value)
  uint32_t _touchVal, touchIncrement, touchThreshold;
  _touchVal = touchRead(TP_PIN);
  touchIncrement = _touchVal / 8;
  touchThreshold = _touchVal + touchIncrement;
  sendMessage(createOutboundMessage(MTYPE_REGISTER, touchThreshold));
  touchAttachInterrupt(TP_PIN, &touchpadCallback, touchIncrement);
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
      inTouchCooldown = !(((currentTimeAbs() - timeLastTouchUs) > UNTOUCH_TIMEOUT_US) && (touchRead(TP_PIN) < (touchThreshold - touchIncrement / 2)));
    }
    // Unset the pendingServerUntouch and inTouchCooldown flags
    if (pendingServerUntouch && !inTouchCooldown) {
      vTaskDelay((TickType_t) (100 / portTICK_PERIOD_MS)); // ~100ms delay before unsetting flags, to avoid re-touch on release. TODO do this better
      inTouchCooldown = false;
      pendingServerUntouch = false;
      writeLed(stateData->colorIdle);
    }
  }
}
