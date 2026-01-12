#pragma once

// Core Includes
#ifdef SIMULATION_MODE
#include "mocks/Arduino.h"
#else
#include <Arduino.h>
#endif
#include "shared.h"

// sensors
#include "sensors/bme280.h"
#include "sensors/photoresistor.h"
#include "sensors/sensor.h"

// lora
#include "lora/sx1278.h"

// display
#include <Adafruit_SSD1306.h>
