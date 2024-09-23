#ifndef NETWORKMODULE_H
#define NETWORKMODULE_H

#define EEPROM_SIZE 512

typedef struct _NetworkModule {
	char networkSsid[32];
	char networkPassword[32];
} NetworkModule;

#endif