#include <Arduino.h>
#include "prov.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include "mqtt.h"
#include "stepper.h"
#include <cstdlib>
#include <strings.h>

#define ONE_WIRE_BUS 4

static constexpr uint8_t PWM_PIN = 10;
static constexpr uint8_t PWM_DUTY_ACTIVE = 255;
static constexpr uint8_t PWM_DUTY_IDLE = 0;
static constexpr unsigned long TEMPERATURE_PUBLISH_INTERVAL_MS = 2000;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

static unsigned long lastTemperaturePublishMs = 0;
static uint8_t currentMotorDuty = PWM_DUTY_IDLE;

static void setMotorDuty(uint8_t duty)
{
  if (duty == currentMotorDuty)
  {
    return;
  }

  currentMotorDuty = duty;
  analogWrite(PWM_PIN, currentMotorDuty);
  Serial.print("Motor PWM duty set to ");
  Serial.println(currentMotorDuty);
}

static void runMotor(uint8_t pin, int speed, int durationMs)
{
  if (speed < 0 || speed > 255)
  {
    Serial.println("Invalid speed value. Must be between 0 and 255.");
    return;
  }

  if (durationMs < 0)
  {
    Serial.println("Invalid duration value. Must be 0 or greater.");
    return;
  }

  if (pin != PWM_PIN)
  {
    Serial.println("Ignoring motor command for unexpected pin.");
    return;
  }

  Serial.println("Running motor...");
  setMotorDuty(static_cast<uint8_t>(speed));

  if (durationMs > 0)
  {
    delay(durationMs);
    setMotorDuty(PWM_DUTY_IDLE);
    Serial.println("Motor stopped.");
  }
}

static bool parseMotorCommand(const char *payload, int &speed, int &durationMs)
{
  // Expected payload formats:
  // - "SPEED"          -> numeric speed 0-255, doesnt stop
  // - "SPEED,DURATION" -> numeric speed 0-255, duration in milliseconds

  if (payload == nullptr || payload[0] == '\0')
  {
    return false;
  }

  int parsedSpeed = 0;
  int parsedDuration = 0;
  int parsedCount = sscanf(payload, "%d,%d", &parsedSpeed, &parsedDuration);
  if (parsedCount < 1)
  {
    return false;
  }

  if (parsedCount == 1)
  {
    parsedDuration = 0;
  }

  speed = parsedSpeed;
  durationMs = parsedDuration;
  return true;
}

static bool parseStepperCommand(const char *payload, int &steps, int &speed)
{
  // Expected payload formats:
  // - "STEPS"           -> move by steps, default speed
  // - "STEPS,SPEED"     -> move by steps with custom speed (steps/sec)
  // - Direction: negative steps = reverse

  if (payload == nullptr || payload[0] == '\0')
  {
    return false;
  }

  int parsedSteps = 0;
  int parsedSpeed = 0;
  int parsedCount = sscanf(payload, "%d,%d", &parsedSteps, &parsedSpeed);
  if (parsedCount < 1)
  {
    return false;
  }

  if (parsedCount == 1)
  {
    parsedSpeed = 500;  // Default speed
  }

  steps = parsedSteps;
  speed = parsedSpeed;
  return true;
}

static void handleStepperCommand(const char *payload)
{
  Serial.print("MQTT stepper command: ");
  Serial.println(payload);

  int steps = 0;
  int speed = 0;
  if (!parseStepperCommand(payload, steps, speed))
  {
    Serial.println("Unrecognized stepper command.");
    return;
  }

  // Set speed first
  Stepper::setSpeed(speed);

  // Then move
  Stepper::moveSteps(steps);
}

static void handleMqttMessage(const MQTT::Message &message)
{
  if (strcmp(message.topic, MQTT::TOPIC_COMMAND) == 0)
  {
    Serial.print("MQTT motor command: ");
    Serial.println(message.payload);

    int speed = 0;
    int durationMs = 0;
    if (!parseMotorCommand(message.payload, speed, durationMs))
    {
      Serial.println("Unrecognized motor command.");
      return;
    }

    runMotor(PWM_PIN, speed, durationMs);
  }
  else if (strcmp(message.topic, MQTT::TOPIC_STEPPER_COMMAND) == 0)
  {
    handleStepperCommand(message.payload);
  }
  else
  {
    Serial.println("Received MQTT message for unrecognized topic:");
    Serial.print("Topic: ");    Serial.println(message.topic);
    Serial.print("Payload: ");  Serial.println(message.payload);
  }
}

static void pollMqttCommands()
{
  MQTT::Message message = {};
  while (MQTT::poll(message, 0))
  {
    handleMqttMessage(message);
  }
}

static void publishTemperatureIfReady(float tempC)
{
  if (tempC == DEVICE_DISCONNECTED_C)
  {
    Serial.println("Error: Could not read temperature data");
    return;
  }

  Serial.print("Temperature: ");
  Serial.print(tempC);
  Serial.println(" °C");

  if (MQTT::publishTemperature(tempC))
  {
    Serial.println("Temperature published to MQTT");
  }
  else
  {
    Serial.println("MQTT publish skipped or failed");
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Hello world!");

  pinMode(PWM_PIN, OUTPUT);
  setMotorDuty(PWM_DUTY_IDLE);

  Stepper::init();

  sensors.begin();

  int deviceCount = sensors.getDeviceCount();
  Serial.print("Found ");
  Serial.print(deviceCount);
  Serial.println(" devices.");

  if (deviceCount <= 0)
  {
    Serial.println("Fatal: No temperature sensor found. Aborting setup.");
    abort();
  }

  initWifi();
  MQTT::init();
}

void loop()
{
  pollMqttCommands();

  // Process stepper motor stepping
  Stepper::run();

  unsigned long now = millis();
  if (now - lastTemperaturePublishMs >= TEMPERATURE_PUBLISH_INTERVAL_MS)
  {
    lastTemperaturePublishMs = now;

    Serial.print("Requesting temperatures...");
    sensors.requestTemperatures();
    Serial.println("DONE");

    float tempC = sensors.getTempCByIndex(0);
    publishTemperatureIfReady(tempC);
  }

  delay(10);
}