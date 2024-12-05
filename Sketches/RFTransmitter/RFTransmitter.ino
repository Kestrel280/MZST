#include "../_include/TimerSyncModule.h"

const int transmitPin = 27;

void setup() {
  Serial.begin(115200);
  pinMode(transmitPin, OUTPUT);
}

void loop() {
  Serial.printf("Broadcasting...");
  for (int i = 0; i < rfKeyLength; i++) {
    digitalWrite(transmitPin, rfKey[i]); // Output the current array value to the pin
    delayMicroseconds(rfPulseIntervalUs); // Delay for the pulse duration
  }
  Serial.printf(" Done\n");
  delay(5000); // 5-second delay
}