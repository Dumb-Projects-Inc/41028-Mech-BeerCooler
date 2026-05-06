#pragma once

#include <Arduino.h>
#include <ConnectionManager.h>
#include <StepMotor.h>
#include <TemperatureMonitor.h>

struct BeerCoolerAppConfig
{
    struct DebugConfig
    {
        bool allowMissingTemperatureSensor = false;
        bool enableMotorDemo = false;
        StepMotorCommandUnit motorDemoUnit = StepMotorCommandUnit::Revolutions;
        float motorDemoAmount = 1.0f;
        unsigned long motorDemoIntervalMs = 15000;
        unsigned long statusIntervalMs = 5000;
        unsigned long motorStatePublishIntervalMs = 500;
    };

    ConnectionManagerConfig connection;
    TemperatureMonitorConfig temperature;
    StepMotorConfig motor;
    DebugConfig debug;
    const char *startupMessage = nullptr;
};

class BeerCoolerApp
{
public:
    explicit BeerCoolerApp(const BeerCoolerAppConfig &config);

    bool begin();
    void update();

private:
    void pollMqttCommands();
    void handleMqttMessage(const ConnectionMessage &message);
    void publishTemperature(float tempC);
    void publishMotorState();
    void printStartupSummary() const;
    void printStatus(unsigned long nowMs);
    void runMotorDemoIfDue(unsigned long nowMs);
    static const char *directionName(StepMotorDirection direction);
    const char *travelName(StepMotorDirection direction) const;
    static const char *unitName(StepMotorCommandUnit unit);

    BeerCoolerAppConfig config_;
    ConnectionManager connection_;
    TemperatureMonitor temperatureMonitor_;
    StepMotor motor_;
    bool hasTemperatureSensor_ = false;
    bool motorDemoForward_ = true;
    bool firstStatusPending_ = true;
    bool firstMotorDemoPending_ = true;
    bool firstMotorStatePublishPending_ = true;
    unsigned long lastStatusMs_ = 0;
    unsigned long lastMotorDemoMs_ = 0;
    unsigned long lastMotorStatePublishMs_ = 0;
};
