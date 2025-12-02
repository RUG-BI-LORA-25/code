#include "shared.h"

void exception() {
    while (1); // Halt execution
}

void exception(const char* msg, HardwareSerial& serial) {
    serial.println(msg);
    while (1); // Halt execution
}

void blinkInternal(int times, int delayMs) {
    pinMode(PA5, OUTPUT);
    for (int i = 0; i < times; i++) {
        digitalWrite(PA5, HIGH);
        delay(delayMs);
        digitalWrite(PA5, LOW);
        delay(delayMs);
    }
}