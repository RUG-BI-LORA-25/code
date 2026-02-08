#ifndef _RADIOLIB_EX_LORAWAN_CONFIG_H
#define _RADIOLIB_EX_LORAWAN_CONFIG_H

#include <RadioLib.h>
const uint32_t uplinkIntervalSeconds = 5UL * 60UL;    // minutes x seconds (unused, kept for reference)
#define RADIOLIB_LORAWAN_JOIN_EUI  0x156962cc56e08ec0

#ifndef RADIOLIB_LORAWAN_DEV_EUI   // Replace with your Device EUI
#define RADIOLIB_LORAWAN_DEV_EUI   0x975c335979fc518c
#endif
#ifndef RADIOLIB_LORAWAN_APP_KEY   // Replace with your App Key 
#define RADIOLIB_LORAWAN_APP_KEY  0xa0, 0x54, 0x7e, 0x61, 0xf1, 0x55, 0x86, 0xda, 0x03, 0x60, 0x0b, 0x44, 0x18, 0x0b, 0x69, 0xec
#endif
#ifndef RADIOLIB_LORAWAN_NWK_KEY   // Put your Nwk Key here
#define RADIOLIB_LORAWAN_NWK_KEY   0xb6, 0x75, 0x5b, 0xab, 0xbf, 0x34, 0x9c, 0xa2, 0x37, 0x7e, 0x4e, 0x6f, 0xe2, 0x25, 0x29, 0x69 
#endif

// Custom single-channel EU433 band for HackRF gateway
// Based on EU433 but all 3 TX channels + RX2 pinned to 433.175 MHz
// so the node never hops away from the single HackRF frequency.
const LoRaWANBand_t EU433_Single = {
  .bandNum = BandEU433,
  .bandType = RADIOLIB_LORAWAN_BAND_DYNAMIC,
  .freqMin = 4330000,
  .freqMax = 4340000,
  .payloadLenMax = { 51, 51, 51, 115, 242, 242, 242, 242, 0, 0, 0, 0, 0, 0, 0 },
  .powerMax = 12,
  .powerNumSteps = 5,
  .dutyCycle = 36000,
  .dwellTimeUp = 0,
  .dwellTimeDn = 0,
  .txParamSupported = false,
  .txFreqs = {
    { .idx = 0, .freq = 4331750, .drMin = 0, .drMax = 5, .dr = 3 },
    { .idx = 1, .freq = 4331750, .drMin = 0, .drMax = 5, .dr = 3 },
    { .idx = 2, .freq = 4331750, .drMin = 0, .drMax = 5, .dr = 3 },
  },
  .numTxSpans = 0,
  .txSpans = {
    RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE,
    RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE
  },
  .rx1Span = RADIOLIB_LORAWAN_CHANNEL_SPAN_NONE,
  .rx1DrTable = {
    {    0,    0,    0,    0,    0,    0, 0x0F, 0x0F },
    {    1,    0,    0,    0,    0,    0, 0x0F, 0x0F },
    {    2,    1,    0,    0,    0,    0, 0x0F, 0x0F },
    {    3,    2,    1,    0,    0,    0, 0x0F, 0x0F },
    {    4,    3,    2,    1,    0,    0, 0x0F, 0x0F },
    {    5,    4,    3,    2,    1,    0, 0x0F, 0x0F },
    {    6,    5,    4,    3,    2,    1, 0x0F, 0x0F },
    {    7,    6,    5,    4,    3,    2, 0x0F, 0x0F },
    { 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F },
    { 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F },
    { 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F },
    { 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F },
    { 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F },
    { 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F },
    { 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F }
  },
  .rx2 = { .idx = 0, .freq = 4331750, .drMin = 0, .drMax = 7, .dr = 0 },
  .txWoR = {
    RADIOLIB_LORAWAN_CHANNEL_NONE,
    RADIOLIB_LORAWAN_CHANNEL_NONE
  },
  .txAck = {
    RADIOLIB_LORAWAN_CHANNEL_NONE,
    RADIOLIB_LORAWAN_CHANNEL_NONE
  },
  .dataRates = {
    { .modem = RADIOLIB_MODEM_LORA, .dr = {.lora = {12, 125, 5}}, .pc = {.lora = {8, false, true, true}}},
    { .modem = RADIOLIB_MODEM_LORA, .dr = {.lora = {11, 125, 5}}, .pc = {.lora = {8, false, true, true}}},
    { .modem = RADIOLIB_MODEM_LORA, .dr = {.lora = {10, 125, 5}}, .pc = {.lora = {8, false, true, false}}},
    { .modem = RADIOLIB_MODEM_LORA, .dr = {.lora = { 9, 125, 5}}, .pc = {.lora = {8, false, true, false}}},
    { .modem = RADIOLIB_MODEM_LORA, .dr = {.lora = { 8, 125, 5}}, .pc = {.lora = {8, false, true, false}}},
    { .modem = RADIOLIB_MODEM_LORA, .dr = {.lora = { 7, 125, 5}}, .pc = {.lora = {8, false, true, false}}},
    { .modem = RADIOLIB_MODEM_LORA, .dr = {.lora = { 7, 250, 5}}, .pc = {.lora = {8, false, true, false}}},
    { .modem = RADIOLIB_MODEM_FSK,  .dr = {.fsk  = {50, 25}},     .pc = {.fsk  = {40, 24, 2}}},
    RADIOLIB_DATARATE_NONE,
    RADIOLIB_DATARATE_NONE,
    RADIOLIB_DATARATE_NONE,
    RADIOLIB_DATARATE_NONE,
    RADIOLIB_DATARATE_NONE,
    RADIOLIB_DATARATE_NONE,
    RADIOLIB_DATARATE_NONE
  }
};

