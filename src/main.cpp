#include <Arduino.h>
#include "prov.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include "mqtt.h"
#include <cstdlib>
#include <strings.h>
#include "stepper.h"

#define ONE_WIRE_BUS 4

static constexpr uint8_t PWM_PIN = 10;
static constexpr unsigned long TEMPERATURE_PUBLISH_INTERVAL_MS = 2000;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

static unsigned long lastTemperaturePublishMs = 0;
static bool parseDistanceCommand(const char *payload, float &distanceMm)
{
  // Expected payload format:
  // - "DISTANCE" -> numeric distance in millimeters; negative values move backward

  if (payload == nullptr || payload[0] == '\0')
  {
    return false;
  }

  float parsedDistance = 0.0f;
  int parsedCount = sscanf(payload, "%f", &parsedDistance);
  if (parsedCount != 1)
  {
    return false;
  }

  distanceMm = parsedDistance;
  return true;
}

static void handleMqttMessage(const MQTT::Message &message)
{
  if (strcmp(message.topic, MQTT::TOPIC_COMMAND) != 0)
  {
    Serial.println("Received MQTT message for unrecognized topic:");
    Serial.print("Topic: ");    Serial.println(message.topic);
    Serial.print("Payload: ");  Serial.println(message.payload);
    return;
  }

  Serial.print("MQTT stepper command: ");
  Serial.println(message.payload);

  float distanceMm = 0.0f;
  if (!parseDistanceCommand(message.payload, distanceMm))
  {
    Serial.println("Unrecognized stepper command.");
    return;
  }

  Stepper::moveDistance(distanceMm);
  Serial.print("Queued stepper move: ");
  Serial.print(distanceMm);
  Serial.println(" mm");
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
  Stepper::init();
  Stepper::moveDistance(10.0f);
}

void loop()
{
  pollMqttCommands();
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