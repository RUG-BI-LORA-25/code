#include "lora/loraWAN.h"
#include <openssl/evp.h>
#include <cstring>
// https://lora-alliance.org/wp-content/uploads/2021/11/LoRaWAN-Link-Layer-Specification-v1.0.4.pdf


// No idea what this does
uint32_t calculateMIC(const uint8_t *data, uint8_t len, const uint8_t *key) {
    uint8_t mic[16];
    size_t micLen = 16;
    // MAC stuff i guess
    EVP_MAC *mac = EVP_MAC_fetch(nullptr, "CMAC", nullptr);
    EVP_MAC_CTX *ctx = EVP_MAC_CTX_new(mac);
    
    OSSL_PARAM params[2];
    // Actual AES-128-CBC cipher
    params[0] = OSSL_PARAM_construct_utf8_string("cipher", (char*)"AES-128-CBC", 0);
    params[1] = OSSL_PARAM_construct_end();
    
    EVP_MAC_init(ctx, key, 16, params);
    EVP_MAC_update(ctx, data, len);
    EVP_MAC_final(ctx, mic, &micLen, 16);
    
    EVP_MAC_CTX_free(ctx);
    EVP_MAC_free(mac);
    
    // first 4 bytes of mic
    // CMAC = aes128_cmac(NwkSKey, B0 | msg)
    // MIC = CMAC[0..3] 
    return *((uint32_t*)mic); // might be shit for embedded
    // but my brain too smol
}

void cryptPayload(MACPayload_t& macPayload, const LoRaWANConfig_t& config, bool uplink) {
    // Extract fields from MACPayload
    uint8_t *data = macPayload.frmPayload;
    uint8_t len = macPayload.frmPayloadLen;
    uint32_t devAddr = macPayload.devAddr;
    uint32_t fcnt = macPayload.fcnt;
    
    // Build Ai block (page 26 of spec)
    // e=mc^2 + ai after all
    uint8_t ai[16] = {0};
    ai[0] = 0x01;
    ai[5] = uplink ? 0x00 : 0x01;
    
    // DevAddr
    // TIL 0x(0-F)^ truncates to smaller in bitwise
    // i.e. DEADBEEF & FF = EF
    ai[6] = (devAddr >> 0) & 0xFF;
    ai[7] = (devAddr >> 8) & 0xFF;
    ai[8] = (devAddr >> 16) & 0xFF;
    ai[9] = (devAddr >> 24) & 0xFF;
    
    // FCnt
    ai[10] = (fcnt >> 0) & 0xFF;
    ai[11] = (fcnt >> 8) & 0xFF;
    ai[12] = (fcnt >> 16) & 0xFF;
    ai[13] = (fcnt >> 24) & 0xFF;
    
    uint8_t *i = &ai[15]; 
    
    uint8_t numBlocks = (len + 15) / 16; 
    
    for (uint8_t j = 0; j < numBlocks; j++) {
        *i = j + 1;
        
        // AES-128 ECB encrypt
        uint8_t s[16];
        EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
        EVP_EncryptInit_ex(ctx, EVP_aes_128_ecb(), nullptr, config.appSKey, nullptr);
        EVP_CIPHER_CTX_set_padding(ctx, 0);
        
        int outlen;
        EVP_EncryptUpdate(ctx, s, &outlen, ai, 16);
        EVP_EncryptFinal_ex(ctx, s + outlen, &outlen);
        EVP_CIPHER_CTX_free(ctx);
        
        // XOR encrypted block with payload
        for (uint8_t b = 0; b < 16 && (j * 16 + b) < len; b++) {
            data[j * 16 + b] ^= s[b];
        }
    }
}

// Join procedure is always initiated by the end-device sending a Join-Request frame.
//       8      8        2
// JoinEUI DevEUI DevNonce
PHYPayload_t joinRequest(const LoRaWANConfig_t& config, uint16_t devNonce) {
    PHYPayload_t phy = {0};
    
    phy.mhdr.ftype = 0b000;  // Join Request
    phy.mhdr.rfu = 0;
    phy.mhdr.major = 0;
    
    memcpy(phy.joinPayload.appEUI, config.appEUI, 8);
    memcpy(phy.joinPayload.devEUI, config.devEUI, 8);
    phy.joinPayload.devNonce = devNonce;
    
    // Build MIC buffer: MHDR | AppEUI | DevEUI | DevNonce
    uint8_t micBuf[19];
    micBuf[0] = (phy.mhdr.ftype << 5) | (phy.mhdr.rfu << 2) | phy.mhdr.major;
    memcpy(micBuf + 1, phy.joinPayload.appEUI, 8);
    memcpy(micBuf + 9, phy.joinPayload.devEUI, 8);
    micBuf[17] = devNonce & 0xFF;
    micBuf[18] = (devNonce >> 8) & 0xFF;
    
    phy.mic = calculateMIC(micBuf, 19, config.appKey);
    return phy;
}

