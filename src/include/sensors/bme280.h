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
    int8_t temperature;    // -128 to +127°C, 1°C resolution
    uint16_t pressure;
    uint16_t humidity;   
} WeatherData;

class BME280 : public Sensor {
public:
    BME280(uint8_t address = BME280_I2C_ADDR, TwoWire *theWire = nullptr);
    WeatherData data();
    uint8_t serialize(uint8_t* buffer) override;
    bool begin() override;
    void print(HardwareSerial& serial) override;

private:
    Adafruit_BME280 bme;
    uint8_t i2cAddress;
    TwoWire *wire;
};
