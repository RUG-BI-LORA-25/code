#pragma once

#include <cstdint>
#include <iostream>

class HardwareSerial {
public:
    void begin(unsigned long baud) {}
    void print(const char* str) { std::cout << str; }
    void print(float val) { std::cout << val; }
    void println(const char* str) { std::cout << str << std::endl; }
    void println(float val) { std::cout << val << std::endl; }
    void println() { std::cout << std::endl; }
};

class TwoWire {
public:
    void begin() {}
};
