#ifndef NETWORKMODULE_H
#define NETWORKMODULE_H

#define EEPROM_SIZE 512

#define NWM_MAXSSID_LEN 32
#define NWM_MAXPW_LEN 32

typedef struct _NetworkModule {
	char networkSsid[NWM_MAXSSID_LEN];
	char networkPassword[NWM_MAXPW_LEN];
} NetworkModule;

#endif
