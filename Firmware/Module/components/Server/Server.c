#include <sys/socket.h>

#include "freertos/semphr.h"
#include "esp_log.h"

#include "Server.h"

#define __server_check_connect_called() if (!sock) {ESP_LOGE(TAG, "%s called before serverConnect()", __PRETTY_FUNCTION__); }

static const char* TAG = "MZST_server";
static int sock = 0;                    // Initialized with call to serverConnect()
static volatile bool connected = false; // Initialized with call to serverConnect(): "lock" on using serverSend() (and possibly other future Server API calls)
static char* serverIp = NULL;           // Initialized with call to serverConnect()
static int serverPort;                  // Initialized with call to serverConnect()
static char ctype;                      // Initialized with call to serverConnect(): type of this module
static uint16_t id;                     // Initialized with call to serverConnect(): id of this module

// Internal helpers for handling outbound messages (thread-safe ring buffer: producer is serverSend, consumer is serverMessageLoop())
SemaphoreHandle_t obrbufLock;
SemaphoreHandle_t obrbufSlots;
int obrbufHead, obrbufTail;
typedef struct _OutboundMessage {
    uint16_t mtype;
    uint16_t id;
    uint32_t data;
} OutboundMessage;
OutboundMessage obrbuf[SERVER_OBRBUF_SIZE];

void __serverReconnect() {
    // If message loop detects a lost connection, attempt to reconnect
    ESP_LOGI(TAG, "Lost connection with server: preparing to reconnect...");

    // Lock the API, to disable serverSend
    connected = false;

    // Flush the outbound message buffer, allowing time for pending serverSend() calls to finish
    while (obrbufHead != obrbufTail) {
        obrbufHead = (obrbufHead + 1) % SERVER_OBRBUF_SIZE;
        xSemaphoreGive(obrbufSlots);
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }

    // Close/cleanup the existing socket
    close(sock);
    sock = 0;

    // Call serverConnect()
    serverConnect(serverIp, serverPort, ctype, id);

    // Re-register with server before re-opening API
    serverSend(MTYPE_REGISTER_NODE, id, 1234);

    // Unlock the API
    connected = true;    
}

bool serverConnect(const char* ip, uint16_t port, char ctype, uint16_t id) {
    struct sockaddr_in dest_addr;
    int eno;

    // Set up outbound message ring buffer
    /* TODO obrbufSlots initialized with maxcount - 1
        somewhat of a bandaid fix -- we check if queue is non-empty by checking tail != head (aka empty is tail == head)...
        but, if the semaphor count is initialized to maxcount, then tail == head is ambiguously full or empty
        so, init'ing with maxcount - 1 means we don't run into this issue, but we also have a queue which is effectively -1 size,
        and i'm worried this might cause sneaky issues later
    */
    obrbufHead = obrbufTail = 0;
    obrbufLock = xSemaphoreCreateMutex();
    obrbufSlots = xSemaphoreCreateCounting(SERVER_OBRBUF_SIZE - 1, SERVER_OBRBUF_SIZE - 1); 

    // Create socket
    inet_pton(AF_INET, ip, &dest_addr.sin_addr); // Convert IP as string to packed byte format, store in dest_addr.sin_addr
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);

    // Connect: create a socket, try connect() -- if fail, create new socket (obnoxious, but I think consequence of lwip doing done_socket() on connect() fail) and try again until success
    ESP_LOGI(TAG, "Connecting to server...");
    while (1) {
        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock < 0) { ESP_LOGE(TAG, "Failure creating socket: errno %d", errno); return false; }
        if (connect(sock, (struct sockaddr*)&dest_addr, sizeof(dest_addr)) < 0) { 
            ESP_LOGI(TAG, "...connecting to server... (ip = %s, port = %d, errno = %d (%s), sock = %d)", ip, port, errno, strerror(errno), sock);
            close(sock);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        } else break;
    };
    ESP_LOGI(TAG, "Successfully connected to server at ip %s, port %d", ip, port);

    serverPort = port;
    serverIp = ip;      // Note -- caller designates the char*, and we re-use it!!! Caller must not change the data at the ptr

    connected = true; // Open API (allow other threads to post outbound messages)

    return true;
}

