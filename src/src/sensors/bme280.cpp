
#include "sensors/bme280.h"

BME280::BME280(uint8_t address, TwoWire *theWire)
: i2cAddress(address), wire(theWire), bme(Adafruit_BME280())
{}

bool BME280::begin() {
    if (!bme.begin(i2cAddress, wire)) {
        return false;
    }
    bme.setSampling(Adafruit_BME280::MODE_FORCED,
        Adafruit_BME280::SAMPLING_X4,
        Adafruit_BME280::SAMPLING_X4,
        Adafruit_BME280::SAMPLING_X4,
        Adafruit_BME280::FILTER_OFF);
    return true;
}

WeatherData BME280::data() {
    WeatherData wd;
    wd.temperature = bme.readTemperature();
    wd.pressure = bme.readPressure() / 100.0F;
    wd.humidity = bme.readHumidity();
    return wd;
}

float BME280::temperature() {
    return bme.readTemperature();
}

float BME280::pressure() {
    return bme.readPressure() / 100.0F;
}

float BME280::humidity() {
    return bme.readHumidity();
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