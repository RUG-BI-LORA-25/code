#include "main.h"
#include "pins.h"

int main(void) {
    #ifndef SIMULATION_MODE
    init();
    #endif
    
    TwoWire I2CBus(I2C_SDA, I2C_SCL);

    #ifndef SIMULATION_MODE
        Serial.begin(9600);
        I2CBus.begin();
        delay(100); 
        #ifdef DEBUG
        log("Serial initialized.", "MAIN", Serial);
        log("I2C bus initialized.", "MAIN", Serial);
        #endif

        initDisplay(&I2CBus);
    #endif
    
    

    Sensor *sensors[] = {
        new BME280(BME280_I2C_ADDR, &I2CBus),
        new Photoresistor(PHOTORESISTOR_PIN)
    };

    // list sensors
    #ifdef DEBUG
    log("Available sensors:", "MAIN", Serial);
    for (Sensor* sensor : sensors) {
        log(sensor->getID(), "MAIN", Serial);
    }
    #endif

    // Initialize sensors
    for (Sensor* sensor : sensors) {
        if (!sensor->begin()) {
            #ifdef DEBUG
            // print name of the sensor that failed
            log("Issue with sensor:", "MAIN", Serial);
            log(sensor->getID(), "MAIN", Serial);
            #endif
            exception("Sensor initialization failed!", Serial);
        }
    }

    LORA lora(LORA_NSS, LORA_RESET, LORA_DIO0, LORA_DIO1);
    lora.begin();

    // LED pin
    pinMode(LED_PIN, OUTPUT);

    for (;;) {
        delay(1000);
        // Read sensors
        for (Sensor* sensor : sensors) {
            sensor->show();
            delay(1000);
        }

        // serialEventRun();
    }
}