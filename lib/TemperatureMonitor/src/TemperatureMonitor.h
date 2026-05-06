#pragma once

#include <Arduino.h>
#include <DS18B20.h>

struct TemperatureMonitorConfig
{
    uint8_t oneWirePin;
    unsigned long publishIntervalMs = 2000;
    uint8_t sensorIndex = 0;
};

class TemperatureMonitor
{
public:
    explicit TemperatureMonitor(const TemperatureMonitorConfig &config);

    void begin();
    bool hasSensors() const;
    uint8_t deviceCount() const;
    bool read(float &tempC);
    bool readIfDue(unsigned long nowMs, float &tempC);

private:
    TemperatureMonitorConfig config_;
    DS18B20 sensor_;
    uint8_t deviceCount_ = 0;
    unsigned long lastReadMs_ = 0;
    unsigned long conversionStartedMs_ = 0;
    unsigned long conversionWaitMs_ = 750;
    bool firstReadPending_ = true;
    bool conversionPending_ = false;
};
