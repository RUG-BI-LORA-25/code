#ifndef SHARED_H
#define SHARED_H
#include <Arduino.h>
void exception();
void exception(const char* msg, HardwareSerial& serial);

void blinkInternal(int times, int delayMs);

#endif // SHARED_H