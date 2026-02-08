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

    // TDMA: initial slot offset so multiple nodes don't transmit simultaneously
    if (TDMA_SLOT_OFFSET_MS > 0) {
        char slotMsg[48];
        snprintf(slotMsg, sizeof(slotMsg), "TDMA slot %d\noffset %lums", TDMA_SLOT, TDMA_SLOT_OFFSET_MS);
        showString(slotMsg);
        log("TDMA slot offset", "MAIN", Serial);
        delay(TDMA_SLOT_OFFSET_MS);
    }

    for (;;) {
        unsigned long loopStart = millis();

        // Read sensors
        uint8_t buffer[10];
        uint8_t offset = 0;
        for (Sensor* sensor : sensors) {
            uint8_t len = sensor->serialize(buffer + offset);
            offset += len;
        }
        uint8_t dlBuf[251];  // max LoRaWAN downlink payload
        size_t dlLen = 0;

        showString("Sending uplink...");
        int16_t state = lora.sendData(buffer, offset, dlBuf, &dlLen);
        
        if (state >= 0) {
            log("Uplink sent OK", "MAIN", Serial);
            showString("Uplink sent OK");
        } else {
            char errMsg[32];
            snprintf(errMsg, sizeof(errMsg), "UL error: %d", state);
            log(errMsg, "MAIN", Serial);
            showString(errMsg);
        }

        if (state > 0) {
            char dlMsg[64];
            snprintf(dlMsg, sizeof(dlMsg), "DL RX%d %u bytes", state, (unsigned)dlLen);
            log(dlMsg, "MAIN", Serial);

            #ifdef DEBUG
            Serial.print("[MAIN]   HEX: ");
            for (size_t i = 0; i < dlLen; i++) {
                if (dlBuf[i] < 0x10) Serial.print('0');
                Serial.print(dlBuf[i], HEX);
            }
            Serial.println();
            #endif
        }

        if (state > 0 && dlLen == sizeof(State)) {
            State params;
            memcpy(&params, dlBuf, sizeof(params));

            displayState.dr = params.datarate;
            displayState.txPower = params.power;

            char paramMsg[48];
            snprintf(paramMsg, sizeof(paramMsg), "DL: DR%d TX%ddBm", params.datarate, params.power);
            log(paramMsg, "MAIN", Serial);

            lora.reBegin(params);
        } else if (state > 0) {
            showString("DL (MAC only)");
        } else {
            showString("No downlink");
        }

        // Sleep only the remaining time so total cycle = UPLINK_INTERVAL_MS,
        // regardless of how long TX + RX windows took.
        unsigned long elapsed = millis() - loopStart;
        if (elapsed < UPLINK_INTERVAL_MS) {
            delay(UPLINK_INTERVAL_MS - elapsed);
        }
    }
}