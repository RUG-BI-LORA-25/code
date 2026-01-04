#ifndef ADAFRUIT_BME280_MOCK_H
#define ADAFRUIT_BME280_MOCK_H

#include <cstdint>

class TwoWire;

class Adafruit_BME280 {
public:
    enum { MODE_FORCED, SAMPLING_X4, FILTER_OFF };
    
    Adafruit_BME280() {}
    bool begin(uint8_t addr = 0x77, TwoWire* theWire = nullptr) { return true; }
    void setSampling(int, int, int, int, int) {}
    float readTemperature() { return 22.5f; }
    float readPressure() { return 101325.0f; }
    float readHumidity() { return 45.0f; }
};

#endif // ADAFRUIT_BME280_MOCK_H

