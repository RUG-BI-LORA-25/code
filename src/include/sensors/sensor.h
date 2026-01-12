#pragma once

#ifdef SIMULATION_MODE
#include "mocks/Arduino.h"
#else
#include <Arduino.h>
#endif

#include "shared.h"

class Sensor {
    char id[3] = {'U', 'N', '\0'};
public:
    explicit Sensor(const char (&newId)[3])
            : id{newId[0], newId[1], newId[2]} {}

        // optional default ctor with a placeholder ID
    Sensor() : id{'U','N','K'} {}   // “UNK”

    const char* getID() { return id; }
    virtual bool begin() = 0;
    virtual void print(HardwareSerial& serial) = 0;
    virtual uint8_t serialize(uint8_t* buffer) = 0;
};