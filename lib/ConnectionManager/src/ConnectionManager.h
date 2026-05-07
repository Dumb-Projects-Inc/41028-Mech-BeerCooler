#pragma once

#include <Arduino.h>
#include <Network.h>
#include <WiFi.h>
#include <WiFiProv.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <mqtt_client.h>

struct WifiProvisioningConfig
{
    const char *proofOfPossession;
    const char *serviceName;
    const char *serviceKey = nullptr;
    bool resetProvisioned = true;
};

struct MqttConnectionConfig
{
    const char *brokerUri;
    const char *username;
    const char *password;
    const char *temperatureTopic;
    const char *commandTopic;
    const char *motorStateTopic;
};

struct ConnectionManagerConfig
{
    WifiProvisioningConfig wifi;
    MqttConnectionConfig mqtt;
};

struct ConnectionMessage
{
    static constexpr size_t TopicSize = 96;
    static constexpr size_t PayloadSize = 128;

    char topic[TopicSize] = {};
    char payload[PayloadSize] = {};
};

class ConnectionManager
{
public:
    explicit ConnectionManager(const ConnectionManagerConfig &config);

    void begin();
    bool poll(ConnectionMessage &message, uint32_t timeoutMs = 0);
    bool publish(const char *topic, const char *payload, int qos = 1, bool retained = false);
    bool publishTemperature(float temperatureC);
    bool publishMotorState(const char *payload);

    const char *commandTopic() const;
    const char *motorStateTopic() const;
    bool isWifiConnected() const;
    bool isMqttConnected() const;

private:
    static constexpr size_t MessageQueueLength = 8;

    static ConnectionManager *activeInstance_;

    static void handleWifiEvent(arduino_event_t *event);
    static void handleMqttEvent(void *handlerArgs, esp_event_base_t base, int32_t eventId, void *eventData);
    static bool safeCopy(char *destination, size_t destinationSize, const char *source, size_t sourceSize);

    void onWifiEvent(arduino_event_t *event);
    void onMqttEvent(esp_mqtt_event_handle_t event);
    void beginProvisioning();
    void beginMqtt();
    bool ensureQueue();

    ConnectionManagerConfig config_;
    QueueHandle_t rxQueue_ = nullptr;
    esp_mqtt_client_handle_t client_ = nullptr;
    bool wifiConnected_ = false;
    bool mqttConnected_ = false;
};
