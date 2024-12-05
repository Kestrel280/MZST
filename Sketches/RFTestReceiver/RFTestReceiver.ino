#include "../_include/TimerSyncModule.h"

const int rfReceivePin = 26;

bool circularBufferMatchesKey(int* buffer, int* key, int startIdx, int length, int allowableMisses = 0) {
  int misses = 0;
  for (int i = 0; i < length; i++) {
    if (buffer[(startIdx + i) % length] != key[i]) {
      misses++;
    }

    if (misses > allowableMisses) { return false; }
  } // End for loop

  return true;
}

void setup() {
  pinMode(rfReceivePin, INPUT);
  Serial.begin(115200); // Initialize serial communication for debugging
}

void loop() {
  int buffer[rfKeyLength]; // Circular buffer to receive incoming signals
    int bufferIdx;
    int j;
    for (j = 0; j < rfKeyLength; j++) {
      buffer[j] = -1;
    }
    
    while(true) {
      buffer[bufferIdx] = digitalRead(rfReceivePin);
      Serial.printf("%d", buffer[bufferIdx]);
      
      // Check if buffer, starting from NEXT value, matches the key
      // If it does, reset timestamp and send a timestamp-reset message to server
      // note: buffer overflows are checked in circularBufferMatchesKey(), don't need to check here
      if (circularBufferMatchesKey(buffer, rfKey, bufferIdx + 1, rfKeyLength, rfKeyAllowableMisses)) {
        Serial.printf("Received matching sequence\n");
      }

      bufferIdx = (bufferIdx + 1) % rfKeyLength;
      if (!bufferIdx) { Serial.printf("\n"); }
      delayMicroseconds(rfPulseIntervalUs); // TODO use proper delay timing
    }
}