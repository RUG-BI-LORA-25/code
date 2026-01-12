#include "shared.h"
#include "cartof.h"

#ifndef SIMULATION_MODE
Adafruit_SSD1306 display(128, 64, &Wire, -1);
void cartof() {
    display.clearDisplay();
    display.drawBitmap(0, 0, cartof_bmp, 128, 160, SSD1306_WHITE);
    display.display();
    #ifdef DEBUG
    log("Displaying cartof logo...", "DISPLAY", Serial);
    #endif
    delay(2000);
    display.clearDisplay();
}

void initDisplay() {
    #ifdef DEBUG
    log("Initializing OLED display...", "DISPLAY", Serial);
    #endif
    display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR); 
    display.clearDisplay();
    display.display();

    cartof();
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
}
#endif

void log(const char* msg, HardwareSerial& serial) {
    serial.println(msg);
}

void log(const char msg[], const char component[], HardwareSerial& serial) {
    serial.print("[");
    serial.print(component);
    serial.print("] ");
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

// No serial -- just printf
void exception(const char* msg) {
    printf("Exception: %s", msg);
    exit(1);
}

void exception(const String& msg, HardwareSerial& serial) {
    serial.println(msg);
    while (1);
}

void blinkInternal(int times, int delayMs) {
    for (int i = 0; i < times; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(delayMs);
        digitalWrite(LED_PIN, LOW);
        delay(delayMs);
    }
}
