#pragma once
#ifdef SIMULATION_MODE
#include "mocks/Arduino.h"
#else
#include <Arduino.h>
#endif

#include "pins.h"
#include <stdint.h>

void exception();
void log(const char* msg, HardwareSerial& serial);
void exception(const char* msg, HardwareSerial& serial);
void exception(const String& msg, HardwareSerial& serial);

void blinkInternal(int times, int delayMs);
