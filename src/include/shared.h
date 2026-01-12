#pragma once
#include "pins.h"
#include <stdint.h>

#ifdef SIMULATION_MODE
#include "mocks/Arduino.h"
#else
#include <Arduino.h>
#include <Adafruit_SSD1306.h>
// #define I2C_SDA PB9
// #define I2C_SCL PB8

Adafruit_SSD1306 display(128, 64, &Wire, -1);

#endif



void exception();
void log(const char* msg, HardwareSerial& serial);
void exception(const char* msg, HardwareSerial& serial);
void showString(const char* str);
void exception(const String& msg, HardwareSerial& serial);

void blinkInternal(int times, int delayMs);
