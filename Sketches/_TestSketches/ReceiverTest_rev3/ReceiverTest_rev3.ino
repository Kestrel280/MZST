#define RED_PIN 2
#define GREEN_PIN 3
#define BLUE_PIN 4

#define CAPSENSE_PIN 7

#define AUDIO_PIN 1
#define AUDIO_EN_PIN 5

#define RF_D0 42
#define RF_D1 41
#define RF_D2 40
#define RF_D3 39
#define RF_D4 38
#define RF_D5 37
#define RF_D6 36
#define RF_D7 35
#define RF_VT 45

/* if not 0, just flash colors and print to serial, for testing; bypass the RF-based LEDs */
#define SERIALTEST 0

float capsense_threshold = 120000;
bool last_state = false;
bool rfKey[8] = {1, 0, 0, 1, 0, 1, 1, 1};
bool readKey[8];

long capsense_val = 10;
long capsense_val_old = 10;
long capsense_val_older = 10;

unsigned int i = 2000000000;
char mode = 0;

void setup() {
   Serial.begin(921600);

   ledcAttach(RED_PIN, 5000, 8);
   ledcAttach(GREEN_PIN, 5000, 8);
   ledcAttach(BLUE_PIN, 5000, 8);

   pinMode(RF_D0, INPUT_PULLDOWN);
   pinMode(RF_D1, INPUT_PULLDOWN);
   pinMode(RF_D2, INPUT_PULLDOWN);
   pinMode(RF_D3, INPUT_PULLDOWN);
   pinMode(RF_D4, INPUT_PULLDOWN);
   pinMode(RF_D5, INPUT_PULLDOWN);
   pinMode(RF_D6, INPUT_PULLDOWN);
   pinMode(RF_D7, INPUT_PULLDOWN);
   pinMode(RF_VT, INPUT_PULLDOWN);

   digitalWrite(AUDIO_PIN, 0);
   digitalWrite(AUDIO_EN_PIN, 0);
}

void loop() {

  bool readKey[8];
  readKey[0] = digitalRead(RF_D0);
  readKey[1] = digitalRead(RF_D1);
  readKey[2] = digitalRead(RF_D2);
  readKey[3] = digitalRead(RF_D3);
  readKey[4] = digitalRead(RF_D4);
  readKey[5] = digitalRead(RF_D5);
  readKey[6] = digitalRead(RF_D6);
  readKey[7] = digitalRead(RF_D7);

  // Print key to serial monitor (MSB first)
  // Serial.print("RF Key: ");
  // for (int i = 7; i >= 0; i--) {
  //   Serial.print((int)readKey[i]);
  // }
  // Serial.println();
  // Serial.print("VT: ");
  // Serial.println(digitalRead(RF_VT));

  // Compare the keys
  bool match = true;
  for (int i = 0; i < 8; i++) {
    if (readKey[i] != rfKey[i]) {
      match = false;
      break;
    }
  }

#if SERIALTEST > 0
  if (!(++i % (2 << 14))) {
    mode = ~mode;
    Serial.printf("%3d | %10u\n", mode, i);
  }
  if (mode) {
    displayColorLED(255, 0, 0);
  } else {
    displayColorLED(0, 255, 0);
  }
#else
  // Light LED accordingly
  if (match) {
    displayColorLED(255, 0, 0);  // Green
  } else {
    displayColorLED(0, 255, 0);  // Red
  }
#endif  
}

void displayColorLED(int r, int g, int b){
  ledcWrite(RED_PIN, r);
  ledcWrite(GREEN_PIN, g);
  ledcWrite(BLUE_PIN, b);
}
