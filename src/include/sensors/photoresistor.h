#pragma once

#ifdef SIMULATION_MODE
#include "mocks/Arduino.h"
#else
#include <Arduino.h>
#endif

#include "shared.h"
#include "sensors/sensor.h"

class Photoresistor: public Sensor {
    uint16_t analogPin;
public:
    Photoresistor();
    Photoresistor(uint16_t analogPin);
    bool begin();
    int data();
    
    void print(HardwareSerial& serial);
};