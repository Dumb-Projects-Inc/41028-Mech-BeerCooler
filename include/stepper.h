#pragma once

#include <Arduino.h>
#include <AccelStepper.h>

namespace Stepper
{
    // L298N Motor Controller GPIO pins (interchangeable - modify here as needed)
    static constexpr uint8_t IN1_PIN = 14;
    static constexpr uint8_t IN2_PIN = 15;
    static constexpr uint8_t IN3_PIN = 32;
    static constexpr uint8_t IN4_PIN = 33;

    // Stepper motor parameters
    static constexpr float MAX_SPEED = 1000.0;        // steps per second
    static constexpr float ACCELERATION = 500.0;      // steps per second squared
    static constexpr int STEPS_PER_REVOLUTION = 200;  // NEMA 17 typical value

    // Initialize stepper motor pins and AccelStepper object
    static AccelStepper &stepperMotor()
    {
        static AccelStepper motor(AccelStepper::FULL4WIRE, IN1_PIN, IN3_PIN, IN2_PIN, IN4_PIN);
        return motor;
    }

    // Initialize stepper (call from setup())
    static void init()
    {
        pinMode(IN1_PIN, OUTPUT);
        pinMode(IN2_PIN, OUTPUT);
        pinMode(IN3_PIN, OUTPUT);
        pinMode(IN4_PIN, OUTPUT);

        stepperMotor().setMaxSpeed(MAX_SPEED);
        stepperMotor().setAcceleration(ACCELERATION);

        Serial.println("Stepper motor initialized on pins: IN1=" + String(IN1_PIN) + 
                       ", IN2=" + String(IN2_PIN) + 
                       ", IN3=" + String(IN3_PIN) + 
                       ", IN4=" + String(IN4_PIN));
    }

    // Move stepper by specified number of steps
    static void moveSteps(long steps)
    {
        stepperMotor().moveTo(stepperMotor().currentPosition() + steps);
        Serial.print("Stepper moving ");
        Serial.print(steps);
        Serial.println(" steps");
    }

    // Move stepper by specified distance in millimeters (assumes 1.8° per step, 200 steps/rev for NEMA 17)
    // Distance in mm, assuming typical mechanical configuration
    static void moveDistance(float distanceMm)
    {
        // Convert mm to steps (adjust factor based on your mechanical setup)
        // Example: 5mm per revolution = 1mm per 40 steps
        // Adjust STEPS_PER_MM based on your gear ratio and lead screw pitch
        static constexpr float STEPS_PER_MM = 40.0;  // Adjust this value for your system
        long steps = (long)(distanceMm * STEPS_PER_MM);
        moveSteps(steps);
    }

    // Set stepper speed in steps per second
    static void setSpeed(float stepsPerSecond)
    {
        if (stepsPerSecond < 0 || stepsPerSecond > MAX_SPEED)
        {
            Serial.print("Invalid speed: ");
            Serial.print(stepsPerSecond);
            Serial.println(" (must be 0-" + String(MAX_SPEED) + ")");
            return;
        }
        stepperMotor().setMaxSpeed(stepsPerSecond);
        Serial.print("Stepper speed set to ");
        Serial.print(stepsPerSecond);
        Serial.println(" steps/sec");
    }

    // Run stepper (call from main loop to process stepping)
    static void run()
    {
        stepperMotor().run();
    }

    // Get current position
    static long getCurrentPosition()
    {
        return stepperMotor().currentPosition();
    }

    // Stop stepper immediately
    static void stop()
    {
        stepperMotor().stop();
        Serial.println("Stepper stopped");
    }

    // Set current position as home (zero)
    static void setHome()
    {
        stepperMotor().setCurrentPosition(0);
        Serial.println("Stepper home position set");
    }
}
