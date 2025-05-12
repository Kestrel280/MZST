#include "Timing.h"

// Efficiently counts the number of set bits (1's) in an int
inline int countSetBits(int inp) {
  int count = 0;
  while (inp) {
    inp = inp & (inp - 1);
    count++;
  }
  return count;
}

// Master RF listen loop. Intended to be run as the SOLE THREAD on a core: MUST NOT BE INTERRUPTED
void rfListen(void* param) {
  // TODO disable interrupts
  Serial.printf("rfListen running on core %d\n", xPortGetCoreID());

  const int LOOP_TIME_US = 1000; // How long it takes to run one 
  unsigned int buf = 0b0; // Bit buffer received on the RF pin. Left boundary is old, right boundary is new
  int matchedBits;

  unsigned int debugMask;

  dbg_time = currentTimeAbs();

  while(true) {

    digitalWrite(SPEAKER_PIN, HIGH);

    // Flush the oldest value and create a slot for the new value
    buf = buf << 1;

    // Read the pin and store it as the final bit
    //  (value of digitalRead is either 0 or 1, so a plain OR will stick it at the right boundary)
    buf = buf | digitalRead(RF_RECEIVE_PIN);
    matchedBits = countSetBits(~(buf ^ rfKey));

    // Debug: print the buf
    //debugMask = 0b10000000000000000000000000000000;
    //for (int i = 0; i < 32; i++) {
    //  Serial.printf("%d", (buf & debugMask) > 0);
    //  debugMask = debugMask >> 1;
    //}
    //Serial.printf(" (%d / %d)", matchedBits, rfKeyRequiredMatches);
    //Serial.printf("\n");
    
    // Check if buffer matches the key to acceptable tolerance
    if (matchedBits >= rfKeyRequiredMatches ) {
      outboundMessageQueue.push(createOutboundMessage(MTYPE_TIMESTAMPRESET, matchedBits));
      timestampLastResetUs = currentTimeAbs();
      Serial.printf("Received timestamp-reset key (%d / %d matched bits, %d required) \n", matchedBits, 32, rfKeyRequiredMatches);
    };

    dbg_time = currentTimeAbs(); // this call just happens to put us at pretty much exactly 1ms for this loop, so. leaving it in :-)
    esp_rom_delay_us(rfPulseIntervalUs + TRANSMIT_LOOP_TIME_US); // (busy wait) Delay for the pulse duration + account for transmitter lag
  }
}
