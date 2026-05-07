#include "DS18B20.h"

DS18B20::DS18B20(uint8_t pin)
    : _pin(pin),
      _oneWire(pin),
      _sensors(&_oneWire)
{
}

void DS18B20::begin()
{
    _sensors.begin();
    _sensors.setWaitForConversion(false);
    _hasBegun = true;
}

void DS18B20::requestTemperatures()
{
    if (!_hasBegun)
    {
        begin();
    }

    _sensors.requestTemperatures();
}

float DS18B20::readLastCelsius(uint8_t index)
{
    if (!_hasBegun)
    {
        begin();
    }

    float tempC = _sensors.getTempCByIndex(index);

    if (tempC == DEVICE_DISCONNECTED_C)
    {
        return NAN;
    }

    return tempC;
}

float DS18B20::readCelsius(uint8_t index)
{
    requestTemperatures();
    delay(conversionTimeMs(index));
    return readLastCelsius(index);
}

bool DS18B20::isConnected(uint8_t index)
{
    if (!_hasBegun)
    {
        begin();
    }

    DeviceAddress address;

    return _sensors.getAddress(address, index);
}

uint8_t DS18B20::getDeviceCount()
{
    if (!_hasBegun)
    {
        begin();
    }

    return _sensors.getDeviceCount();
}

void DS18B20::setResolution(uint8_t resolution)
{
    if (!_hasBegun)
    {
        begin();
    }

    if (resolution < 9)
    {
        resolution = 9;
    }

    if (resolution > 12)
    {
        resolution = 12;
    }

    _sensors.setResolution(resolution);
}

uint8_t DS18B20::getResolution(uint8_t index)
{
    if (!_hasBegun)
    {
        begin();
    }

    DeviceAddress address;

    if (!_sensors.getAddress(address, index))
    {
        return 0;
    }

    return _sensors.getResolution(address);
}

unsigned long DS18B20::conversionTimeMs(uint8_t index)
{
    const uint8_t resolution = getResolution(index);

    switch (resolution)
    {
    case 9:
        return 94;
    case 10:
        return 188;
    case 11:
        return 375;
    case 12:
    default:
        return 750;
    }
}
