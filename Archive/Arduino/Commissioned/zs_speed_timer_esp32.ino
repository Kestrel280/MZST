#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <Ticker.h>
#include <String>
#include "SoundData.h"
#include "XT_DAC_Audio.h"

/********* Global Variables **********/
int numberOfNetworks = 0;
String ssidsString;
String phoneWebServerURL = "-1";
boolean oneSecondPassed = false;
int counter = 0;
int statusCounter = 0;
#define STATUS_CHECK_TIME 4   //After this many seconds a status check will be sent to the web server
int potentiometerPin = 34;

/**** Touch Sensor Definitions *******/
int buttonState = 1;
int lastButtonState = 1;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 40; //The bouncing time of the touch sensor can be adjusted here

#define THRESHOLD_LOWER_LIMIT 10
#define THRESHOLD_UPPER_LIMIT 20
int TOUCH_THRESHOLD = THRESHOLD_LOWER_LIMIT;

/***** Sound Definitions *****/
XT_Wav_Class Ding(ding);
XT_Wav_Class Success(success);
XT_DAC_Audio_Class DacAudio(26, 0);

/********** Checkpoint ID **********/
#define CHECKPOINT_ID "001A"      //Change this id according to the below data

/*
   001A
   001B
   001C
   001D
   001D
*/

#define RESET_CREDENTIALS_DURATION 10

/*********** Objects **************/
WebServer server(80);
Ticker ticker;

/*********** Pin Definitions **************/
const int capTouchButton = 4;

/********** RGB Pin Definitions ***********/
const int redPin = 19;
const int greenPin = 21;
const int bluePin = 18;

const int redChannel = 0;
const int greenChannel = 1;
const int blueChannel = 2;

const int freq = 5000;
const int ledChannel = 0;
const int resolution = 8;

/****** Function Prototypes ******/
bool wait10SecondsForWifi(void);
void setupAP(void);
void createWebServer(void);
void resetSavedCredentials(void);
void oneSecondPassedFun(void);
void resetCredentialsButtonFunction(void);
void buttonPressFunction(void);
void sendButtonPressEventRequest(void);
void statusCheck(void);
void updateStatus(String);
void displayColorLED(int r, int g, int b) ;
void buttonPressResult(String);
void touchThresholdOperation(void);

void setup() {
  Serial.begin(115200);

  /************** EEPROM Setup *****************/
  EEPROM.begin(512); //Initialasing EEPROM
  //resetSavedCredentials(); //Uncomment to clear the saved credentials in EEPROM

  /*********** RGB LED Setup *************/
  ledcSetup(redChannel, 5000, 8);
  ledcAttachPin(redPin, redChannel);

  ledcSetup(greenChannel, 5000, 8);
  ledcAttachPin(greenPin, greenChannel);

  ledcSetup(blueChannel, 5000, 8);
  ledcAttachPin(bluePin, blueChannel);

  //Initially the RGB LED will be off
  ledcWrite(redChannel, 0);
  ledcWrite(greenChannel, 0);
  ledcWrite(blueChannel, 0);

  /************** Timer Setup ************/
  ticker.attach(1, oneSecondPassedFun);

  /************** Read EEPROM for SSID  from 0-32 ******************/
  Serial.println("Reading EEPROM SSID...");
  String eepromSSID;

  for (int i = 0; i < 32; ++i) {
    eepromSSID += char(EEPROM.read(i));
  }

  Serial.print("Saved SSID: ");
  Serial.println(eepromSSID);
  Serial.println();
  /*****************************************************************/

  /************** Read EEPROM for Password  from 32-64 ******************/
  Serial.println("Reading EEPROM Password...");
  String eepromPassword = "";

  for (int i = 32; i < 64; ++i) {
    eepromPassword += char(EEPROM.read(i));
  }

  Serial.print("Saved Password: ");
  Serial.println(eepromPassword);
  Serial.println();
  /*****************************************************************/

  /************** Read EEPROM for Password  from 64-96 ******************/
  Serial.println("Reading Phone Webserver URL...");
  String eepromURL = "";
  for (int i = 64; i < 96; ++i) {

    if (char(EEPROM.read(i)) != '\0') {  //Dont add null character
      eepromURL += char(EEPROM.read(i));
    }
  }

  Serial.print("Saved web-server URL: ");
  Serial.println(eepromURL);
  Serial.println();

  phoneWebServerURL = eepromURL;  //setting the webserver url read from eeprom to global variable
  /*****************************************************************/

  /******** Trying to connect to Wi-Fi ***********/
  WiFi.mode(WIFI_STA);
  WiFi.begin(eepromSSID.c_str(), eepromPassword.c_str());

  if (wait10SecondsForWifi()) {
    Serial.println();
    Serial.println("Succesfully Connected!!!");

    Serial.print("Local IP: ");
    Serial.println(WiFi.localIP());

    //Connection Successful Sound
    DacAudio.Play(&Success);
    for (uint32_t i = 0; i < 5000; i++) {
      DacAudio.FillBuffer();
      delay(1);
    }

  } else {
    Serial.println("Staring the Web-Server to get Wi-Fi Credentials....");
    setupAP();

    Serial.println("Waiting");
  }

  Serial.println();

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
    server.handleClient();
  }

}

