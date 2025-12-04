#include "main.h"
#include "pins.h"

int main(void) {
    init();

    Serial.begin(9600);
    #ifdef DEBUG
    Serial.println("Starting up...");
    #endif
    
    TwoWire I2CBus(I2C_SDA, I2C_SCL);

    Sensor *sensors[] = {
        new BME280(BME280_SLAVE_ADDRESS, &I2CBus),
        new Photoresistor(PHOTORESISTOR_PIN)
    };

    // Initialize sensors
    for (Sensor* sensor : sensors) {
        if (!sensor->begin()) {
            exception("Sensor initialization failed!", Serial);
        }
    }

    BME280 bme280(BME280_SLAVE_ADDRESS, &I2CBus);
    Photoresistor photoresistor(PHOTORESISTOR_PIN);

    // LED pin
    pinMode(LED_PIN, OUTPUT);

    for (;;) {
        // Read sensors
        for (Sensor* sensor : sensors) {
            sensor->print(Serial);
        }
        blinkInternal(1, 200);

        delay(1000);
        // serialEventRun();
    }
}