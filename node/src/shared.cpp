#include "shared.h"
#include "cartof.h"

DisplayState displayState = {0, 12}; // default DR0 (SF12), 12 dBm

#ifndef SIMULATION_MODE
Adafruit_SSD1306* display;

void cartof() {
    display->clearDisplay();
    display->drawBitmap(0, 0, cartof_bmp, 128, 64, SSD1306_WHITE);
    display->display();

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

#include "identity.h"

static const int DR_TO_SF[] = {12, 11, 10, 9, 8, 7};

static void drawBanner() {
    // bottom 10px: horizontal line + status text
    display->drawFastHLine(0, 53, 128, SSD1306_WHITE);
    display->setTextSize(1);
    display->setTextColor(SSD1306_WHITE);
    display->setCursor(0, 55);
    int sf = (displayState.dr >= 0 && displayState.dr <= 5) ? DR_TO_SF[displayState.dr] : 12;
    uint16_t devPrefix = (uint16_t)((RADIOLIB_LORAWAN_DEV_EUI >> 48) & 0xFFFF);
    char banner[32];
    snprintf(banner, sizeof(banner), "%04x|SF%d|TX%d|S%d/%d", devPrefix, sf, displayState.txPower, TDMA_SLOT, TDMA_NUM_SLOTS);
    display->print(banner);
}

void showString(const char* str) {
    if (!display) return;
    display->clearDisplay();
    display->setTextColor(SSD1306_WHITE);

    // message area: top 52 pixels (above the banner line)
    int len = strlen(str);

    if (len <= 10) {
        // short message: big text, centered in message area
        display->setTextSize(2);
        int16_t x1, y1;
        uint16_t w, h;
        display->getTextBounds(str, 0, 0, &x1, &y1, &w, &h);
        display->setCursor((128 - w) / 2, (52 - h) / 2);
    } else if (len <= 21) {
        // medium: size 2, slight offset
        display->setTextSize(2);
        display->setCursor(0, 4);
    } else {
        // long: size 1 (21 chars/line)
        display->setTextSize(1);
        display->setCursor(0, 0);
    }

    display->println(str);
    drawBanner();
    display->display();
}
#endif

void log(const char* msg, HardwareSerial& serial) {
    serial.println(msg);
    showString(msg);
}

void log(const char msg[], const char component[], HardwareSerial& serial) {
    serial.print("[");
    serial.print(component);
    serial.print("] ");
    serial.println(msg);
    showString(msg);
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
