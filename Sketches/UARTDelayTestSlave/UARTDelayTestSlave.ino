#include <HardwareSerial.h>

#define SERIAL_BAUDRATE 1000000

#define TEST_PIN 12

HardwareSerial SerialPort(2); //use UART2

void setup() {
  Serial.begin(SERIAL_BAUDRATE);
  SerialPort.begin(SERIAL_BAUDRATE, SERIAL_8N1, 16, 17);
  pinMode(TEST_PIN, OUTPUT);

  Serial.printf("--- initialized ---\n");
}

void loop() {
  if(SerialPort.available()){
    digitalWrite(TEST_PIN,SerialPort.read());
    delay(20);
  }
  digitalWrite(TEST_PIN,0);
}