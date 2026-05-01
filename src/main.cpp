#include <Arduino.h>
#include "prov.h"
#include <DS18B20.h>

#define TEMP_SENSOR_PIN 4

DS18B20 tempSensor(TEMP_SENSOR_PIN);

// put function declarations here:
int myFunction(int, int);

void setup()
{
  Serial.begin(115200);
  tempSensor.begin();
  tempSensor.setResolution(12);

  initWifi();
}

void loop()
{
  // put your main code here, to run repeatedly:
}

// put function definitions here:
int myFunction(int x, int y) { return x + y; }
