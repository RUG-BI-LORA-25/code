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
    const uint8_t netKey[] = { RADIOLIB_LORAWAN_NET_KEY };
    const uint8_t appSKey[] = { RADIOLIB_LORAWAN_APP_S_KEY };

    int state = node.beginABP(RADIOLIB_LORAWAN_DEV_ADDR, NULL, NULL, netKey, appSKey);
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
    Serial.print("[LORA] Algo requested DR change: ");
    Serial.print(targetDr);
    Serial.print(" -> ");
    Serial.print(target.datarate);
    Serial.print(", TX power: ");
    Serial.println(target.power);
    targetDr = target.datarate;
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

    // Apply single-channel overrides (frequency, RX delays, etc.)
    applyOverrides();
    Serial.println("[LORA] RX overrides applied");

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

void LORA::applyOverrides() {
    // Force all dynamic channels to 433.175 MHz so the node never hops
    // away from our single-channel HackRF, even if ChirpStack sends
    // NewChannelReq / DlChannelReq MAC commands.
    for (int i = 0; i < 3; i++) {
        node.dynamicChannels[RADIOLIB_LORAWAN_UPLINK][i].idx   = i;
        node.dynamicChannels[RADIOLIB_LORAWAN_UPLINK][i].freq  = 4331750;
        node.dynamicChannels[RADIOLIB_LORAWAN_UPLINK][i].drMin = 0;
        node.dynamicChannels[RADIOLIB_LORAWAN_UPLINK][i].drMax = 5;
        node.dynamicChannels[RADIOLIB_LORAWAN_UPLINK][i].dr    = this->targetDr;

        node.dynamicChannels[RADIOLIB_LORAWAN_DOWNLINK][i] =
            node.dynamicChannels[RADIOLIB_LORAWAN_UPLINK][i];
    }

    node.channels[RADIOLIB_LORAWAN_RX2].freq = 4331750;
    node.channels[RADIOLIB_LORAWAN_RX2].dr   = 0;

    node.channels[RADIOLIB_LORAWAN_UPLINK].dr = this->targetDr;

    node.rx1DrOffset = 5;

    node.rxDelays[1] = 5500;   // RX1 opens 5.0s after TX end
    node.rxDelays[2] = 7500;   // RX2 opens 7.5s after TX end
    node.scanGuard   = 250;
}

int16_t LORA::sendData(const uint8_t* data, size_t len, uint8_t* dlBuf, size_t* dlLen) {
    Serial.print("[LORA] Sending ");
    Serial.print(len);
    Serial.println(" bytes...");

    // Re-apply overrides BEFORE every send, because ChirpStack MAC commands
    // (RXTimingSetupReq, RXParamSetupReq) in previous downlinks overwrite them.
    applyOverrides();

    int16_t result;
    if (dlBuf && dlLen) {
        result = node.sendReceive(data, len, 1, dlBuf, dlLen);
    } else {
        result = node.sendReceive(data, len, 1);
    }

    // Re-apply overrides AFTER send too, in case this downlink had MAC commands
    applyOverrides();

    return result;
}
