#ifndef MODULE_H
#define MODULE_H

#define EEPROM_SIZE 512
#define MODULE_MAXSSID_LEN 32
#define MODULE_MAXPW_LEN 32

#define SERVER_PORT 5000
#define ADMIN_PORT 7777

// Message handler for module-specific messages; defined in module-specific implementation files
void processModuleSpecificMessage(std::string msg);

typedef struct _Module {
	char networkSsid[MODULE_MAXSSID_LEN];
	char networkPassword[MODULE_MAXPW_LEN];
    char serverIp[16];
    unsigned short serverPort;
    unsigned short moduleId;
} Module;

// Global
Module module;

#endif
