#pragma once

#ifdef SIMULATION_MODE
#include "mocks/Arduino.h"
#else
#include <Arduino.h>
#endif

class Sensor {
public:
    virtual bool begin() = 0;
    virtual void print(HardwareSerial& serial) = 0;
    virtual uint8_t serialize(uint8_t* buffer) = 0;
};