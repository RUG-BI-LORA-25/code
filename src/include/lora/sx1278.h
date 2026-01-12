#pragma once

#include "shared.h"
#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>

class LORA{
   public:
    LORA(int nss, int reset, int dio0, int dio1);
    void begin();

   private:
    SX1278* radio;
    int pin_nss;
    int pin_reset; 
    int pin_dio0;   
    int pin_dio1;
};