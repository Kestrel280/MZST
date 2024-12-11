#ifndef EEPROM_HELPERS_H
#define EEPROM_HELPERS_H

#include <EEPROM.h>

#define EEPROM_SIZE 512

void dumpEeprom() {
  const int bytesPerRow = 32;
  int i = 0;

  Serial.printf("--- Dumping %d bytes from EEPROM ---\n", EEPROM_SIZE);
  for (int row = 0; row < (EEPROM_SIZE / bytesPerRow + 1); row++) {
    Serial.printf("%08x | ", i);
    for (int j = 0; j < bytesPerRow; j++) {
      Serial.printf("%c", EEPROM.read(i));
      i++;
    }
    Serial.printf("\n");
  }
  Serial.printf("--- End of EEPROM ---\n");
}

void readEeprom(char* out, int startByte, int size) {
	for (int i = startByte; i < startByte + size; i++) {
		*(out + i) = EEPROM.read(i);
	}
}

#endif