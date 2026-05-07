#include "BeerCoolerApp.h"

#include <cmath>
#include <cstdio>
#include <cstring>

BeerCoolerApp::BeerCoolerApp(const BeerCoolerAppConfig &config)
    : config_(config),
      connection_(config.connection),
      temperatureMonitor_(config.temperature),
      motor_(config.motor)
{
}

bool BeerCoolerApp::begin()
{
    if (config_.startupMessage != nullptr && config_.startupMessage[0] != '\0')
    {
        Serial.println(config_.startupMessage);
    }

    motor_.begin();
    temperatureMonitor_.begin();

    Serial.print("Found ");
    Serial.print(temperatureMonitor_.deviceCount());
    Serial.println(" devices.");

    hasTemperatureSensor_ = temperatureMonitor_.hasSensors();
    if (!hasTemperatureSensor_)
    {
        if (!config_.debug.allowMissingTemperatureSensor)
        {
            Serial.println("Fatal: No temperature sensor found. Aborting setup.");
            return false;
        }

        Serial.println("Temperature sensor not found. Continuing in debug mode.");
    }

    printStartupSummary();
    connection_.begin();
    return true;
}

void BeerCoolerApp::update()
{
    const unsigned long nowMs = millis();
    motor_.update(micros());

    pollMqttCommands();

    if (hasTemperatureSensor_)
    {
        float tempC = 0.0f;
        if (temperatureMonitor_.readIfDue(nowMs, tempC))
        {
            publishTemperature(tempC);
        }
    }

    runMotorDemoIfDue(nowMs);
    if (firstMotorStatePublishPending_ || nowMs - lastMotorStatePublishMs_ >= config_.debug.motorStatePublishIntervalMs)
    {
        publishMotorState();
        firstMotorStatePublishPending_ = false;
        lastMotorStatePublishMs_ = nowMs;
    }

    // Avoid verbose serial logging while stepping. It can add visible jitter.
    if (!motor_.isMoving())
    {
        printStatus(nowMs);
    }
}

void BeerCoolerApp::pollMqttCommands()
{
    ConnectionMessage message = {};
    while (connection_.poll(message, 0))
    {
        handleMqttMessage(message);
    }
}

void BeerCoolerApp::handleMqttMessage(const ConnectionMessage &message)
{
    if (strcmp(message.topic, connection_.commandTopic()) != 0)
    {
        Serial.println("Received MQTT message for unrecognized topic:");
        Serial.print("Topic: ");
        Serial.println(message.topic);
        Serial.print("Payload: ");
        Serial.println(message.payload);
        return;
    }

    Serial.print("MQTT motor command: ");
    Serial.println(message.payload);

    StepMotorCommand command = {};
    if (!parseStepMotorCommand(message.payload, command))
    {
        Serial.println("Unrecognized motor command.");
        Serial.println("Use 'steps,amount,direction', 'revolutions,amount,direction', 'meters,amount,direction',");
        Serial.println("or 'start,direction' / 'stop' / 'zero'. Direction can be forward/reverse or down/up.");
        return;
    }

    if (!motor_.runCommand(command, micros()))
    {
        Serial.println("Failed to execute step motor command.");
        return;
    }

    publishMotorState();
}

void BeerCoolerApp::publishMotorState()
{
    char payload[192];
    snprintf(
        payload,
        sizeof(payload),
        "{\"moving\":%s,\"continuous\":%s,\"direction\":\"%s\",\"travel\":\"%s\",\"position_steps\":%ld,\"position_m\":%.4f,\"depth_m\":%.4f}",
        motor_.isMoving() ? "true" : "false",
        motor_.isContinuous() ? "true" : "false",
        directionName(motor_.currentDirection()),
        travelName(motor_.currentDirection()),
        static_cast<long>(motor_.currentPositionSteps()),
        motor_.currentPositionMeters(),
        motor_.currentDepthMeters());

    if (!connection_.publishMotorState(payload))
    {
        Serial.println("Motor state publish skipped or failed");
    }
}

