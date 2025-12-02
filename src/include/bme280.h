#ifndef BME280_H
#define BME280_H

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

class HardwareSerial;

typedef struct {
    float temperature;
    float pressure;
    float humidity;
} WeatherData;

class BME280 {
public:
    BME280(uint8_t address = 0x76, TwoWire *theWire = &Wire);
    bool begin();
    WeatherData data();
    float temperature();
    float pressure();
    float humidity();

    void print(HardwareSerial& serial); // for debugging

private:
    Adafruit_BME280 bme;
    uint8_t i2cAddress;
    TwoWire *wire;
};
#endif // BME280_H
