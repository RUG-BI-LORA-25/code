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
    Serial.print("[LORA] Algo requested DR change: ");
    Serial.print(targetDr);
    Serial.print(" -> ");
    Serial.println(target.datarate);
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

    // Force single-channel RX2 on 433.175 MHz, always DR0 (SF12) for RX2
    node.channels[RADIOLIB_LORAWAN_RX2].freq = 4331750;
    node.channels[RADIOLIB_LORAWAN_RX2].dr   = 0;

    // Force the active uplink DR to the algo-requested value.
    // MAC commands (LinkADRReq) update channels[UPLINK].dr directly, bypassing
    // the dynamicChannels pool.  Without this line ChirpStack can silently
    // change the node's DR even though ADR is disabled.
    node.channels[RADIOLIB_LORAWAN_UPLINK].dr = this->targetDr;

    // Force RX1 to always use DR0 (SF12) regardless of uplink DR.
    // rx1DrTable[uplinkDR][5] = DR0 for all uplink DRs in EU433.
    // This keeps the downlink at the same SF the TMST_OFFSET was calibrated for.
    node.rx1DrOffset = 5;

    // Force RX delays to match ChirpStack rx1_delay=5
    // Stock RadioLib computes the RX-window timeout as:
    //   timeoutUs = toaMin + (scanGuard + 1500) * 1000
    // The gateway applies a TMST_OFFSET to compensate for GnuRadio/HackRF
    // pipeline latency, so the signal arrives close to on-time.
    // With scanGuard=250 the timeout ranges from ~1.77s (SF7) to ~2.41s (SF12),
    // all fitting within the 2.5s gap between RX1 (5.0s) and RX2 (7.5s).
    node.rxDelays[1] = 5000;   // RX1 opens 5.0s after TX end
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
