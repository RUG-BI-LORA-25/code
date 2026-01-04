
#include "sensors/bme280.h"

BME280::BME280(uint8_t address, TwoWire *theWire)
: i2cAddress(address), wire(theWire) 
#ifndef SIMULATION_MODE
, bme(Adafruit_BME280())
#endif
{}

bool BME280::begin() {
#ifdef SIMULATION_MODE
    return true;
#else
    if (!bme.begin(i2cAddress, wire)) {
        return false;
    }
    bme.setSampling(Adafruit_BME280::MODE_FORCED,
        Adafruit_BME280::SAMPLING_X4,
        Adafruit_BME280::SAMPLING_X4,
        Adafruit_BME280::SAMPLING_X4,
        Adafruit_BME280::FILTER_OFF);
    return true;
#endif
}

WeatherData BME280::data() {
    WeatherData wd;
#ifdef SIMULATION_MODE
    wd.temperature = 22.5f;
    wd.pressure = 1013.25f;
    wd.humidity = 45.0f;
#else
    wd.temperature = bme.readTemperature();
    wd.pressure = bme.readPressure() / 100.0F;
    wd.humidity = bme.readHumidity();
#endif
    return wd;
}

float BME280::temperature() {
#ifdef SIMULATION_MODE
    return 22.5f;
#else
    return bme.readTemperature();
#endif
}

float BME280::pressure() {
#ifdef SIMULATION_MODE
    return 1013.25f;
#else
    return bme.readPressure() / 100.0F;
#endif
}

float BME280::humidity() {
#ifdef SIMULATION_MODE
    return 45.0f;
#else
    return bme.readHumidity();
#endif
}

void BME280::print(HardwareSerial& serial) {
    WeatherData wd = data();
    serial.print("Temperature = ");
    serial.print(wd.temperature);
    serial.println(" *C");

    serial.print("Pressure = ");
    serial.print(wd.pressure);
    serial.println(" hPa");

    serial.print("Humidity = ");
    serial.print(wd.humidity);
    serial.println(" %");

    serial.println();
}