#include <DS18B20.h>

#define TEMP_SENSOR_PIN 4

DS18B20 tempSensor(TEMP_SENSOR_PIN);

void setup()
{
    Serial.begin(115200);

    tempSensor.begin();
    tempSensor.setResolution(12);

    Serial.print("Sensors found: ");
    Serial.println(tempSensor.getDeviceCount());
}

void loop()
{
    float tempC = tempSensor.readCelsius();

    if (isnan(tempC))
    {
        Serial.println("Sensor not connected or read failed");
    }
    else
    {
        Serial.print("Temperature: ");
        Serial.print(tempC);
        Serial.println(" °C");
    }

    delay(1000);
}