#include "lora/sx1278.h"

LORA::LORA(int nss, int reset, int dio0, int dio1)
    : pin_nss(nss), pin_reset(reset), pin_dio0(dio0), pin_dio1(dio1) 
{
    SPIClass spi(LORA_MOSI, LORA_MISO, LORA_SCK);
    spi.begin();

    (*radio) = new Module(pin_nss, pin_dio0, pin_reset, pin_dio1, spi);
}

void LORA::begin() {
    log("Initializing LoRa SX1278...", "LORA", Serial);
    if(radio == nullptr) {
        exception("LORA::begin(): radio is nullptr", Serial);
        return;
    }

    if(int state = radio->begin(); state != RADIOLIB_ERR_NONE) {
        exception("Failed to initialize SX1278, error code: " + String(state), Serial);
        return;
    }
}