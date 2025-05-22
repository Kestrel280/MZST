#ifndef EEPROM_HELPERS_H
#define EEPROM_HELPERS_H

/* 
EepromHelpers.h -- header-only file containing utilities for handling module EEPROM
*/

#include <EEPROM.h>

#define EEPROM_SIZE 512

void dumpEeprom() {
  const int bytesPerRow = 32;
  int i = 0;
  int j = 0;

  Serial.printf("--- Dumping %d bytes from EEPROM ---\n", EEPROM_SIZE);
  for (int row = 0; row < (EEPROM_SIZE / bytesPerRow + 1); row++) {
    Serial.printf("%08x | ", i);
    for (j = 0; j < bytesPerRow; j++) {
      Serial.printf("%c", EEPROM.read(i));
      i++;
    }
    Serial.printf(" | ");
    i -= bytesPerRow;
    for (j = 0; j < bytesPerRow; j++) {
      Serial.printf("%2x ", EEPROM.read(i));
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

void wipeEeprom() {
  int i;
  for (i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
}

void writeEeprom(char* dataToFlash, int bytesToFlash, int offset) {
  int i;

  // Write data at offset
  for (i = offset; (i < bytesToFlash) && (i < EEPROM_SIZE); i++) {
    EEPROM.write(i, *(dataToFlash + i));
  }
  EEPROM.commit();
}

#endif