#include "transport/lora.h"

LORA::LORA(int nss, int reset, int dio0, int dio1)
    : pin_nss(nss), pin_reset(reset), pin_dio0(dio0), pin_dio1(dio1), radio(nullptr)
{
    SPIClass spi(LORA_MOSI, LORA_MISO, LORA_SCK);
    spi.begin();
    log("SPI for LoRa initialized.", "LORA", Serial);
    radio = new Module(pin_nss, pin_dio0, pin_dio1, pin_reset, spi);
}

void LORA::begin() {
    log("Initializing LoRa SX1278...", "LORA", Serial);
    

    int state = radio.begin(); 
    if(state != RADIOLIB_ERR_NONE) {
        exception("Failed to initialize SX1278, error code: " + String(state), Serial);
        return;
    }
    log("LoRa SX1278 initialized successfully.", "LORA", Serial);
}