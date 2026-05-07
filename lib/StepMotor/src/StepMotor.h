#pragma once

#include <Arduino.h>

struct StepMotorConfig
{
    uint8_t stepPin;
    uint8_t dirPin;
    uint8_t enablePin = 0xFF;
    bool enableActiveLow = true;
    unsigned int pulseWidthMicros = 1500;
    uint32_t stepsPerRevolution = 1000;
    float wheelDiameterMeters = 0.14f;
    bool forwardIncreasesDepth = true;
};

enum class StepMotorAction : uint8_t
{
    Move,
    StartContinuous,
    Stop,
    Zero,
};

enum class StepMotorCommandUnit : uint8_t
{
    Steps,
    Revolutions,
    Meters,
};

enum class StepMotorDirection : uint8_t
{
    Reverse = 0,
    Forward = 1,
};

struct StepMotorCommand
{
    StepMotorAction action = StepMotorAction::Move;
    StepMotorCommandUnit unit = StepMotorCommandUnit::Steps;
    float amount = 0.0f;
    StepMotorDirection direction = StepMotorDirection::Forward;
};

bool parseStepMotorCommand(const char *payload, StepMotorCommand &command);

class StepMotor
{
public:
    static constexpr uint8_t NoEnablePin = 0xFF;
    static constexpr float Pi = 3.14159265358979323846f;

    explicit StepMotor(const StepMotorConfig &config);

    void begin();
    void update(unsigned long nowMicros);
    void setEnabled(bool enabled);
    void setPulseWidthMicros(unsigned int pulseWidthMicros);
    uint32_t stepsPerRevolution() const;
    float wheelDiameterMeters() const;
    float wheelCircumferenceMeters() const;
    uint32_t metersToSteps(float distanceMeters) const;
    float currentPositionMeters() const;
    float currentDepthMeters() const;
    int32_t currentPositionSteps() const;
    bool isMoving() const;
    bool isContinuous() const;
    StepMotorDirection currentDirection() const;
    void moveSteps(uint32_t steps, bool direction);
    void moveRevolutions(float revolutions, bool direction);
    void moveMeters(float distanceMeters, bool direction);
    void startMoveSteps(uint32_t steps, StepMotorDirection direction, unsigned long nowMicros);
    void startMoveRevolutions(float revolutions, StepMotorDirection direction, unsigned long nowMicros);
    void startMoveMeters(float distanceMeters, StepMotorDirection direction, unsigned long nowMicros);
    void startContinuous(StepMotorDirection direction, unsigned long nowMicros);
    void stop();
    void zeroPosition();
    bool runCommand(const StepMotorCommand &command, unsigned long nowMicros);

private:
    void beginMove(StepMotorDirection direction, unsigned long nowMicros);

    StepMotorConfig config_;
    bool hasEnablePin_;
    bool moving_ = false;
    bool continuousMode_ = false;
    bool stepPinHigh_ = false;
    uint32_t remainingSteps_ = 0;
    int32_t currentPositionSteps_ = 0;
    unsigned long lastTransitionMicros_ = 0;
    StepMotorDirection currentDirection_ = StepMotorDirection::Forward;
};
