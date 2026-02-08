#pragma once

#include "shared.h"
#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>

class LORA{
   public:
    LORA(int nss, int reset, int dio0, int dio1, SPIClass& spi);
    void begin();
    void reBegin(State target);
    int16_t setFrequency(float freq);
    int16_t setBandwidth(float bw);
    int16_t setSpreadingFactor(uint8_t sf);
    int16_t setPower(int8_t power);
    int16_t setGain(uint8_t gain);
    int16_t sendData(const uint8_t* data, size_t len, uint8_t* dlBuf = nullptr, size_t* dlLen = nullptr);
    void applyOverrides();
    
   private:
    LoRaWANNode node;
    SX1278 radio;
    int pin_nss;
    int pin_reset; 
    int pin_dio0;   
    int pin_dio1;
    int targetDr = 0;  // DR requested by algo, used in applyOverrides()
};