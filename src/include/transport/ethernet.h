#pragma once

#include "shared.h"

#ifndef SIMULATION_MODE
#include <SPI.h>
#endif

class EthernetModule {
public:
    EthernetModule(const byte* mac, const byte* ip = nullptr);
    bool begin();
    void tick();

private:
    const byte* macAddress;
    const byte* ipAddress;
};