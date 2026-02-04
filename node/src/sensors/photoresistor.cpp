#include "sensors/photoresistor.h"

#ifdef SIMULATION_MODE
#define PIN_PHOTORESISTOR 0
#else
#define PIN_PHOTORESISTOR PC3
#endif

Photoresistor::Photoresistor() : Sensor({"PR"}), analogPin(PIN_PHOTORESISTOR) {}

Photoresistor::Photoresistor(uint16_t pin) : Sensor({"PR"}), analogPin(pin) {}

bool Photoresistor::begin() {
#ifdef SIMULATION_MODE
    return true;
#else
    #ifdef DEBUG
    log("Set up sensor.", "PR", Serial);
    #endif
    pinMode(analogPin, INPUT);
    return true;
#endif
}

int Photoresistor::data() {
#ifdef SIMULATION_MODE
    return 1337; // Mock: mid-range light level
#else
    return analogRead(analogPin);
#endif
}

void Photoresistor::show() {
    int lightLevel = data();
    #ifndef SIMULATION_MODE
    showString(("Photoresistor\nLight Level: " + String(lightLevel)).c_str());
    #endif
}

void Photoresistor::print(HardwareSerial& serial) {
    int lightLevel = data();
    serial.print("Light Level = ");
    serial.println(lightLevel);
    serial.println();
}

uint8_t Photoresistor::serialize(uint8_t* buffer) {
    uint16_t light = data();
    
    buffer[0] = light & 0xFF;
    buffer[1] = (light >> 8) & 0xFF;
    
    return 2;
}