void serverMessageLoop(void (*processCommandFunction)(char* cmd)) {
    /*
    Should run in its own task

    Infinite loop:
        send any messages in outbound queue,
        receive data into inbound buffer,
        and dispatch '\n'-delineated commands from inbound buffer to processCommand()
        This WILL block on processCommand(), so we don't need to allocate a separate buffer
            -- we can just pass the string as-is
    */

    __server_check_connect_called(); // Ensure that serverConnect() was called first

    char buf[SERVER_RECEIVE_BUF_SIZE];  // Inbound buffer
    int i;
    ssize_t msgSize;

    ESP_LOGI(TAG, "Server message loop running on core %d\n", xPortGetCoreID());

    while (1) {
        // --- 1. Check outbound message queue and send any messages
        while (obrbufHead != obrbufTail) { // Check if there's any outbound messages in the queue (as the only consumer, we can do this safely, without needing to acquire any locks or anything)
            if (send(sock, &(obrbuf[obrbufHead]), sizeof(obrbuf[0]), 0) < 0) {
                ESP_LOGE(TAG, "Failure sending outbound message with error code %d (%s)", errno, strerror(errno));
                if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                    break;
                } else if ((errno == ENOTCONN) || (errno == ECONNRESET)) {
                    __serverReconnect();
                    continue;
                } else {
                    ESP_LOGE(TAG, "Unhandled error in serverMessageLoop: 1st recv() got errno %d (%s)", errno, strerror(errno)); 
                }
            } else ESP_LOGI(TAG, "Sent message to server with mtype = %d, id = %d, data = %ld", obrbuf[obrbufHead].mtype, obrbuf[obrbufHead].id, obrbuf[obrbufHead].data);
            obrbufHead = (obrbufHead + 1) % SERVER_OBRBUF_SIZE;
            xSemaphoreGive(obrbufSlots);
        }

        // --- 2. Process inbound data
        while (1) {
            // Peek the socket: if no data, break -- if data, store it in buf and continue // TODO check to ensure errors are either EAGAIN or EWOULDBLOCK
            msgSize = recv(sock, buf, SERVER_RECEIVE_BUF_SIZE, MSG_PEEK | MSG_DONTWAIT);
            if (msgSize < 0) {
                if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                    break;
                } else if ((errno == ENOTCONN) || (errno == ECONNRESET)) {
                    ESP_LOGE(TAG, "Rare case in server.c: connection reset while parsing inbound data. Recovery from this case has not been tested!");
                    __serverReconnect();
                    continue; // TODO can't continue from here, in inner loop. goto...?
                } else {
                    ESP_LOGE(TAG, "Unhandled error in serverMessageLoop: 1st recv() got errno %d (%s)", errno, strerror(errno)); 
                }
            }
            
            // If execution reaches this point, we have data in buf
            // If last char received isn't '\n', scan from beginning until reaching one (if it IS one, skip the scan and just set 'i' to the index of the last char)
            // If we don't find one, break from this inner loop -- we don't yet have a full command to process
            // (Note: Can't scan backwards, since that could accidentally yield multiple commands)
            if (!(buf[i = (msgSize - 1)] == '\n')) {
                for (i = 0; i < msgSize; i++) { if (buf[i] == '\n') break; }
                if (i == msgSize) break;
            }

            // If execution reaches this point (an '\n' was found), buf[0:i] now contains a command
            msgSize = recv(sock, buf, msgSize, MSG_DONTWAIT); // Somewhat redundant, but necessary: do another recv, to get the command out of the socket buffer
            if (msgSize < 0) {
                if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                    break;
                } else if ((errno == ENOTCONN) || (errno == ECONNRESET)) {
                    __serverReconnect();
                    continue;
                } else {
                    ESP_LOGE(TAG, "Unhandled error in serverMessageLoop: 1st recv() got errno %d (%s)", errno, strerror(errno)); 
                }
            }
            buf[i] = '\0'; // Replace the newline with a null terminator: allows processMessage() to treat the buf as a string directly
            processCommandFunction(buf);
        }
        taskYIELD();
    }
}

bool serverSend(uint16_t mtype, uint16_t id, uint32_t data) {
    // Warning: this function can block, if the outbound message queue is full!

    OutboundMessage obm;
    obm.mtype = mtype;
    obm.id = id;
    obm.data = data;

    __server_check_connect_called();
    if (!connected) return false;

    // Block until there's space in the obrbuf
    xSemaphoreTake(obrbufSlots, portMAX_DELAY);

    // Block until acquiring obrbuf lock
    xSemaphoreTake(obrbufLock, portMAX_DELAY);

    // Post message in queue
    obrbuf[obrbufTail] = obm;
    obrbufTail = (obrbufTail + 1) % SERVER_OBRBUF_SIZE; // Wrap ring buffer tail if necessary

    // Release obrbuf lock
    xSemaphoreGive(obrbufLock);

    return connected; // Only false if we lost connection partway through sending 
}