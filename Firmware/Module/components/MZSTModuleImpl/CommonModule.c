#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_system.h"

#include "MZSTModuleImpl.h"
#include "NVSUtil.h"
#include "Server.h"

extern uint16_t mid;    // main.c, loaded from NVS
static const char* TAG = "MZST_CommonModule";

void processCommandCommon(char* cmd) {
    /* Nominally entered by Server's messageLoop()
    cmd should be a SINGLE, null-terminated command
        (note that the server sends newline-terminated commands;
        but by the time the command reaches this function,
        the newline should have been replaced with a null byte)
    */
    char *token, *s1, *s2;
    ESP_LOGI(TAG, "Processing cmd [%s]", cmd);

    // No while-loop to process commands: input is already a well-formed single command
    // strtok() extracts the first space-separated token from the command
    //  The space is replaced with a null byte, so token is a usable null-terminated string
    //  The value of cmd is advanced to point to the start of the NEXT token
    // Subsequent calls for this command should specify NULL as first arg
    token = strtok(cmd, " ");

    /* Special cases: single if-elseif-else block */
    // Restart/shutdown
    if ((strcmp(token, "RESTART") == 0) || (strcmp(token, "SHUTDOWN") == 0)) {
        esp_restart();
    }

    /* Prefixes: individual if blocks (TODO if we define more prefixes, we should also specify an order that they must occur in)*/
    // REQ_ACK: post an ACK response, get next token
    if (strcmp(token, "REQ_ACK") == 0) {
        serverSend(MTYPE_ACK, mid, 1234); // TODO time
        token = strtok(NULL, " ");
    }

    /* Common commands: single if-elseif-else block
        'else' case falls through to module-specific implementation processCommandSpecific() */
    // NVS write
    if (strcmp(token, "SET_EEPROM_VALUE") == 0) {
        s1 = strtok(NULL, " "); // What value to update
        s2 = strtok(NULL, " "); // New value

        bool success = false;
        if (strcmp(s1, "NETWORKSSID") == 0)             success = nvsSetStr(NVS_NTWK_SSID_KEY, s2);
        else if (strcmp(s1, "NETWORKPASSWORD") == 0)    success = nvsSetStr(NVS_NTWK_PSWD_KEY, s2);
        else if (strcmp(s1, "SERVERIP") == 0)           success = nvsSetStr(NVS_SERVER_IP_KEY, s2);
        else if (strcmp(s1, "SERVERPORT") == 0)         success = nvsSetInt(NVS_SERVER_PORT_KEY, atoi(s2));
        else if (strcmp(s1, "MODULEID") == 0)           success = nvsSetInt(NVS_VERSION_KEY, atoi(s2));

        if (success && nvsCommit()) {
            ESP_LOGI(TAG, "Wrote value [%s] to NVS [%s] field", s2, s1);
        } else {
            ESP_LOGI(TAG, "Failure writing value [%s] to NVS [%s] field", s2, s1);
        }
    }

    else {
        ESP_LOGI(TAG, "... no common case found for command, falling through to module-specific handler for cmd [%s]", cmd);
        processCommandSpecific(token);
    }
}