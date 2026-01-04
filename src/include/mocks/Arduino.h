#ifndef ARDUINO_MOCK_H
#define ARDUINO_MOCK_H

// This is LLM'ed, full disclosure.
#include <cstdint>
#include <iostream>
#include <string>

typedef uint8_t byte;
typedef bool boolean;

#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2

#define LOW 0
#define HIGH 1


inline void pinMode(uint8_t pin, uint8_t mode) {}
inline void digitalWrite(uint8_t pin, uint8_t value) {}
inline int digitalRead(uint8_t pin) { return 0; }
inline int analogRead(uint8_t pin) { return 0; }
inline void analogWrite(uint8_t pin, int value) {}
inline void delay(unsigned long ms) {}
inline void delayMicroseconds(unsigned int us) {}
inline unsigned long millis() { return 0; }
inline unsigned long micros() { return 0; }

class HardwareSerial {
public:
    void begin(unsigned long baud) {}
    void end() {}
    
    void print(const char* str) { std::cout << str; }
    void print(char c) { std::cout << c; }
    void print(int n) { std::cout << n; }
    void print(unsigned int n) { std::cout << n; }
    void print(long n) { std::cout << n; }
    void print(unsigned long n) { std::cout << n; }
    void print(float n, int digits = 2) { std::cout << n; }
    void print(double n, int digits = 2) { std::cout << n; }
    
    void println(const char* str) { std::cout << str << std::endl; }
    void println(char c) { std::cout << c << std::endl; }
    void println(int n) { std::cout << n << std::endl; }
    void println(unsigned int n) { std::cout << n << std::endl; }
    void println(long n) { std::cout << n << std::endl; }
    void println(unsigned long n) { std::cout << n << std::endl; }
    void println(float n, int digits = 2) { std::cout << n << std::endl; }
    void println(double n, int digits = 2) { std::cout << n << std::endl; }
    void println() { std::cout << std::endl; }
    
    size_t write(uint8_t byte) { return 1; }
    int available() { return 0; }
    int read() { return -1; }
    int peek() { return -1; }
    void flush() {}
};

extern HardwareSerial Serial;

class TwoWire {
public:
    TwoWire() {}
    TwoWire(uint8_t sda, uint8_t scl) {}
    TwoWire(int sda, int scl) {}
    
    void begin() {}
    void begin(uint8_t address) {}
    void begin(int address) {}
    
    void beginTransmission(uint8_t address) {}
    void beginTransmission(int address) {}
    
    uint8_t endTransmission(bool sendStop = true) { return 0; }
    
    uint8_t requestFrom(uint8_t address, uint8_t quantity, bool sendStop = true) { return 0; }
    uint8_t requestFrom(int address, int quantity, bool sendStop = true) { return 0; }
    
    size_t write(uint8_t data) { return 1; }
    size_t write(const uint8_t* data, size_t quantity) { return quantity; }
    
    int available() { return 0; }
    int read() { return -1; }
    int peek() { return -1; }
    
    void setClock(uint32_t frequency) {}
};

extern TwoWire Wire;

#endif // ARDUINO_MOCK_H
