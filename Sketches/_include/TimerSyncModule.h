#ifndef TIMERSYNCMODULE_H
#define TIMERSYNCMODULE_H

int rfKey[] = {
	1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1
};

const int rfKeyLength = sizeof(rfKey) / sizeof(rfKey[0]);

const int rfPulseIntervalUs = 100000; // Microseconds

const int rfKeyAllowableMisses = 4; // 

#endif
