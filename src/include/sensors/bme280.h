#pragma once

#ifdef SIMULATION_MODE
#include "mocks/Arduino.h"
#include "mocks/Adafruit_Sensor.h"
#include "mocks/Adafruit_BME280.h"
#else
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#endif

#include "shared.h"
#include "sensors/sensor.h"


typedef struct {
    float temperature;
    float pressure;
    float humidity;
} WeatherData;

class BME280 : public Sensor {
public:
    BME280(uint8_t address = BME280_I2C_ADDR, TwoWire *theWire = nullptr);
    bool begin();
    WeatherData data();
    float temperature();
    float pressure();
    float humidity();

    void print(HardwareSerial& serial);

private:
    Adafruit_BME280 bme;
    uint8_t i2cAddress;
    TwoWire *wire;
};
