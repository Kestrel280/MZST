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

/* serverConnect: (const char*, uint16_t, char, uint16_t) -> (bool)
Sets up internal outbound-message buffer,
    establishes socket for server connection,
    connects to it,
    and opens the Server API ('connected' bool) (which is currently only used for accessing serverSend()).
*/
bool serverConnect(const char* ip, uint16_t port, char ctype, uint16_t id);

/* serverMessageLoop: (*processCommandFunction)(char*) -> ()
Thread/task function.
Runs a continuous loop of:
    1. Sending any pending outbound messages to the server (posted using serverSend())
    2. Receiving messages from server, parsing them, and processing them using processCommandFunction
Note that the prototype is of type void fn(void), but the implementation should expect a function pointer to processCommandFunction(char*)!
*/
void serverMessageLoop();

/* serverSend: (uint16_t, uint_16t, uint32_t) -> (bool)
Sends a message to the server.
Implementation actually posts the message to a buffer, which is processed by serverMessageLoop().
Access to the buffer (via this function) is thread-safe and CAN BLOCK if the buffer is full.
*/
bool serverSend(uint16_t mtype, uint16_t id, uint32_t data); 