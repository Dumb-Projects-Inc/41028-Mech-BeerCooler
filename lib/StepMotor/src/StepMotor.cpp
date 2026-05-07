#include "StepMotor.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <strings.h>

namespace
{
bool parseDirectionToken(const char *token, StepMotorDirection &direction)
{
    if (token == nullptr)
    {
        return false;
    }

    if (strcasecmp(token, "forward") == 0 ||
        strcasecmp(token, "down") == 0 ||
        strcasecmp(token, "fwd") == 0 ||
        strcasecmp(token, "cw") == 0 ||
        strcmp(token, "1") == 0)
    {
        direction = StepMotorDirection::Forward;
        return true;
    }

    if (strcasecmp(token, "reverse") == 0 ||
        strcasecmp(token, "up") == 0 ||
        strcasecmp(token, "backward") == 0 ||
        strcasecmp(token, "back") == 0 ||
        strcasecmp(token, "rev") == 0 ||
        strcasecmp(token, "ccw") == 0 ||
        strcmp(token, "0") == 0 ||
        strcmp(token, "-1") == 0)
    {
        direction = StepMotorDirection::Reverse;
        return true;
    }

    return false;
}

bool parseUnitToken(const char *token, StepMotorCommandUnit &unit)
{
    if (token == nullptr)
    {
        return false;
    }

    if (strcasecmp(token, "steps") == 0 || strcasecmp(token, "step") == 0)
    {
        unit = StepMotorCommandUnit::Steps;
        return true;
    }

    if (strcasecmp(token, "revolutions") == 0 ||
        strcasecmp(token, "revolution") == 0 ||
        strcasecmp(token, "rounds") == 0 ||
        strcasecmp(token, "round") == 0 ||
        strcasecmp(token, "rev") == 0)
    {
        unit = StepMotorCommandUnit::Revolutions;
        return true;
    }

    if (strcasecmp(token, "meters") == 0 ||
        strcasecmp(token, "meter") == 0 ||
        strcmp(token, "m") == 0)
    {
        unit = StepMotorCommandUnit::Meters;
        return true;
    }

    return false;
}

bool parseActionToken(const char *token, StepMotorAction &action)
{
    if (token == nullptr)
    {
        return false;
    }

    if (strcasecmp(token, "start") == 0 || strcasecmp(token, "continuous") == 0)
    {
        action = StepMotorAction::StartContinuous;
        return true;
    }

    if (strcasecmp(token, "stop") == 0)
    {
        action = StepMotorAction::Stop;
        return true;
    }

    if (strcasecmp(token, "zero") == 0 || strcasecmp(token, "home") == 0)
    {
        action = StepMotorAction::Zero;
        return true;
    }

    return false;
}
}

bool parseStepMotorCommand(const char *payload, StepMotorCommand &command)
{
    if (payload == nullptr || payload[0] == '\0')
    {
        return false;
    }

    char unitToken[24] = {};
    char directionToken[24] = {};
    float amount = 0.0f;
    char trailing = '\0';

    StepMotorAction action = StepMotorAction::Move;
    if (sscanf(payload, " %23[^,] %c", unitToken, &trailing) == 1 && parseActionToken(unitToken, action))
    {
        command.action = action;
        return action == StepMotorAction::Stop || action == StepMotorAction::Zero;
    }

    memset(unitToken, 0, sizeof(unitToken));
    memset(directionToken, 0, sizeof(directionToken));
    const int parsedContinuous = sscanf(
        payload,
        " %23[^,] , %23[^,] %c",
        unitToken,
        directionToken,
        &trailing);

    if (parsedContinuous == 2 && parseActionToken(unitToken, action) && action == StepMotorAction::StartContinuous)
    {
        StepMotorDirection direction = StepMotorDirection::Forward;
        if (!parseDirectionToken(directionToken, direction))
        {
            return false;
        }

        command.action = action;
        command.direction = direction;
        return true;
    }

    memset(unitToken, 0, sizeof(unitToken));
    memset(directionToken, 0, sizeof(directionToken));
    const int parsedWithUnit = sscanf(
        payload,
        " %23[^,] , %f , %23[^,] %c",
        unitToken,
        &amount,
        directionToken,
        &trailing);

    if (parsedWithUnit == 3)
    {
        StepMotorCommandUnit unit = StepMotorCommandUnit::Steps;
        StepMotorDirection direction = StepMotorDirection::Forward;

        if (!parseUnitToken(unitToken, unit) || !parseDirectionToken(directionToken, direction) || amount < 0.0f)
        {
            return false;
        }

        command.action = StepMotorAction::Move;
        command.unit = unit;
        command.amount = amount;
        command.direction = direction;
        return true;
    }

    memset(directionToken, 0, sizeof(directionToken));
    const int parsedDefault = sscanf(
        payload,
        " %f , %23[^,] %c",
        &amount,
        directionToken,
        &trailing);

    if (parsedDefault == 2)
    {
        StepMotorDirection direction = StepMotorDirection::Forward;
        if (!parseDirectionToken(directionToken, direction) || amount < 0.0f)
        {
            return false;
        }

        command.action = StepMotorAction::Move;
        command.unit = StepMotorCommandUnit::Steps;
        command.amount = amount;
        command.direction = direction;
        return true;
    }

    return false;
}

