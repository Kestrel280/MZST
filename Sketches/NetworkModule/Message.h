#ifndef MESSAGE_H
#define MESSAGE_H

// Utilities for handling messages with server

// Message types (char -- range is 0-255)
#define MTYPE_REGISTER       2
#define MTYPE_TOUCHED        100
#define MTYPE_ERROR          33
#define MTYPE_TIMESTAMPRESET 66
#define MTYPE_ACK            111

struct OutboundMessage {
  short type;
  short id;
  uint32_t data;
};

void messageLoop(void* param);
OutboundMessage createOutboundMessage(char type, uint32_t data);
void processIncomingMessage(std::string msg);
void sendMessage(OutboundMessage msg);

#endif
