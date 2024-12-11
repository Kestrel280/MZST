#ifndef TRANSMITTERMODULE_H
#define TRANSMITTERMODULE_H

#define EEPROM_SIZE 512

#define TM_MAXSSID_LEN 32
#define TM_MAXPW_LEN 32

typedef struct _TransmitterModule {
	char networkSsid[TM_MAXSSID_LEN];
	char networkPassword[TM_MAXPW_LEN];
} TransmitterModule;

#endif
