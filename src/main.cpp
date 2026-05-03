#include <Arduino.h>
#include "prov.h"
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 4

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// put function declarations here:
int myFunction(int, int);

void setup()
{
  Serial.begin(115200);
  Serial.println("Hello world!");

  initWifi();
}

void loop()
{
  // put your main code here, to run repeatedly:
}

// put function definitions here:
int myFunction(int x, int y) { return x + y; }