/************************************************* LOOP *******************************************/
void loop() {
  resetCredentialsButtonFunction();
  buttonPressFunction();
  statusCheck();            //Sends a request to the phone web-server for status every set number of seconds
  touchThresholdOperation();  //Potentiometer touch threshold operation
}

//Wait 10 seconds for ESP to connect to Wi-Fi
bool wait10SecondsForWifi(void) {
  int c = 0;
  Serial.println("Waiting for Wifi to connect");

  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED) {
      return true;
    }
    delay(500);
    Serial.print("*");
    c++;
  }

  Serial.println("");
  Serial.println("Connect timed out, opening AP");
  return false;
}

void setupAP() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  numberOfNetworks = WiFi.scanNetworks();  //San for Wi-Fi Networks
  Serial.println("Scan done");

  if (numberOfNetworks == 0) {
    Serial.println("No networks found");

  } else {
    Serial.print(numberOfNetworks);
    Serial.println(" networks found");

    for (int i = 0; i < numberOfNetworks; ++i) {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.println(")");
      delay(10);
    }
  }
  Serial.println("");

  ssidsString = "";
  for (int i = 1; i <= numberOfNetworks; ++i) {
    ssidsString += "\"";
    ssidsString += "n";
    ssidsString += i;
    ssidsString += "\"";

    ssidsString += ":\"";
    ssidsString += WiFi.SSID(i - 1);
    ssidsString += "\"";

    if (i != numberOfNetworks) {
      ssidsString += ", ";
    }
  }
  delay(100);

  WiFi.softAP(CHECKPOINT_ID);   //Start ESP Hotspot (Access Point)
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected");
  }

  createWebServer();

  // Start the server
  server.begin();
  Serial.println("Server started");
}

// Creating Webserver for Wi-Fi Manager
void createWebServer() {
  server.on("/", []() {
    String responseSsids = "";

    if (numberOfNetworks > 0) {
      responseSsids = "{\"n\":";
      responseSsids += numberOfNetworks;
      responseSsids += ", ";
      responseSsids += ssidsString;
      responseSsids += "}";

    } else {
      responseSsids = "{\"n\":";
      responseSsids += numberOfNetworks;
      responseSsids += "}";
    }
    server.send(200, "text/plain", responseSsids);
  });

  server.on("/setting", []() {
    //Getting credentials
    String querySsid = server.arg("ssid");
    String queryPassword = server.arg("pass");
    String webServerURL =  server.arg("web_url");
    phoneWebServerURL = webServerURL;

    //Saving Credentials and phone webserver URL to EEPROM
    if ((querySsid.length() > 0) && (queryPassword.length() > 0)) {
      Serial.println("clearing eeprom");
      for (int i = 0; i < 96; ++i) {
        EEPROM.write(i, 0);
      }
      for (int i = 0; i < querySsid.length(); ++i) {
        EEPROM.write(i, querySsid[i]);
      }
      for (int i = 0; i < queryPassword.length(); ++i) {
        EEPROM.write(32 + i, queryPassword[i]);
      }
      for (int i = 0; i < webServerURL.length(); ++i) {
        EEPROM.write(64 + i, webServerURL[i]);
      }
      EEPROM.commit();

      //Respond to the request
      server.send(200, "text/plain", "{\"r\":\"cre_rec\"}");   //cre_rec = Credentials Received
      delay(3500);

      //Restart the esp to connect to Wi-Fi with new credentials.
      ESP.restart();
    }
  });
}

void oneSecondPassedFun() {
  oneSecondPassed = true;
}

