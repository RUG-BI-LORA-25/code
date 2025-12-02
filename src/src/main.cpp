#include "main.h"

TwoWire I2CBus(PB9, PB8);  // I2C2: SDA, SCL
BME280 bme280(0x76, &I2CBus);
unsigned long delayTime;

void setup() {
    pinMode(PA5, OUTPUT); // Initialize LED pin FIRST
    
    // Blink to show we entered setup
    for(int i = 0; i < 5; i++) {
        digitalWrite(PA5, HIGH);
        delay(200);
        digitalWrite(PA5, LOW);
        delay(200);
    }
    
    Serial.begin(9600);
    delay(1000);
    Serial.println("Starting up...");

    // initialize bme280 sensor
    if (!bme280.begin()) {
        exception("Could not find a valid BME280 sensor, check wiring!", Serial);
    }
    
    Serial.println("BME280 initialized!");
}

void loop() {
    digitalWrite(PA5, HIGH);
    bme280.print(Serial);
    digitalWrite(PA5, LOW);
    delay(2000);
}