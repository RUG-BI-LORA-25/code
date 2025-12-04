#pragma once

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