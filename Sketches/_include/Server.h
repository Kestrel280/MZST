#ifndef SERVER_H
#define SERVER_H

/* 
Server.h -- header-only file containing utilities for handling bidirectional server communications
Import AFTER importing appropriate module header file!
    Module header MUST expose a processModuleSpecificMessage(std::string) function, defined in the module's implementation file
*/

#include <queue>
#include <string>
#include <sstream>
#include <WiFi.h>
#include "Utils.h"
#include "Eeprom_Helpers.h"

/* -----------------------------------------------------------------------------
Prototypes / Interface / Definitions
------------------------------------------------------------------------------*/

// Message types (char -- range is 0-255)
#define MTYPE_REGISTER_USER  1
#define MTYPE_REGISTER_NODE  2
#define MTYPE_REGISTER_TRNS  3
#define MTYPE_TOUCHED        100
#define MTYPE_ERROR          33
#define MTYPE_TIMESTAMPRESET 66
#define MTYPE_ACK            111

// 
struct OutboundMessage {
  short type;
  short id;
  uint32_t data;
};

// Global queue for outbound messages
// Module loop should handle pulling these out and sending
std::queue<OutboundMessage> outboundMessageQueue;

// Global sockets for server and admin communications
WiFiClient serverSocket;                            // Socket for server communication
WiFiClient adminSocket;                             // Socket for pre-server-connection "admin" communications
WiFiServer adminSocketListener;                     // Listener socket for incoming "admin" comms

/*  createOutboundMessage
    Bundles an outgoing message into an OutboundMessage object
    
    char type: one of MTYPE_*
    uint32_t data: associated data. Only used by certain message types
*/
OutboundMessage createOutboundMessage(char type, uint32_t data);

/*  processIncomingMessage
    Handles a message from the server. Dispatches to appropriate logic.
    
    std::string msg: the parsed message from the server.
*/
void processIncomingMessage(std::string msg);

/* sendMessage -- globals: serverSocket
    Writes a message to the server's socket.
    
    OutboundMessage msg: msg (constructed by createOutboundMessage) to send
*/
void sendMessage(OutboundMessage msg);

/* -----------------------------------------------------------------------------
Definitions
------------------------------------------------------------------------------*/

OutboundMessage createOutboundMessage(char type, unsigned long data) {
  OutboundMessage msg;
  msg.type = type;
  msg.id = module.moduleId;
  msg.data = data;
  return msg;
}


void sendMessage(OutboundMessage msg) {
  serverSocket.write_P((char*)(&msg), sizeof(OutboundMessage));
}


void processIncomingMessage(std::string msg) {
  Serial.printf("Received message from server: %s\n", msg.c_str());

  std::stringstream iss(msg);
  std::string command, value;

  std::getline(iss, command, ' '); // Read the first word of the message into 'command'

  /* --- Messages that all modules must be able to process --- */
  if ((command == "RESTART") || (command == "SHUTDOWN")) {
    Serial.printf("Received %s message! Restarting!", command.c_str());
    esp_restart();
  }

  if (command == "REQ_ACK") { // Server is requesting an ACK; send it, then continue
    outboundMessageQueue.push(createOutboundMessage(MTYPE_ACK, currentTimeAbs()));
    std::getline(iss, command, ' ');
  }

  if (command == "SET_EEPROM_VALUE") {
    std::getline(iss, command, ' '); // Second word tells us what value to update
    std::getline(iss, value, ' '); // Third word tells us what the new value is

    if      (command == "NETWORKSSID")      strncpy(module.networkSsid, value.c_str(), sizeof(module.networkSsid));
    else if (command == "NETWORKPASSWORD")  strncpy(module.networkPassword, value.c_str(), sizeof(module.networkPassword));
    else if (command == "SERVERIP")         strncpy(module.serverIp, value.c_str(), sizeof(module.serverIp));
    else if (command == "SERVERPORT")       module.serverPort = (unsigned int) atoi(value.c_str());
    else if (command == "MODULEID")         module.moduleId = (unsigned int) atoi(value.c_str());

    writeEeprom((char*)&module, sizeof(module), 0);
  }

  /* --- Do any module-specific processing required for this message --- */
  (*processModuleSpecificMessage)(msg);
  
  return;
}

#endif