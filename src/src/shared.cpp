#include "shared.h"
#include "cartof.h"

#ifndef SIMULATION_MODE
Adafruit_SSD1306* display;

void cartof() {
    display->clearDisplay();
    display->drawBitmap(0, 0, cartof_bmp, 128, 64, SSD1306_WHITE);
    display->display();
    
    #ifdef DEBUG
    log("Displaying cartof logo...", "DISPLAY", Serial);
    #endif

    delay(2000);
    display->clearDisplay();
    display->display();
}

void initDisplay(TwoWire* wire) {
    #ifdef DEBUG
    log("Initializing OLED display.", "DISPLAY", Serial);
    #endif
    display = new Adafruit_SSD1306(128, 64, wire);
    if (!display->begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        // we don't really NEED a display, so just continue
        #ifdef DEBUG
        log("SSD1306 allocation failed", "DISPLAY", Serial);
        #endif
    }
    delay(1000);

    cartof();
    // flash();

    #ifdef DEBUG
    log("OLED display initialized", "DISPLAY", Serial);
    #endif
}

void showString(const char* str) {
    display->clearDisplay();
    display->setTextSize(1);
    display->setTextColor(SSD1306_WHITE);
    display->setCursor(0, 0);
    display->println(str);
    display->display();
    delay(100);
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
    // #ifndef SIMULATION_MODE
    // showString(msg);
    // #endif
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

// scan i2c bus
// void scanI2CBus(TwoWire* I2CBus) {
//     log("Scanning I2C bus...", "MAIN", Serial);
//     for (uint8_t address = 1; address < 127; address++ )
//     {
//         I2CBus->beginTransmission(address);
//         if (I2CBus->endTransmission() == 0)
//         {
//             log("I2C device found at address: ", "MAIN", Serial);
//             Serial.print("0x");
//             if (address < 16) Serial.print("0");
//             Serial.println(address, HEX);
//         }
//     }
// }