PHYPayload_t UCDup(const LoRaWANConfig_t& config, MACPayload_t& macPayload) {
    cryptPayload(macPayload, config, true);
    
    PHYPayload_t phy = {0};
    phy.mhdr.ftype = 0b010;  // Unconfirmed Data Up
    phy.mhdr.rfu = 0;
    phy.mhdr.major = 0;
    phy.macPayload = macPayload;
    
    uint8_t buf[255];
    uint8_t len = phy.serialize(buf, sizeof(buf));
    phy.mic = calculateMIC(buf, len - 4, config.nwkSKey);
    
    return phy;
}

PHYPayload_t UCDdown(const LoRaWANConfig_t& config, MACPayload_t& macPayload) {
    cryptPayload(macPayload, config, false);
    
    PHYPayload_t phy = {0};
    phy.mhdr.ftype = 0b011;  // Unconfirmed Data Down
    phy.mhdr.rfu = 0;
    phy.mhdr.major = 0;
    phy.macPayload = macPayload;
    
    uint8_t buf[255];
    uint8_t len = phy.serialize(buf, sizeof(buf));
    phy.mic = calculateMIC(buf, len - 4, config.nwkSKey);
    
    return phy;
}


// member functions
void MACPayload_t::debugPrint(HardwareSerial &serial) {
    serial.print("DevAddr: 0x");
    serial.println(devAddr, HEX);
    serial.print("FCtrl: 0x");
    serial.println(fctrl, HEX);
    serial.print("FCnt: ");
    serial.println(fcnt);
    serial.print("FPort: ");
    serial.println(fport);
    serial.print("FRMPayload: ");
    for (uint8_t i = 0; i < frmPayloadLen; i++) {
        serial.print("0x");
        serial.print(frmPayload[i], HEX);
        serial.print(" ");
    }
    serial.println();
}

void PHYPayload_t::debugPrint(HardwareSerial &serial) {
    serial.print("MHDR - MType: 0b");
    serial.print((mhdr.ftype >> 2) & 0x01);
    serial.print((mhdr.ftype >> 1) & 0x01);
    serial.print(mhdr.ftype & 0x01);
    serial.print(", RFU: 0b");
    serial.print((mhdr.rfu >> 2) & 0x01);
    serial.print((mhdr.rfu >> 1) & 0x01);
    serial.print(mhdr.rfu & 0x01);
    serial.print(", Major: 0b");
    serial.print((mhdr.major >> 1) & 0x01);
    serial.println(mhdr.major & 0x01);
    
    macPayload.debugPrint(serial);
    
    serial.print("MIC: 0x");
    serial.println(mic, HEX);
}

// packthatbih
uint8_t PHYPayload_t::serialize(uint8_t *buffer, uint8_t maxLen) {
    uint8_t index = 0;
    
    // MHDR
    buffer[index++] = (mhdr.ftype << 5) | (mhdr.rfu << 2) | mhdr.major;
    
    // MACPayload
    // DevAddr
    buffer[index++] = (macPayload.devAddr >> 0) & 0xFF;
    buffer[index++] = (macPayload.devAddr >> 8) & 0xFF;
    buffer[index++] = (macPayload.devAddr >> 16) & 0xFF;
    buffer[index++] = (macPayload.devAddr >> 24) & 0xFF;
    
    // FCtrl
    buffer[index++] = macPayload.fctrl;
    
    // FCnt
    buffer[index++] = (macPayload.fcnt >> 0) & 0xFF;
    buffer[index++] = (macPayload.fcnt >> 8) & 0xFF;
    
    // FPort
    buffer[index++] = macPayload.fport;
    
    // FRMPayload
    for (uint8_t i = 0; i < macPayload.frmPayloadLen; i++) {
        buffer[index++] = macPayload.frmPayload[i];
    }
    
    return index; // total length
}