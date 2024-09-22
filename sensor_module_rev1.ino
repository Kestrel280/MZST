#define RED_PIN 19
#define GREEN_PIN 18
#define BLUE_PIN 21

#define CAPSENSE_PIN 33

float capsense_threshold = 14;
bool last_state = false;

long capsense_val = 10;
long capsense_val_old = 10;
long capsense_val_older = 10;

void setup() {
   Serial.begin(115200);

   ledcAttach(RED_PIN, 5000, 8);
   ledcAttach(GREEN_PIN, 5000, 8);
   ledcAttach(BLUE_PIN, 5000, 8);

   pinMode(POT_PIN, INPUT);
}

void loop() {
  if(checkpointTouched()){
    displayColorLED(0,255,0);
  }
  else{
    displayColorLED(255,0,0);
  }
}

bool checkpointTouched(){
  long capsense_val = touchRead(CAPSENSE_PIN);
  long capsense_3avg = ((capsense_val+capsense_val_old+capsense_val_older)/3);
  Serial.print(capsense_threshold);
  Serial.print(",");
  Serial.println(capsense_3avg);

  if(capsense_3avg < capsense_threshold){
    if(!last_state){
      capsense_threshold = capsense_threshold + 6;
    }
    last_state = true;
    capsense_val_older = capsense_val_old;
    capsense_val_old = capsense_val;
    return true;
  }
  else{
    if(last_state){
      capsense_threshold = capsense_threshold - 6;
    }
    last_state = false;
    capsense_val_older = capsense_val_old;
    capsense_val_old = capsense_val;
    return false;
  }
}

void displayColorLED(int r, int g, int b){
  ledcWrite(RED_PIN, r);
  ledcWrite(GREEN_PIN, g);
  ledcWrite(BLUE_PIN, b);
}
