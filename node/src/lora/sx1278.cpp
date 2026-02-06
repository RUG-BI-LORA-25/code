#include "lora/sx1278.h"
#include "lora/config.h"

#define JOIN 0 // 1 for OTAA, 0 for ABP

LORA::LORA(int nss, int reset, int dio0, int dio1, SPIClass& spi)
    : pin_nss(nss), pin_reset(reset), pin_dio0(dio0), pin_dio1(dio1),
      radio(new Module(nss, dio0, reset, dio1, spi)), 
      node(&radio, &Region, subBand) {

    log("SPI for LoRa initialized.", "LORA", Serial);
}

#if JOIN == 1
int join(LoRaWANNode& node) {
    int state = node.beginOTAA(joinEUI, devEUI, NULL, appKey);
    if (state != RADIOLIB_ERR_NONE) {
        return state;
    }
    state = node.activateOTAA();
    if (state != RADIOLIB_ERR_NONE) {
        return state;
    }

    return state;
}
#else
int join(LoRaWANNode& node) {
    // network ses key: 770a3b13f1910257ab5f5ce134418621
    const uint8_t netKey[] = { 0x77, 0x0a, 0x3b, 0x13, 0xf1, 0x91, 0x02, 0x57, 0xab, 0x5f, 0x5c, 0xe1, 0x34, 0x41, 0x86, 0x21 };
    // app key: b768e931022d35d8681b3cc85e2e9002
    const uint8_t a[] = { 0xb7, 0x68, 0xe9, 0x31, 0x02, 0x2d, 0x35, 0xd8, 0x68, 0x1b, 0x3c, 0xc8, 0x5e, 0x2e, 0x90, 0x02 };

    int state = node.beginABP(0x01fa06f3, NULL, NULL, netKey, a);
    if (state != RADIOLIB_ERR_NONE) {
        return state;
    }
    state = node.activateABP();
    if (state == RADIOLIB_LORAWAN_NEW_SESSION || 
        state == RADIOLIB_LORAWAN_SESSION_RESTORED) {
        return RADIOLIB_ERR_NONE;
    }
    log("LoRaWAN activation failed", "LORA", Serial);
    return state;
}
#endif

void LORA::reBegin(State target) {
    log("Reinitializing LoRa with new parameters...", "LORA", Serial);
    
    int state = radio.setSpreadingFactor(target.spreadingFactor);
    state = radio.setBandwidth(target.bandwidth);
    
}

void LORA::begin() {
    log("Initializing LoRa SX1278...", "LORA", Serial);
    
    // Initialize radio at 433.175 MHz, 125kHz BW, SF12
    int state = radio.begin(433.175, 125.0, 12, 5, RADIOLIB_SX127X_SYNC_WORD, 17, 8, 0);
    
    if(state != RADIOLIB_ERR_NONE) {
        debug(state != RADIOLIB_ERR_NONE, F("Failed to initialize SX1278"), state, true);
        return;
    }
    
    log("LoRa SX1278 initialized successfully.", "LORA", Serial);
    
    // LoRaWAN takes over from here
    state = join(node);
    
    if (state != RADIOLIB_ERR_NONE) {
        debug(state != RADIOLIB_ERR_NONE, F("Failed to join LoRaWAN network"), state, true);
        return;
    }

    // Disable ADR - we control datarate manually
    node.setADR(false);
    
    // Set datarate - DR0=SF12, DR3=SF9, etc.
    state = node.setDatarate(0);  // DR0 = SF12
    if (state != RADIOLIB_ERR_NONE) {
        Serial.print("[LORA] Failed to set datarate: ");
        Serial.println(state);
    } else {
        Serial.println("[LORA] Datarate set to DR0 (SF12/125kHz)");
    }
    
    log("LoRaWAN activation successful.", "LORA", Serial);
}

int16_t LORA::setFrequency(float freq) {
    return radio.setFrequency(freq);
}

int16_t LORA::setBandwidth(float bw) {
    return radio.setBandwidth(bw);
}

int16_t LORA::setSpreadingFactor(uint8_t sf) {
    return radio.setSpreadingFactor(sf);
}

int16_t LORA::setPower(int8_t power) {
    return radio.setOutputPower(power);
}

int16_t LORA::setGain(uint8_t gain) {
    return radio.setGain(gain);
}

int16_t LORA::sendData(const uint8_t* data, size_t len, uint8_t* dlBuf, size_t* dlLen) {
    Serial.print("[LORA] Sending ");
    Serial.print(len);
    Serial.println(" bytes...");
    
    if (dlBuf && dlLen) {
        return node.sendReceive(data, len, 1, dlBuf, dlLen);
    }
    
    
    return node.sendReceive(data, len, 1);
}
