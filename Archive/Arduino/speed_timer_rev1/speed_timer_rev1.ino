#include "SoundData.h"
#include "XT_DAC_Audio.h"

/***** Sound Definitions *****/
XT_Wav_Class Ding(ding);
XT_Wav_Class Success(success);
XT_DAC_Audio_Class DacAudio(26, 0);

#define RED_PIN 19
#define BLUE_PIN 21
#define GREEN_PIN 18
#define BUZZER_PIN 25
#define POT_PIN 34

#define RED_CHANNEL 1
#define BLUE_CHANNEL 2
#define GREEN_CHANNEL 0
#define BUZZER_CHANNEL 3

float capsense_threshold = 14;
bool last_state = false;

long capsense_val = 10;
long capsense_val_old = 10;
long capsense_val_older = 10;

void setup() {
   Serial.begin(115200);

   ledcSetup(RED_CHANNEL, 5000, 8);
   ledcSetup(BLUE_CHANNEL, 5000, 8);
   ledcSetup(GREEN_CHANNEL, 5000, 8);
   
   ledcAttachPin(GREEN_PIN, GREEN_CHANNEL);
   ledcAttachPin(RED_PIN, RED_CHANNEL);
   ledcAttachPin(BLUE_PIN, BLUE_CHANNEL);

   pinMode(POT_PIN, INPUT);
}

void loop() {
  DacAudio.FillBuffer();               

  if(checkpointTouched()){
    displayColorLED(0,255,0);
    if(Ding.Playing==false)      
    DacAudio.Play(&Ding);
  }
  else{
    displayColorLED(255,0,0);
  }
  //delay(1);
}

bool checkpointTouched(){
  long capsense_val = touchRead(4);
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
  ledcWrite(RED_CHANNEL, r);
  ledcWrite(GREEN_CHANNEL, g);
  ledcWrite(BLUE_CHANNEL, b);

  //PURPLE COLOR =====>Setup Checkpoints
  //YELLOW COLOR =====>Set Course
  //RED, GREEN COLOR =====>In Run
  //BLUE COLOR =====>Idle state
}
