#include "sensors/photoresistor.h"

#ifdef SIMULATION_MODE
#define PIN_PHOTORESISTOR 0
#else
#define PIN_PHOTORESISTOR PC3
#endif

Photoresistor::Photoresistor() : analogPin(PIN_PHOTORESISTOR) {}

Photoresistor::Photoresistor(uint16_t pin) : analogPin(pin) {}

bool Photoresistor::begin() {
#ifdef SIMULATION_MODE
    return true;
#else
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

void Photoresistor::print(HardwareSerial& serial) {
    int lightLevel = data();
    serial.print("Light Level = ");
    serial.println(lightLevel);
    serial.println();
}