#ifndef TIMERSYNCMODULE_H
#define TIMERSYNCMODULE_H

const unsigned int rfKey = 0b10101011001100101111110001011010;

const int rfPulseIntervalUs = 250; // Microseconds

const int rfKeyRequiredMatches = 25; // 

// How long it takes to run the receive loop (calculated experimentally, for now)
// The loop overhead takes this long to read and process 1 bit
// So, whatever our pulse interval is, the TRANSMITTER needs to wait an additional RECEIVE_LOOP_TIME_US
//  in between sending bits, to account for receiver lag 
const int RECEIVE_LOOP_TIME_US = 1000;

// Similar: how long it takes to run the transmit loop (calculated experimentally, for now)
// Loop overhead takes this long to transmit 1 bit
// The RECEIVERS need to wait an additional TRANSMIT_LOOP_TIME_US in between reading bits,
//  to account for transmitter lag
const int TRANSMIT_LOOP_TIME_US = 24;

#endif