const LoRaWANBand_t& Region = EU433_Single;

const uint8_t subBand = 0;

uint64_t joinEUI =   RADIOLIB_LORAWAN_JOIN_EUI;
uint64_t devEUI  =   RADIOLIB_LORAWAN_DEV_EUI;
uint8_t appKey[] = { RADIOLIB_LORAWAN_APP_KEY };
uint8_t nwkKey[] = { RADIOLIB_LORAWAN_NWK_KEY };

// create the LoRaWAN node

// result code to text - these are error codes that can be raised when using LoRaWAN
// however, RadioLib has many more - see https://jgromes.github.io/RadioLib/group__status__codes.html for a complete list
String stateDecode(const int16_t result) {
  switch (result) {
  case RADIOLIB_ERR_NONE:
    return "ERR_NONE";
  case RADIOLIB_ERR_CHIP_NOT_FOUND:
    return "ERR_CHIP_NOT_FOUND";
  case RADIOLIB_ERR_PACKET_TOO_LONG:
    return "ERR_PACKET_TOO_LONG";
  case RADIOLIB_ERR_RX_TIMEOUT:
    return "ERR_RX_TIMEOUT";
  case RADIOLIB_ERR_MIC_MISMATCH:
    return "ERR_MIC_MISMATCH";
  case RADIOLIB_ERR_INVALID_BANDWIDTH:
    return "ERR_INVALID_BANDWIDTH";
  case RADIOLIB_ERR_INVALID_SPREADING_FACTOR:
    return "ERR_INVALID_SPREADING_FACTOR";
  case RADIOLIB_ERR_INVALID_CODING_RATE:
    return "ERR_INVALID_CODING_RATE";
  case RADIOLIB_ERR_INVALID_FREQUENCY:
    return "ERR_INVALID_FREQUENCY";
  case RADIOLIB_ERR_INVALID_OUTPUT_POWER:
    return "ERR_INVALID_OUTPUT_POWER";
  case RADIOLIB_ERR_NETWORK_NOT_JOINED:
	  return "RADIOLIB_ERR_NETWORK_NOT_JOINED";
  case RADIOLIB_ERR_DOWNLINK_MALFORMED:
    return "RADIOLIB_ERR_DOWNLINK_MALFORMED";
  case RADIOLIB_ERR_INVALID_REVISION:
    return "RADIOLIB_ERR_INVALID_REVISION";
  case RADIOLIB_ERR_INVALID_PORT:
    return "RADIOLIB_ERR_INVALID_PORT";
  case RADIOLIB_ERR_NO_RX_WINDOW:
    return "RADIOLIB_ERR_NO_RX_WINDOW";
  case RADIOLIB_ERR_INVALID_CID:
    return "RADIOLIB_ERR_INVALID_CID";
  case RADIOLIB_ERR_UPLINK_UNAVAILABLE:
    return "RADIOLIB_ERR_UPLINK_UNAVAILABLE";
  case RADIOLIB_ERR_COMMAND_QUEUE_FULL:
    return "RADIOLIB_ERR_COMMAND_QUEUE_FULL";
  case RADIOLIB_ERR_COMMAND_QUEUE_ITEM_NOT_FOUND:
    return "RADIOLIB_ERR_COMMAND_QUEUE_ITEM_NOT_FOUND";
  case RADIOLIB_ERR_JOIN_NONCE_INVALID:
    return "RADIOLIB_ERR_JOIN_NONCE_INVALID";
  case RADIOLIB_ERR_DWELL_TIME_EXCEEDED:
    return "RADIOLIB_ERR_DWELL_TIME_EXCEEDED";
  case RADIOLIB_ERR_CHECKSUM_MISMATCH:
    return "RADIOLIB_ERR_CHECKSUM_MISMATCH";
  case RADIOLIB_ERR_NO_JOIN_ACCEPT:
    return "RADIOLIB_ERR_NO_JOIN_ACCEPT";
  case RADIOLIB_LORAWAN_SESSION_RESTORED:
    return "RADIOLIB_LORAWAN_SESSION_RESTORED";
  case RADIOLIB_LORAWAN_NEW_SESSION:
    return "RADIOLIB_LORAWAN_NEW_SESSION";
  case RADIOLIB_ERR_NONCES_DISCARDED:
    return "RADIOLIB_ERR_NONCES_DISCARDED";
  case RADIOLIB_ERR_SESSION_DISCARDED:
    return "RADIOLIB_ERR_SESSION_DISCARDED";
  }
  return "See https://jgromes.github.io/RadioLib/group__status__codes.html";
}

// helper function to display any issues
void debug(bool failed, const __FlashStringHelper* message, int state, bool halt) {
  if(failed) {
    Serial.print(message);
    Serial.print(" - ");
    Serial.print(stateDecode(state));
    Serial.print(" (");
    Serial.print(state);
    Serial.println(")");
    while(halt) { delay(1); }
  }
}

// helper function to display a byte array
void arrayDump(uint8_t *buffer, uint16_t len) {
  for(uint16_t c = 0; c < len; c++) {
    char b = buffer[c];
    if(b < 0x10) { Serial.print('0'); }
    Serial.print(b, HEX);
  }
  Serial.println();
}

#endif