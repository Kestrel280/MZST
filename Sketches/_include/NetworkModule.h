#ifndef NETWORKMODULE_H
#define NETWORKMODULE_H

#define EEPROM_SIZE 512

#define NWM_MAXSSID_LEN 32
#define NWM_MAXPW_LEN 32

#define SERVER_PORT 5000
#define ADMIN_PORT 7777

typedef struct _NetworkModule {
	char networkSsid[NWM_MAXSSID_LEN];
	char networkPassword[NWM_MAXPW_LEN];
    char serverIp[16];
    unsigned short serverPort;
    unsigned short moduleId;
} NetworkModule;

#endif
