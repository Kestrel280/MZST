#include <sys/socket.h>
#include <stdio.h>

#include <freertos/FreeRTOS.h>
#include "esp_log.h"

#include "AdminListener.h"

static const char* TAG = "MZST_AdminListener";

void adminListenerLoop(void (*processCommandFunction)(char* cmd)) {
    char buf[ADMIN_RECEIVE_BUF_SIZE];
    struct sockaddr_in dest_addr, client_addr;
    socklen_t client_addr_len;
    int adminSock;
    int clientFd;
    ssize_t msgSize;

    ESP_LOGI(TAG, "Admin listener loop running on core %d\n", xPortGetCoreID());

    // Start a listener socket
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(ADMIN_LISTEN_PORT);
    adminSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (adminSock == -1) {
        ESP_LOGE(TAG, "Failure with AdminListener creating socket, errno = %d", errno);
        vTaskDelete(NULL);
    }
    /*
    if (fcntl(adminSock, F_SETFL, fcntl(adminSock, F_GETFL, 0) | O_NONBLOCK) == -1) {
        ESP_LOGE(TAG, "Failure with AdminListener setting socket to O_NONBLOCK, errno = %d", errno);
        close(adminSock);
        vTaskDelete(NULL);
    }*/
    if (bind(adminSock, (struct sockaddr*)&dest_addr, sizeof(dest_addr)) == -1) {
        ESP_LOGE(TAG, "Failure with AdminListener bind(), errno = %d", errno);
        close(adminSock);
        vTaskDelete(NULL);
    }
    if (listen(adminSock, 5) == -1) {
        ESP_LOGE(TAG, "Failure with AdminListener listen(), errno = %d", errno);
        close(adminSock);
        vTaskDelete(NULL);
    };

    while (1) {
        // Accept a connection
        clientFd = accept(adminSock, (struct sockaddr*)&client_addr, &client_addr_len);
        if (clientFd == -1) {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) vTaskDelay(250 / portTICK_PERIOD_MS);
            else {
                ESP_LOGE(TAG, "Failure with AdminListener accept(), errno = %d", errno);
                close(adminSock);
                vTaskDelete(NULL);
            }
        } else {
            msgSize = recv(clientFd, buf, ADMIN_RECEIVE_BUF_SIZE, 0);
            buf[msgSize] = '\x00';
            ESP_LOGI(TAG, "Admin sent cmd '%s'", buf);
            processCommandFunction(buf);
            close(clientFd);
        }
    }
}
