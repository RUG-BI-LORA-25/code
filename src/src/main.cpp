#include "main.h"
#include "pins.h"

int main(void) {
    #ifndef SIMULATION_MODE
    init();
    #endif

    #ifndef SIMULATION_MODE
        Serial.begin(9600);
        #ifdef DEBUG
        Serial.println("Starting up...");
        #endif
    #endif
    
    TwoWire I2CBus(I2C_SDA, I2C_SCL);

    Sensor *sensors[] = {
        new BME280(BME280_I2C_ADDR, &I2CBus),
        new Photoresistor(PHOTORESISTOR_PIN)
    };

    // Initialize sensors
    for (Sensor* sensor : sensors) {
        if (!sensor->begin()) {
            exception("Sensor initialization failed!", Serial);
        }
    }

    LORA lora(LORA_NSS, LORA_RESET, LORA_DIO0, LORA_DIO1);
    lora.begin();

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