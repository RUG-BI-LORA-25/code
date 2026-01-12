#pragma once

#ifdef SIMULATION_MODE
#include "mocks/Arduino.h"
#else
#include <Arduino.h>
#endif

#include "sensors/sensor.h"

class Photoresistor: public Sensor {
    uint16_t analogPin;
public:
    Photoresistor();
    Photoresistor(uint16_t analogPin);
    int data();
    bool begin() override;
    void print(HardwareSerial& serial) override;
    uint8_t serialize(uint8_t* buffer) override;
    void show() override;
};