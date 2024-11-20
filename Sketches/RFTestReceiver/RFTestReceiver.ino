const int receivePin = 26;
const int pulseSequence[] = {0, 1, 0, 0, 1, 0, 1, 1, 0, 0}; // pulse pattern
const int pulseDelay = 1000; // width of pulse in microseconds
const int sequenceLength = sizeof(pulseSequence) / sizeof(pulseSequence[0]); // length of the pulse array
const int matchThreshold = .9; // Minimum percentage of matches required (e.g., 80%)

void setup() {
  pinMode(receivePin, INPUT);
  Serial.begin(115200); // Initialize serial communication for debugging
}

void loop() {
  // Wait for a HIGH signal to start
  while (digitalRead(receivePin) == LOW) {
    // Stay in this loop until a HIGH signal is detected
  }

  delayMicroseconds((int)pulseDelay/2); // Small delay so we're in the "middle" of the pulse

  // Now read the pulse sequence
  int receivedSequence[sequenceLength];
  for (int i = 0; i < sequenceLength; i++) {
    receivedSequence[i] = digitalRead(receivePin); // Read the pin state
    delayMicroseconds(pulseDelay); // Wait for the expected pulse timing
  }

  // Compare received sequence with the expected sequence
  int matchCount = 0;
  for (int i = 0; i < sequenceLength; i++) {
    if (receivedSequence[i] == pulseSequence[i]) {
      matchCount++; // Count how many values match
    }
  }

  // Calculate the percentage of matches
  float matchPercentage = (float)matchCount/sequenceLength;

  // Check if the match percentage meets the threshold
  if (matchPercentage >= matchThreshold) {
    Serial.println("Sequence matched!");
  }
}