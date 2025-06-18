#include <stdbool.h>

#define SERVER_RECEIVE_BUF_SIZE 1024 /* inbound commands must be smaller than this */
#define SERVER_OBRBUF_SIZE 8

// Client types (char -- range is 0-255)
#define CTYPE_USER          1
#define CTYPE_NODE          2
#define CTYPE_TRNS          3

// Message types (char -- range is 0-255)
#define MTYPE_REGISTER_USER  CTYPE_USER
#define MTYPE_REGISTER_NODE  CTYPE_NODE
#define MTYPE_REGISTER_TRNS  CTYPE_TRNS
#define MTYPE_TOUCHED        100
#define MTYPE_ERROR          33
#define MTYPE_TIMESTAMPRESET 66
#define MTYPE_ACK            111

bool serverConnect(const char* ip, uint16_t port, char ctype, uint16_t id);
void serverMessageLoop();
bool serverSend(uint16_t mtype, uint16_t id, uint32_t data); 