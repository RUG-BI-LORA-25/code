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

#endif

void exception();
void log(const char* msg, HardwareSerial& serial);
void log(const char msg[], const char component[], HardwareSerial& serial);
void exception(const char* msg, HardwareSerial& serial);
void showString(const char* str);
void exception(const String& msg, HardwareSerial& serial);
void initDisplay(TwoWire* wire);
void blinkInternal(int times, int delayMs);

struct __attribute__((packed)) State {
    uint8_t spreadingFactor;
    float bandwidth;
    int rssi;
};
// void scanI2CBus(TwoWire* I2CBus);
