#include "main.h"
#include "pins.h"
#define LEN(x) (sizeof(x) / sizeof((x)[0]))
int main(void) {
    #ifndef SIMULATION_MODE
    init();
    #endif
    
    TwoWire I2CBus(I2C_SDA, I2C_SCL);

    #ifndef SIMULATION_MODE
        Serial.begin(115200);
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

    SPIClass spi(LORA_MOSI, LORA_MISO, LORA_SCK);
    spi.begin();

    delay(100);
    LORA lora(LORA_NSS, LORA_RESET, LORA_DIO0, LORA_DIO1, spi);
    lora.begin();
    

    // LED pin
    pinMode(LED_PIN, OUTPUT);

    for (;;) {
        delay(5000);
        // Read sensors
        uint8_t buffer[10];
        uint8_t offset = 0;
        for (Sensor* sensor : sensors) {
            uint8_t len = sensor->serialize(buffer + offset);
            offset += len;
        }
        int state = lora.sendData(buffer, offset);
        
        #ifdef DEBUG
        if (state >= 0) {
            log("Data sent via LoRaWAN successfully.", "MAIN", Serial);
        } else {
            Serial.print("[MAIN] Failed to send data, error: ");
            Serial.println(state);
        }
        #endif

        // serialEventRun();
    }
}