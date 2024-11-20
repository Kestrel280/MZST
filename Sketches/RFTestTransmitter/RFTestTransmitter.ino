const int transmitPin = 27;
const int pulseSequence[] = {1, 0, 1, 0, 1, 0, 1, 1, 0, 0}; // pulse pattern
const int pulseDelay = 1000; // width of pulse in microseconds
const int sequenceLength = sizeof(pulseSequence) / sizeof(pulseSequence[0]); // length of the pulse array


void setup() {
  pinMode(transmitPin, OUTPUT);
}

void loop() {
  for (int i = 0; i < sequenceLength; i++) {
    digitalWrite(transmitPin, pulseSequence[i]); // Output the current array value to the pin
    delayMicroseconds(pulseDelay); // Delay for the pulse duration
  }
  delay(1000); // 1-second delay
}