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
    _hasBegun = true;
}

float DS18B20::readCelsius(uint8_t index)
{
    if (!_hasBegun)
    {
        begin();
    }

    _sensors.requestTemperatures();

    float tempC = _sensors.getTempCByIndex(index);

    if (tempC == DEVICE_DISCONNECTED_C)
    {
        return NAN;
    }

    return tempC;
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