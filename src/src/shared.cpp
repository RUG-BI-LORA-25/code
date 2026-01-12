#include "shared.h"
#include "cartof.h"

#ifndef SIMULATION_MODE
display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR); 
display.clearDisplay();
display.display();
#endif

void log(const char* msg, HardwareSerial& serial) {
    serial.println(msg);
}

void exception() {
    while (1); // Halt execution
}

void exception(const char* msg, HardwareSerial& serial) {
    serial.println(msg);
    #ifndef SIMULATION_MODE
    showString(msg);
    #endif
    while (1); // Halt execution
}

void cartof() {
    display.clearDisplay();
    display.drawBitmap(0, 0, cartof, 128, 160, SSD1306_WHITE);
    display.display();
    delay(2000);
    display.clearDisplay();
}

void showString(const char* str) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(str);
    display.display();
    delay(2000);
    display.clearDisplay();
void exception(const String& msg, HardwareSerial& serial) {
    serial.println(msg);
    while (1);
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
