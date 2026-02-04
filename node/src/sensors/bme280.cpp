
#include "sensors/bme280.h"

BME280::BME280(uint8_t address, TwoWire *theWire)
: Sensor({"BM"}), i2cAddress(address), wire(theWire), bme(Adafruit_BME280())
{}

bool BME280::begin() {
    if (!bme.begin(i2cAddress, wire)) {
        return false;
    }
    #ifdef DEBUG
    log("Set up sensor.", "BME280", Serial);
    #endif

    bme.setSampling(Adafruit_BME280::MODE_FORCED,
        Adafruit_BME280::SAMPLING_X4,
        Adafruit_BME280::SAMPLING_X4,
        Adafruit_BME280::SAMPLING_X4,
        Adafruit_BME280::FILTER_OFF);
    return true;
}

void BME280::show() {
    WeatherData wd = data();
    #ifndef SIMULATION_MODE
    showString(("BME280\nT: " + String(wd.temperature) + " C\nP: " + String(wd.pressure + 950) + " hPa\nH: " + String(wd.humidity / 100.0f) + " %").c_str());
    #endif
}

WeatherData BME280::data() {
    WeatherData wd;
    wd.temperature = static_cast<int8_t>(bme.readTemperature());
    float pressure_hPa = bme.readPressure() / 100.0f;
    wd.pressure = static_cast<uint8_t>(pressure_hPa - 950.0f);
    wd.humidity = static_cast<uint16_t>(bme.readHumidity() * 100);
    return wd;
}


void BME280::print(HardwareSerial& serial) {
    WeatherData wd = data();
    serial.print("Temperature = ");
    serial.print(wd.temperature);
    serial.println(" *C");

    serial.print("Pressure = ");
    serial.print(wd.pressure + 950); // add offset back
    serial.println(" hPa");

    serial.print("Humidity = ");
    serial.print(wd.humidity);
    serial.println(" %");

    serial.println();
}

uint8_t BME280::serialize(uint8_t* buffer) {
    WeatherData wd = data();
    
    int16_t temp = (int16_t)(wd.temperature * 100);  // Store as 0.01°C
    buffer[0] = temp & 0xFF;
    buffer[1] = (temp >> 8) & 0xFF;
    
    // Pressure: uint16_t (range 300-1100 hPa)
    uint16_t press = (uint16_t)(wd.pressure * 10);  // Store as 0.1 hPa
    buffer[2] = press & 0xFF;
    buffer[3] = (press >> 8) & 0xFF;
    
    // Humidity: uint16_t (0-100%)
    uint16_t hum = wd.humidity;  // Already in 0.01%
    buffer[4] = hum & 0xFF;
    buffer[5] = (hum >> 8) & 0xFF;
    
    return 6;  // 3 x int16_t = 6 bytes
}
