#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_GPIO 2

OneWire oneWire(ONE_WIRE_GPIO);
DallasTemperature sensors(&oneWire);

// put function declarations here:
int myFunction(int, int);

void setup()
{
  Serial.begin(115200);
  sensors.begin();

  Serial.print("Found sensors: ");
  Serial.println(sensors.getDeviceCount());
}

void loop()
{
  // put your main code here, to run repeatedly:
}

// put function definitions here:
int myFunction(int x, int y)
{
  return x + y;
}