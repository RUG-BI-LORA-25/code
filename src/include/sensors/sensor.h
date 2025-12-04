#pragma once

class Sensor {
public:
    virtual bool begin() = 0;
    virtual void print(HardwareSerial& serial) = 0;
};