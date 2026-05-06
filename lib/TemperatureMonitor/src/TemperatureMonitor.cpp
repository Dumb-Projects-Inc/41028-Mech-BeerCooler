#include "TemperatureMonitor.h"

#include <cmath>

TemperatureMonitor::TemperatureMonitor(const TemperatureMonitorConfig &config)
    : config_(config),
      sensor_(config.oneWirePin)
{
}

void TemperatureMonitor::begin()
{
    sensor_.begin();
    deviceCount_ = sensor_.getDeviceCount();
    if (deviceCount_ > 0)
    {
        conversionWaitMs_ = sensor_.conversionTimeMs(config_.sensorIndex);
    }
}

bool TemperatureMonitor::hasSensors() const
{
    return deviceCount_ > 0;
}

uint8_t TemperatureMonitor::deviceCount() const
{
    return deviceCount_;
}

bool TemperatureMonitor::read(float &tempC)
{
    tempC = sensor_.readCelsius(config_.sensorIndex);
    return !std::isnan(tempC);
}

bool TemperatureMonitor::readIfDue(unsigned long nowMs, float &tempC)
{
    if (conversionPending_)
    {
        if (nowMs - conversionStartedMs_ < conversionWaitMs_)
        {
            return false;
        }

        conversionPending_ = false;
        lastReadMs_ = nowMs;
        tempC = sensor_.readLastCelsius(config_.sensorIndex);
        return true;
    }

    if (!firstReadPending_ && nowMs - lastReadMs_ < config_.publishIntervalMs)
    {
        return false;
    }

    firstReadPending_ = false;
    conversionPending_ = true;
    conversionStartedMs_ = nowMs;
    sensor_.requestTemperatures();
    return false;
}
