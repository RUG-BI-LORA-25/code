#include <Arduino.h>
#include "sensors/photoresistor.h"

#define PIN_PHOTORESISTOR PC3

Photoresistor::Photoresistor() {
    // default constructor
}

Photoresistor::Photoresistor(uint16_t pin) 
: analogPin(pin) {}

bool Photoresistor::begin() {
    pinMode(analogPin, INPUT);
    return true;
}

int Photoresistor::data() {
    return analogRead(analogPin);
}

void Photoresistor::print(HardwareSerial& serial) {
    int lightLevel = data();
    serial.print("Light Level = ");
    serial.println(lightLevel);
    serial.println();
}