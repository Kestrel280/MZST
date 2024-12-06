#include "../_include/TimerSyncModule.h"

const int transmitPin = 27;

void setup() {
  Serial.begin(115200);
  pinMode(transmitPin, OUTPUT);
}

void loop() {
  unsigned int mask = 0b10000000000000000000000000000000;
  unsigned int val;
  Serial.printf("Broadcasting...");
  for (int i = 0; i < 32; i++) {
    val = (mask & rfKey) > 0;
    digitalWrite(transmitPin, val); // Output the current array value to the pin
    Serial.printf("%d", val);
    mask = mask >> 1;
    delay(rfPulseIntervalMs); // Delay for the pulse duration
  }
  Serial.printf(" Done\n");
  delay(2500); // 1-second delay
}