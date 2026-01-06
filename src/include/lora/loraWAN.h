#pragma once
#include "shared.h"

// https://lora-alliance.org/wp-content/uploads/2021/11/LoRaWAN-Link-Layer-Specification-v1.0.4.pdf
// PHYPayload = MHDR (1) | MACPayload (7-M) | MIC (4)

// LoRaWAN Configuration
struct LoRaWANConfig_t {
  // OTAA (Over-The-Air Activation) keys
  uint8_t devEUI[8];  // Device EUI (globally unique identifier)
  uint8_t appEUI[8];  // Application EUI (Join Server identifier)
  uint8_t appKey[16]; // Application Key (128-bit AES key)

  // ABP (Activation By Personalization) keys
  uint32_t devAddr;    // Device Address (assigned after join or pre-configured)
  uint8_t nwkSKey[16]; // Network Session Key (for MIC calculation)
  uint8_t appSKey[16]; // Application Session Key (for payload encryption)

  // Session state
  uint16_t fcntUp;   // Uplink frame counter
  uint16_t fcntDown; // Downlink frame counter
};

struct MHDR_t {
  uint8_t major : 2;
  uint8_t rfu : 3;
  // FType | Description
  // ------|---------------------------
  //  000  | Join-Request
  //  001  | Join-Accept
  //  010  | Unconfirmed Data Uplink
  //  011  | Unconfirmed Data Downlink
  //  100  | Confirmed Data Uplink
  //  101  | Confirmed Data Downlink
  //  110  | RFU
  //  111  | Proprietary
  uint8_t ftype : 3;
};

struct MACPayload_t {
  uint32_t devAddr;
  uint8_t fctrl;
  uint16_t fcnt;
  uint8_t fport;
  uint8_t *frmPayload;
  uint8_t frmPayloadLen;

  void debugPrint(HardwareSerial &serial);
};

struct JoinRequestPayload_t {
    uint8_t appEUI[8];
    uint8_t devEUI[8];
    uint16_t devNonce;
};

struct PHYPayload_t {
    MHDR_t mhdr;
    union {
        MACPayload_t macPayload; // data
        JoinRequestPayload_t joinPayload;  // join
    };
    uint32_t mic;
    
    void debugPrint(HardwareSerial &serial);
    uint8_t serialize(uint8_t *buffer, uint8_t maxLen);
};

uint32_t calculateMIC(const uint8_t *data, uint8_t len,
                      const uint8_t *key);
// aes-128-ctr is symmetric, which is cool
void cryptPayload(MACPayload_t &macPayload, const LoRaWANConfig_t &config,
                  bool uplink);

PHYPayload_t UCDup(const LoRaWANConfig_t &config, uint8_t fport,
                   const uint8_t *payload, uint8_t payloadLen);
PHYPayload_t UCDdown(const LoRaWANConfig_t &config, uint8_t fport,
                  const uint8_t *payload, uint8_t payloadLen);
PHYPayload_t joinRequest(const LoRaWANConfig_t &config, uint16_t devNonce);

// Message type constants
#define MTYPE_JOIN_REQUEST 0x00
#define MTYPE_JOIN_ACCEPT 0x01
#define MTYPE_UNCONFIRMED_UP 0x02
#define MTYPE_UNCONFIRMED_DOWN 0x03
#define MTYPE_CONFIRMED_UP 0x04
#define MTYPE_CONFIRMED_DOWN 0x05
#define MTYPE_PROPRIETARY 0x07

// FCtrl bit positions
#define FCTRL_ADR 0x80
#define FCTRL_ADRACKREQ 0x40
#define FCTRL_ACK 0x20
#define FCTRL_FPENDING 0x10
#define FCTRL_FOPTSLEN 0x0F