StepMotor::StepMotor(const StepMotorConfig &config)
    : config_(config),
      hasEnablePin_(config.enablePin != NoEnablePin)
{
}

void StepMotor::begin()
{
    pinMode(config_.stepPin, OUTPUT);
    pinMode(config_.dirPin, OUTPUT);

    if (hasEnablePin_)
    {
        pinMode(config_.enablePin, OUTPUT);
        setEnabled(true);
    }

    digitalWrite(config_.stepPin, LOW);
    digitalWrite(config_.dirPin, LOW);
    stepPinHigh_ = false;
    lastTransitionMicros_ = micros();
}

void StepMotor::update(unsigned long nowMicros)
{
    if (!moving_)
    {
        return;
    }

    const unsigned long pulseWidthMicros = config_.pulseWidthMicros;
    while (moving_ && static_cast<unsigned long>(nowMicros - lastTransitionMicros_) >= pulseWidthMicros)
    {
        lastTransitionMicros_ += pulseWidthMicros;

        if (stepPinHigh_)
        {
            digitalWrite(config_.stepPin, LOW);
            stepPinHigh_ = false;

            currentPositionSteps_ += currentDirection_ == StepMotorDirection::Forward ? 1 : -1;

            if (!continuousMode_)
            {
                if (remainingSteps_ > 0)
                {
                    --remainingSteps_;
                }

                if (remainingSteps_ == 0)
                {
                    moving_ = false;
                }
            }
        }
        else
        {
            if (!continuousMode_ && remainingSteps_ == 0)
            {
                moving_ = false;
                break;
            }

            digitalWrite(config_.stepPin, HIGH);
            stepPinHigh_ = true;
        }
    }
}

void StepMotor::setEnabled(bool enabled)
{
    if (!hasEnablePin_)
    {
        return;
    }

    const uint8_t pinState = enabled == config_.enableActiveLow ? LOW : HIGH;
    digitalWrite(config_.enablePin, pinState);
}

void StepMotor::setPulseWidthMicros(unsigned int pulseWidthMicros)
{
    config_.pulseWidthMicros = pulseWidthMicros;
}

uint32_t StepMotor::stepsPerRevolution() const
{
    return config_.stepsPerRevolution;
}

float StepMotor::wheelDiameterMeters() const
{
    return config_.wheelDiameterMeters;
}

float StepMotor::wheelCircumferenceMeters() const
{
    return config_.wheelDiameterMeters * Pi;
}

uint32_t StepMotor::metersToSteps(float distanceMeters) const
{
    const float circumference = wheelCircumferenceMeters();
    if (circumference <= 0.0f || config_.stepsPerRevolution == 0)
    {
        return 0;
    }

    const float revolutions = distanceMeters / circumference;
    const float steps = revolutions * static_cast<float>(config_.stepsPerRevolution);
    return static_cast<uint32_t>(lroundf(steps));
}

float StepMotor::currentPositionMeters() const
{
    if (config_.stepsPerRevolution == 0)
    {
        return 0.0f;
    }

    return (static_cast<float>(currentPositionSteps_) / static_cast<float>(config_.stepsPerRevolution)) *
           wheelCircumferenceMeters();
}

