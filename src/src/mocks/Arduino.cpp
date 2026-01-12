#ifdef SIMULATION_MODE
#include "mocks/Arduino.h"

// Define global Arduino objects for simulation
HardwareSerial Serial;
TwoWire Wire;

#endif // SIMULATION_MODE
