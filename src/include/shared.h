#pragma once
#include <Arduino.h>
#include <stdint.h>

void exception();
void exception(const char* msg, HardwareSerial& serial);

void blinkInternal(int times, int delayMs);
