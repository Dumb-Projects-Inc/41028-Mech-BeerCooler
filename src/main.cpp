#include <Arduino.h>
#include <BeerCoolerApp.h>

namespace
{
    constexpr uint8_t ONE_WIRE_BUS = 4;
    constexpr uint8_t STEP_PIN = 20;
    constexpr uint8_t DIR_PIN = 21;
    constexpr uint8_t ENABLE_PIN = 22;

    constexpr uint32_t STEPS_PER_REVOLUTION = 1000;
    constexpr float WHEEL_DIAMETER_METERS = 0.14f;
    constexpr unsigned int STEP_PULSE_WIDTH_US = 1500;

    constexpr unsigned long TEMPERATURE_PUBLISH_INTERVAL_MS = 2000;
    constexpr unsigned long STATUS_INTERVAL_MS = 5000;
    constexpr unsigned long MOTOR_DEMO_INTERVAL_MS = 15000;
    constexpr unsigned long MOTOR_STATE_PUBLISH_INTERVAL_MS = 500;
    constexpr unsigned long LOOP_DELAY_MS = 1;

    constexpr bool ALLOW_MISSING_TEMPERATURE_SENSOR = true;
    constexpr bool ENABLE_MOTOR_DEMO = false;
    constexpr StepMotorCommandUnit MOTOR_DEMO_UNIT = StepMotorCommandUnit::Meters;
    constexpr float MOTOR_DEMO_AMOUNT = 1.0f;

    constexpr char STARTUP_MESSAGE[] = "BeerCooler boot-up";

    constexpr char WIFI_PROOF_OF_POSSESSION[] = "abcd1234";
    constexpr char WIFI_SERVICE_NAME[] = "PROV_BeerCooler";
    constexpr bool WIFI_RESET_PROVISIONED = false;

    constexpr char MQTT_BROKER_URI[] = "mqtt://www.maqiatto.com:1883";
    constexpr char MQTT_USERNAME[] = "tyyinxoxerhanedhac@fxavaj.com";
    constexpr char MQTT_PASSWORD[] = "DetteErVoresKodeTilEt12TalsProjekt";
    constexpr char MQTT_TEMPERATURE_TOPIC[] = "tyyinxoxerhanedhac@fxavaj.com/temperature";
    constexpr char MQTT_COMMAND_TOPIC[] = "tyyinxoxerhanedhac@fxavaj.com/motor";
    constexpr char MQTT_MOTOR_STATE_TOPIC[] = "tyyinxoxerhanedhac@fxavaj.com/motor/state";

    const BeerCoolerAppConfig APP_CONFIG = {
        .connection =
            {
                .wifi =
                    {
                        .proofOfPossession = WIFI_PROOF_OF_POSSESSION,
                        .serviceName = WIFI_SERVICE_NAME,
                        .serviceKey = nullptr,
                        .resetProvisioned = WIFI_RESET_PROVISIONED,
                    },
                .mqtt =
                    {
                        .brokerUri = MQTT_BROKER_URI,
                        .username = MQTT_USERNAME,
                        .password = MQTT_PASSWORD,
                        .temperatureTopic = MQTT_TEMPERATURE_TOPIC,
                        .commandTopic = MQTT_COMMAND_TOPIC,
                        .motorStateTopic = MQTT_MOTOR_STATE_TOPIC,
                    },
            },
        .temperature =
            {
                .oneWirePin = ONE_WIRE_BUS,
                .publishIntervalMs = TEMPERATURE_PUBLISH_INTERVAL_MS,
                .sensorIndex = 0,
            },
        .motor =
            {
                .stepPin = STEP_PIN,
                .dirPin = DIR_PIN,
                .enablePin = ENABLE_PIN,
                .enableActiveLow = true,
                .pulseWidthMicros = STEP_PULSE_WIDTH_US,
                .stepsPerRevolution = STEPS_PER_REVOLUTION,
                .wheelDiameterMeters = WHEEL_DIAMETER_METERS,
                .forwardIncreasesDepth = true,
            },
        .debug =
            {
                .allowMissingTemperatureSensor = ALLOW_MISSING_TEMPERATURE_SENSOR,
                .enableMotorDemo = ENABLE_MOTOR_DEMO,
                .motorDemoUnit = MOTOR_DEMO_UNIT,
                .motorDemoAmount = MOTOR_DEMO_AMOUNT,
                .motorDemoIntervalMs = MOTOR_DEMO_INTERVAL_MS,
                .statusIntervalMs = STATUS_INTERVAL_MS,
                .motorStatePublishIntervalMs = MOTOR_STATE_PUBLISH_INTERVAL_MS,
            },
        .startupMessage = STARTUP_MESSAGE,
    };

    BeerCoolerApp app(APP_CONFIG);
}

void setup()
{
    Serial.begin(115200);

    if (!app.begin())
    {
        abort();
    }
}

void loop()
{
    app.update();
    delay(LOOP_DELAY_MS);
}