//Reset the Wi-Fi SSID from EEPROM
void resetSavedCredentials() {
  char sid[] = "Enter WiFi";

  for (int i = 0; i < 10; ++i) {
    EEPROM.write(i, sid[i]);
  }

  EEPROM.commit();
  Serial.println("Credentials Reset Successfully!");
}

void resetCredentialsButtonFunction() {
  if (touchRead(capTouchButton) < TOUCH_THRESHOLD) {
    if (oneSecondPassed) {
      counter++;
      Serial.println(counter);

      if (counter >= RESET_CREDENTIALS_DURATION) {
        resetSavedCredentials();

        Serial.println();
        Serial.println("Wi-Fi Credentials RESET Successfully!");
        Serial.println();

        counter = 0;

        //Blink Red Color LED 2 Times
        for (int i = 0; i < 2; i++) {
          displayColorLED(255, 0, 0);
          delay(350);
          displayColorLED(0, 0, 0);
          delay(350);
        }

        Serial.println();
        Serial.println("Restaring the esp in 2 seconds.");
        Serial.println();

        //Restart the esp
        delay(2000);
        ESP.restart();
      }
      oneSecondPassed = false;
    }
  }
  else {
    counter = 0;
  }
}

void buttonPressFunction() {
  int reading = 1;
  int cap = touchRead(capTouchButton);

  if (cap < TOUCH_THRESHOLD) {
    reading = 0;
  } else {
    reading = 1;
  }

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;

      if (buttonState == 1) { //Was touched and released
        Serial.println("The button is released");
        sendButtonPressEventRequest();
      }
    }
  }
  lastButtonState = reading;
}

void sendButtonPressEventRequest() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;

    String serverPath = "http://" + phoneWebServerURL + "/button_press?id=" + CHECKPOINT_ID;

    Serial.println(serverPath);
    http.begin(client, serverPath);

    // Send HTTP GET request
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);

      String payload = http.getString();
      Serial.println(payload);

      payload.trim();
      buttonPressResult(payload);

    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }
}

void statusCheck() {
  if (oneSecondPassed) {
    statusCounter ++;

    if (statusCounter > STATUS_CHECK_TIME) {
      if (WiFi.status() == WL_CONNECTED) {
        WiFiClient client;
        HTTPClient http;

        String serverPath = "http://" + phoneWebServerURL + "/status_check";
        Serial.println(serverPath);
        http.begin(client, serverPath);

        // Send HTTP GET request
        int httpResponseCode = http.GET();

        if (httpResponseCode > 0) {
          Serial.print("HTTP Response code: ");
          Serial.println(httpResponseCode);

          String payload = http.getString();
          Serial.println(payload);

          //Update the state
          payload.trim();
          updateStatus(payload);

        } else {
          Serial.print("Error code: ");
          Serial.println(httpResponseCode);
        }
        http.end();
      } else {
        Serial.println("WiFi Disconnected");
      }
      statusCounter = 0;
    }
    oneSecondPassed = false;
  }
}

void updateStatus(String payload) {
  if (payload == "IDLE_STATE" ) {
    displayColorLED(0, 0, 255);     //WHITE

  } else if (payload == "SETUP_CHECKPOINT") {
    displayColorLED(230, 230, 250); //PURPLE

  } else if (payload ==  "SET_COURSE") {
    displayColorLED(255, 255, 0);  //YELLOW

  } else if (payload ==  "RUN_STARTED") {

  } else if (payload ==  "RUN_WILL_START") {
    displayColorLED(255, 0, 0);  //RED
  }
}

void displayColorLED(int r, int g, int b) {
  ledcWrite(redChannel, r);
  ledcWrite(greenChannel, g);
  ledcWrite(blueChannel, b);

  //PURPLE COLOR =====>Setup Checkpoints
  //YELLOW COLOR =====>Set Course
  //RED, GREEN COLOR =====>In Run
  //BLUE COLOR =====>Idle state
}

void buttonPressResult(String payload) {
  if (payload == "checkpoint_hit") {
    displayColorLED(0, 255, 0);  //Green

    //Ding Sound
    DacAudio.Play(&Ding);
    for (int i = 0; i < 3500; i++) {
      DacAudio.FillBuffer();
      delay(1);
    }
  }
}

void touchThresholdOperation() {  //Potentiometer
  TOUCH_THRESHOLD = map(analogRead(potentiometerPin), 0, 4095, THRESHOLD_LOWER_LIMIT, THRESHOLD_UPPER_LIMIT);
  //Serial.println(TOUCH_THRESHOLD);
}
