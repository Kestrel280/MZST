#define PUSHBUTTON D10
#define USER_LED D9
#define RF_D0 D8
#define RF_D1 D7
#define RF_D2 D5
#define RF_D3 D4
#define RF_D4 D3
#define RF_D5 D2
#define RF_D6 D1
#define RF_D7 D0
#define RF_TE D6

bool rfKey[8] = {1, 0, 0, 1, 0, 1, 1, 1};

void setup() {
  Serial.begin(115200);
  pinMode(PUSHBUTTON, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(USER_LED, OUTPUT);
  pinMode(RF_D0, OUTPUT);
  pinMode(RF_D1, OUTPUT);
  pinMode(RF_D2, OUTPUT);
  pinMode(RF_D3, OUTPUT);
  pinMode(RF_D4, OUTPUT);
  pinMode(RF_D5, OUTPUT);
  pinMode(RF_D6, OUTPUT);
  pinMode(RF_D7, OUTPUT);
  pinMode(RF_TE, OUTPUT);

  digitalWrite(RF_D0, 1);
  digitalWrite(RF_D1, 0);
  digitalWrite(RF_D2, 0);
  digitalWrite(RF_D3, 1);
  digitalWrite(RF_D4, 0);
  digitalWrite(RF_D5, 1);
  digitalWrite(RF_D6, 1);
  digitalWrite(RF_D7, 1);

  digitalWrite(RF_TE, 0);
}

void loop() {
  bool buttonState = digitalRead(PUSHBUTTON);
  Serial.println(buttonState);
  digitalWrite(RF_TE, buttonState);
  digitalWrite(LED_BUILTIN, !buttonState);
  digitalWrite(USER_LED, buttonState);
}