float StepMotor::currentDepthMeters() const
{
    const float sign = config_.forwardIncreasesDepth ? 1.0f : -1.0f;
    return currentPositionMeters() * sign;
}

int32_t StepMotor::currentPositionSteps() const
{
    return currentPositionSteps_;
}

bool StepMotor::isMoving() const
{
    return moving_;
}

bool StepMotor::isContinuous() const
{
    return continuousMode_;
}

StepMotorDirection StepMotor::currentDirection() const
{
    return currentDirection_;
}

void StepMotor::moveSteps(uint32_t steps, bool direction)
{
    startMoveSteps(steps, direction ? StepMotorDirection::Forward : StepMotorDirection::Reverse, micros());
    while (isMoving())
    {
        update(micros());
        yield();
    }
}

void StepMotor::moveRevolutions(float revolutions, bool direction)
{
    if (revolutions <= 0.0f)
    {
        return;
    }

    const uint32_t steps = static_cast<uint32_t>(lroundf(revolutions * static_cast<float>(config_.stepsPerRevolution)));
    moveSteps(steps, direction);
}

void StepMotor::moveMeters(float distanceMeters, bool direction)
{
    if (distanceMeters <= 0.0f)
    {
        return;
    }

    moveSteps(metersToSteps(distanceMeters), direction);
}

void StepMotor::beginMove(StepMotorDirection direction, unsigned long nowMicros)
{
    currentDirection_ = direction;
    digitalWrite(config_.dirPin, direction == StepMotorDirection::Forward ? HIGH : LOW);
    setEnabled(true);
    moving_ = true;
    stepPinHigh_ = false;
    digitalWrite(config_.stepPin, LOW);
    lastTransitionMicros_ = nowMicros - config_.pulseWidthMicros;
}

void StepMotor::startMoveSteps(uint32_t steps, StepMotorDirection direction, unsigned long nowMicros)
{
    if (steps == 0)
    {
        stop();
        return;
    }

    continuousMode_ = false;
    remainingSteps_ = steps;
    beginMove(direction, nowMicros);
}

void StepMotor::startMoveRevolutions(float revolutions, StepMotorDirection direction, unsigned long nowMicros)
{
    if (revolutions <= 0.0f)
    {
        stop();
        return;
    }

    const uint32_t steps = static_cast<uint32_t>(lroundf(revolutions * static_cast<float>(config_.stepsPerRevolution)));
    startMoveSteps(steps, direction, nowMicros);
}

void StepMotor::startMoveMeters(float distanceMeters, StepMotorDirection direction, unsigned long nowMicros)
{
    if (distanceMeters <= 0.0f)
    {
        stop();
        return;
    }

    startMoveSteps(metersToSteps(distanceMeters), direction, nowMicros);
}

void StepMotor::startContinuous(StepMotorDirection direction, unsigned long nowMicros)
{
    continuousMode_ = true;
    remainingSteps_ = 0;
    beginMove(direction, nowMicros);
}

void StepMotor::stop()
{
    moving_ = false;
    continuousMode_ = false;
    remainingSteps_ = 0;
    stepPinHigh_ = false;
    digitalWrite(config_.stepPin, LOW);
}

void StepMotor::zeroPosition()
{
    currentPositionSteps_ = 0;
}

bool StepMotor::runCommand(const StepMotorCommand &command, unsigned long nowMicros)
{
    switch (command.action)
    {
    case StepMotorAction::Move:
        switch (command.unit)
        {
        case StepMotorCommandUnit::Steps:
            startMoveSteps(static_cast<uint32_t>(lroundf(command.amount)), command.direction, nowMicros);
            return true;
        case StepMotorCommandUnit::Revolutions:
            startMoveRevolutions(command.amount, command.direction, nowMicros);
            return true;
        case StepMotorCommandUnit::Meters:
            startMoveMeters(command.amount, command.direction, nowMicros);
            return true;
        default:
            return false;
        }
    case StepMotorAction::StartContinuous:
        startContinuous(command.direction, nowMicros);
        return true;
    case StepMotorAction::Stop:
        stop();
        return true;
    case StepMotorAction::Zero:
        zeroPosition();
        return true;
    default:
        return false;
    }
}