void BeerCoolerApp::printStartupSummary() const
{
    Serial.println("Bring-up configuration:");
    Serial.print("  Stepper steps/revolution: ");
    Serial.println(motor_.stepsPerRevolution());
    Serial.print("  Wheel diameter (m): ");
    Serial.println(motor_.wheelDiameterMeters(), 4);
    Serial.print("  Wheel circumference (m): ");
    Serial.println(motor_.wheelCircumferenceMeters(), 4);
    Serial.print("  Approx steps per meter: ");
    Serial.println(motor_.metersToSteps(1.0f));

    Serial.println("MQTT motor command examples:");
    Serial.println("  steps,1000,down");
    Serial.println("  revolutions,1,up");
    Serial.println("  meters,1,down");
    Serial.println("  start,down");
    Serial.println("  start,up");
    Serial.println("  stop");
    Serial.println("  zero");
    Serial.print("Travel mapping: forward=");
    Serial.print(travelName(StepMotorDirection::Forward));
    Serial.print(", reverse=");
    Serial.println(travelName(StepMotorDirection::Reverse));
    Serial.print("Motor state topic: ");
    Serial.println(connection_.motorStateTopic());
}

void BeerCoolerApp::printStatus(unsigned long nowMs)
{
    if (!firstStatusPending_ && nowMs - lastStatusMs_ < config_.debug.statusIntervalMs)
    {
        return;
    }

    firstStatusPending_ = false;
    lastStatusMs_ = nowMs;

    Serial.print("Status | WiFi: ");
    Serial.print(connection_.isWifiConnected() ? "connected" : "waiting");
    Serial.print(" | MQTT: ");
    Serial.print(connection_.isMqttConnected() ? "connected" : "waiting");
    Serial.print(" | Temp sensor: ");
    Serial.print(hasTemperatureSensor_ ? "present" : "missing");
    Serial.print(" | Temp devices: ");
    Serial.print(temperatureMonitor_.deviceCount());
    Serial.print(" | Depth(m): ");
    Serial.print(motor_.currentDepthMeters(), 3);
    Serial.print(" | Travel: ");
    Serial.print(travelName(motor_.currentDirection()));
    Serial.print(" | Moving: ");
    Serial.print(motor_.isMoving() ? "yes" : "no");
    Serial.print(" | Demo: ");
    Serial.println(config_.debug.enableMotorDemo ? "enabled" : "disabled");
}

void BeerCoolerApp::runMotorDemoIfDue(unsigned long nowMs)
{
    if (!config_.debug.enableMotorDemo)
    {
        return;
    }

    if (motor_.isMoving())
    {
        return;
    }

    if (!firstMotorDemoPending_ && nowMs - lastMotorDemoMs_ < config_.debug.motorDemoIntervalMs)
    {
        return;
    }

    firstMotorDemoPending_ = false;

    StepMotorCommand command = {};
    command.unit = config_.debug.motorDemoUnit;
    command.amount = config_.debug.motorDemoAmount;
    command.direction = motorDemoForward_ ? StepMotorDirection::Forward : StepMotorDirection::Reverse;

    Serial.print("Motor demo: moving ");
    Serial.print(command.amount, 3);
    Serial.print(' ');
    Serial.print(unitName(command.unit));
    Serial.print(' ');
    Serial.println(directionName(command.direction));

    if (!motor_.runCommand(command, micros()))
    {
        Serial.println("Motor demo failed.");
        return;
    }

    motorDemoForward_ = !motorDemoForward_;
    lastMotorDemoMs_ = nowMs;
    publishMotorState();
}

const char *BeerCoolerApp::directionName(StepMotorDirection direction)
{
    return direction == StepMotorDirection::Forward ? "forward" : "reverse";
}

const char *BeerCoolerApp::travelName(StepMotorDirection direction) const
{
    const bool isForward = direction == StepMotorDirection::Forward;
    const bool increasesDepth = config_.motor.forwardIncreasesDepth;
    return (isForward == increasesDepth) ? "down" : "up";
}

const char *BeerCoolerApp::unitName(StepMotorCommandUnit unit)
{
    switch (unit)
    {
    case StepMotorCommandUnit::Steps:
        return "steps";
    case StepMotorCommandUnit::Revolutions:
        return "revolutions";
    case StepMotorCommandUnit::Meters:
        return "meters";
    default:
        return "unknown";
    }
}

void BeerCoolerApp::publishTemperature(float tempC)
{
    if (std::isnan(tempC))
    {
        Serial.println("Error: Could not read temperature data");
        return;
    }

    Serial.print("Temperature: ");
    Serial.print(tempC);
    Serial.println(" C");

    if (connection_.publishTemperature(tempC))
    {
        Serial.println("Temperature published to MQTT");
    }
    else
    {
        Serial.println("MQTT publish skipped or failed");
    }
}
