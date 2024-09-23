#include <EEPROM.h>
#include "../_include/NetworkModule.h"
#include "../_include/Eeprom_Helpers.h"

#define SERIAL_BAUDRATE 115200
#define EEPROM_SIZE 512

#define RED_PIN 19
#define GREEN_PIN 18
#define BLUE_PIN 21
#define CAPSENS_PIN 33

// Store a buffer of the n most recent values from the capsens
// We will use the AVG of this buffer as our value to compare against a threshold, providing a bit of hysteresis
// Increasing the size of the buffer results in a smoother/more reliable output, but also increases lag
#define CAPSENS_BUFFER_LENGTH 5
double gCapsensBuf[CAPSENS_BUFFER_LENGTH]; // Buffer of the n most recent values from the capsens
bool gTouched = false;                     // PRIMARY OUTPUT: Is the sensor touched
const float gCapsensThreshold = 15.0;      // Threshold value for identifying when the sensor is touched
double gCapsensBufAvg;                     // Avg value of the buffer
int gCapsensBufIdx = 0;                    // Helper indexer for the buffer

void setup() {
  Serial.begin(SERIAL_BAUDRATE);
  EEPROM.begin(EEPROM_SIZE);

  // Initialize capsens buffer and avg
  for (int i = 0; i < CAPSENS_BUFFER_LENGTH; i++) { gCapsensBuf[i] = 25.0; }
  gCapsensBufAvg = gCapsensBuf[0];

  ledcAttach(RED_PIN, 5000, 8);
  ledcAttach(GREEN_PIN, 5000, 8);
  ledcAttach(BLUE_PIN, 5000, 8);

  Serial.printf("--- initialized ---\n");

  dumpEeprom();

  // Connect to wifi
  //while (!connectToWifi()) {};
}

void loop() {
  bool newTouched = checkpointTouched();
  if (gTouched != newTouched) { Serial.printf("%s\n", newTouched ? "Touched" : "Released"); }
  gTouched = newTouched;

  if (gTouched) {
    displayColorLED(0, 255, 0);
  } else {
    displayColorLED(255, 0, 0);
  }
}

bool checkpointTouched() {
  bool out;
  
  // Get current value from capsens and update buffer
  //  Update avg without recomputing overall avg; just subtract old component and add new component
  double newCapsensVal = touchRead(CAPSENS_PIN);
  gCapsensBufAvg = gCapsensBufAvg + (newCapsensVal - gCapsensBuf[gCapsensBufIdx]) / (double)CAPSENS_BUFFER_LENGTH;
  gCapsensBuf[gCapsensBufIdx] = newCapsensVal;
  gCapsensBufIdx = (gCapsensBufIdx + 1) % CAPSENS_BUFFER_LENGTH;

  // Update 'touched'
  // Compare new avg vs threshold
  //  If we were previously touching, add a little headroom to the threshold
  out = gCapsensBufAvg < (gTouched ? gCapsensThreshold + 3.0 : gCapsensThreshold);

  // Debug
  // Serial.printf("%f, %f: %s\n", capsensThreshold, capsensBufAvg, out ? "TOUCHED" : "NOT TOUCHED");

  return out;
}

void displayColorLED(int r, int g, int b) {
  ledcWrite(RED_PIN, r);
  ledcWrite(GREEN_PIN, g);
  ledcWrite(BLUE_PIN, b);
}

bool connectToWifi() {
  return false;
}
