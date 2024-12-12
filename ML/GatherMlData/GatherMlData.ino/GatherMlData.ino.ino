#include <deque>
#include "esp32-hal-touch.h"

#define TOUCH_BUFSIZE 25
#define SERIAL_BAUDRATE 921600

std::deque<uint32_t> buf;
uint32_t baseline = 0;
uint32_t avg = 0;
bool touched = false;

// Hardware constants
const touch_pad_t tpPin = TOUCH_PAD_NUM7;

const int speakerPin = D0;
const int ledRedPin = D2;
const int ledGreenPin = D3;
const int ledBluePin = D1;

void setup() {

  Serial.begin(SERIAL_BAUDRATE);
  pinMode(speakerPin, OUTPUT);
  ledcAttach(ledRedPin, 5000, 8);
  ledcAttach(ledGreenPin, 5000, 8);
  ledcAttach(ledBluePin, 5000, 8);
  digitalWrite(speakerPin, HIGH);

  writeLed(255, 0, 0);

  // Sleep for 3 seconds to give me a second to back up from the module
  sleep(3);

  writeLed(0, 0, 255);

  // Collect baseline data for the buffer
  for (int i = 0; i < TOUCH_BUFSIZE; i++) {
    uint32_t touchVal = touchRead(tpPin);
    buf.push_front(touchVal);

    avg += touchVal;
  }
  avg = avg / TOUCH_BUFSIZE;
  baseline = avg;

  Serial.printf("t, raw, avg, variance, peaktopeak, lastminusstart, label\n");
  writeLed(255, 255, 255);
}

void loop() {
  uint32_t touchVal, min, max, avg, var;

  for (int i = 0; i < TOUCH_BUFSIZE; i++) {
    digitalWrite(speakerPin, HIGH);
    touchVal = touchRead(tpPin);
    buf.pop_front();
    buf.push_back(touchVal);
    delay(1);
  }

  avg = dequeAvg(buf);
  var = dequeVariance(buf, avg);
  min = dequeMin(buf);
  max = dequeMax(buf);
  Serial.printf("%10d, %10d, %10d, %10d, %10d, %10d, ", millis(), touchVal, avg, var, max-min, ((int32_t)buf.back()) - ((int32_t)buf.front()));

  if ((!touched) && (touchVal > (baseline + (baseline / 8)))) {
    touched = true;
    Serial.printf("TOUCHED");
  } else if (touched && (touchVal < (baseline + (baseline / 8)))) {
    touched = false;
    Serial.printf("UNTOUCHED");
  } else if (touched) {
    Serial.printf("TOUCHED");
  } else {
    Serial.printf("NO_EVENT");
  }
  Serial.printf("\n");
  
}

void writeLed(int r, int g, int b) {
  ledcWrite(ledRedPin, r);
  ledcWrite(ledGreenPin, g);
  ledcWrite(ledBluePin, b);
}

uint32_t dequeMax(std::deque<uint32_t> d) {
  if (d.empty()) { return -1; }

  uint32_t max = 0;

  for (uint32_t val : d) {
    max = val > max ? val : max;
  }
  return max;
}

uint32_t dequeMin(std::deque<uint32_t> d) {
  if (d.empty()) { return 0; }

  uint32_t min = 0b11111111111111111111111111111111;

  for (uint32_t val : d) {
    min = val < min ? val : min;
  }
  return min;
}

uint32_t dequeAvg(std::deque<uint32_t> d) {
  if (d.empty()) { return 0; }

  uint32_t sum = 0;

  for (uint32_t val : d) {
    sum += val;
  }
  return sum / d.size();
}

uint32_t dequeVariance(std::deque<uint32_t> d, uint32_t avg) {
  if (d.empty()) { return 0; }

  uint32_t acc = 0;
  int32_t v;
  
  //Serial.printf("avg = %d    |    ", avg);

  for (uint32_t val : d) {
    v = ((int32_t)val) - ((int32_t)avg);
    //Serial.printf("%d -> %d, ", val, v);
    acc += (v * v);
  }
  //Serial.printf("\n");

  return acc / d.size();
}