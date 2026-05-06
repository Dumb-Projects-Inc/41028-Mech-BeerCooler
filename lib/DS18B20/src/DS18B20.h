#pragma once

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

class DS18B20
{
public:
    explicit DS18B20(uint8_t pin);

    void begin();

    void requestTemperatures();
    float readCelsius(uint8_t index = 0);
    float readLastCelsius(uint8_t index = 0);

    bool isConnected(uint8_t index = 0);
    uint8_t getDeviceCount();

    void setResolution(uint8_t resolution);
    uint8_t getResolution(uint8_t index = 0);
    unsigned long conversionTimeMs(uint8_t index = 0);

private:
    uint8_t _pin;

    OneWire _oneWire;
    DallasTemperature _sensors;

    bool _hasBegun = false;
};
