#include "ConnectionManager.h"

#include <cstdio>
#include <cstring>

namespace
{
uint8_t kProvisioningUuid[16] = {
    0xb4, 0xdf, 0x5a, 0x1c,
    0x3f, 0x6b, 0xf4, 0xbf,
    0xea, 0x4a, 0x82, 0x03,
    0x04, 0x90, 0x1a, 0x02,
};
}

ConnectionManager *ConnectionManager::activeInstance_ = nullptr;

ConnectionManager::ConnectionManager(const ConnectionManagerConfig &config)
    : config_(config)
{
}

void ConnectionManager::begin()
{
    activeInstance_ = this;
    WiFi.onEvent(handleWifiEvent);
    beginProvisioning();
    beginMqtt();
}

bool ConnectionManager::poll(ConnectionMessage &message, uint32_t timeoutMs)
{
    if (rxQueue_ == nullptr)
    {
        return false;
    }

    return xQueueReceive(rxQueue_, &message, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
}

bool ConnectionManager::publish(const char *topic, const char *payload, int qos, bool retained)
{
    if (client_ == nullptr)
    {
        return false;
    }

    return esp_mqtt_client_publish(client_, topic, payload, 0, qos, retained ? 1 : 0) >= 0;
}

bool ConnectionManager::publishTemperature(float temperatureC)
{
    char payload[80];
    snprintf(payload, sizeof(payload), "{\"temperature_c\":%.2f}", temperatureC);
    return publish(config_.mqtt.temperatureTopic, payload, 1, false);
}

bool ConnectionManager::publishMotorState(const char *payload)
{
    return publish(config_.mqtt.motorStateTopic, payload, 1, true);
}

const char *ConnectionManager::commandTopic() const
{
    return config_.mqtt.commandTopic;
}

const char *ConnectionManager::motorStateTopic() const
{
    return config_.mqtt.motorStateTopic;
}

bool ConnectionManager::isWifiConnected() const
{
    return wifiConnected_;
}

bool ConnectionManager::isMqttConnected() const
{
    return mqttConnected_;
}

void ConnectionManager::handleWifiEvent(arduino_event_t *event)
{
    if (activeInstance_ != nullptr)
    {
        activeInstance_->onWifiEvent(event);
    }
}

void ConnectionManager::handleMqttEvent(void *handlerArgs, esp_event_base_t, int32_t, void *eventData)
{
    ConnectionManager *instance = static_cast<ConnectionManager *>(handlerArgs);
    if (instance != nullptr)
    {
        instance->onMqttEvent(static_cast<esp_mqtt_event_handle_t>(eventData));
    }
}

bool ConnectionManager::safeCopy(char *destination, size_t destinationSize, const char *source, size_t sourceSize)
{
    if (destination == nullptr || source == nullptr || destinationSize == 0)
    {
        return false;
    }

    size_t copyLength = sourceSize;
    if (copyLength >= destinationSize)
    {
        copyLength = destinationSize - 1;
    }

    memcpy(destination, source, copyLength);
    destination[copyLength] = '\0';
    return copyLength == sourceSize;
}

void ConnectionManager::onWifiEvent(arduino_event_t *event)
{
    switch (event->event_id)
    {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        wifiConnected_ = true;
        Serial.print("\nConnected IP address: ");
        Serial.println(IPAddress(event->event_info.got_ip.ip_info.ip.addr));
        break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        wifiConnected_ = false;
        Serial.println("\nDisconnected. Connecting to the AP again...");
        break;
    case ARDUINO_EVENT_PROV_START:
        Serial.println("\nProvisioning started");
        Serial.println("Give credentials using the Espressif mobile app");
        break;
    case ARDUINO_EVENT_PROV_CRED_RECV:
        Serial.println("\nReceived Wi-Fi credentials");
        Serial.print("SSID: ");
        Serial.println(reinterpret_cast<const char *>(event->event_info.prov_cred_recv.ssid));
        Serial.print("Password: ");
        Serial.println(reinterpret_cast<const char *>(event->event_info.prov_cred_recv.password));
        break;
    case ARDUINO_EVENT_PROV_CRED_FAIL:
        Serial.println("\nProvisioning failed");
        if (event->event_info.prov_fail_reason == NETWORK_PROV_WIFI_STA_AUTH_ERROR)
        {
            Serial.println("Wi-Fi AP password incorrect");
        }
        else
        {
            Serial.println("Wi-Fi AP not found");
        }
        break;
    case ARDUINO_EVENT_PROV_CRED_SUCCESS:
        Serial.println("\nProvisioning successful");
        break;
    case ARDUINO_EVENT_PROV_END:
        Serial.println("\nProvisioning ended");
        break;
    default:
        break;
    }
}

void ConnectionManager::onMqttEvent(esp_mqtt_event_handle_t event)
{
    switch (event->event_id)
    {
    case MQTT_EVENT_CONNECTED:
    {
        mqttConnected_ = true;
        const int messageId = esp_mqtt_client_subscribe(event->client, config_.mqtt.commandTopic, 0);
        Serial.print("Subscribed to: ");
        Serial.print(config_.mqtt.commandTopic);
        Serial.print(" msg_id=");
        Serial.println(messageId);
        break;
    }
    case MQTT_EVENT_DISCONNECTED:
        mqttConnected_ = false;
        break;
    case MQTT_EVENT_DATA:
    {
        ConnectionMessage message = {};
        safeCopy(message.topic, sizeof(message.topic), event->topic, event->topic_len);
        safeCopy(message.payload, sizeof(message.payload), event->data, event->data_len);

        if (rxQueue_ != nullptr)
        {
            xQueueSend(rxQueue_, &message, 0);
        }
        break;
    }
    case MQTT_EVENT_ERROR:
        Serial.println("MQTT error");
        break;
    default:
        break;
    }
}

void ConnectionManager::beginProvisioning()
{
    WiFi.mode(WIFI_STA);

    Serial.println("Begin provisioning using BLE");
    WiFiProv.beginProvision(
        NETWORK_PROV_SCHEME_BLE,
        NETWORK_PROV_SCHEME_HANDLER_FREE_BTDM,
        NETWORK_PROV_SECURITY_1,
        config_.wifi.proofOfPossession,
        config_.wifi.serviceName,
        config_.wifi.serviceKey,
        kProvisioningUuid,
        config_.wifi.resetProvisioned);

    WiFiProv.printQR(config_.wifi.serviceName, config_.wifi.proofOfPossession, "ble");
}

void ConnectionManager::beginMqtt()
{
    if (!ensureQueue() || client_ != nullptr)
    {
        return;
    }

    esp_mqtt_client_config_t mqttConfig = {};
    mqttConfig.broker.address.uri = config_.mqtt.brokerUri;
    mqttConfig.credentials.username = config_.mqtt.username;
    mqttConfig.credentials.authentication.password = config_.mqtt.password;

    client_ = esp_mqtt_client_init(&mqttConfig);
    esp_mqtt_client_register_event(client_, MQTT_EVENT_ANY, handleMqttEvent, this);
    esp_mqtt_client_start(client_);
}

bool ConnectionManager::ensureQueue()
{
    if (rxQueue_ == nullptr)
    {
        rxQueue_ = xQueueCreate(MessageQueueLength, sizeof(ConnectionMessage));
    }

    return rxQueue_ != nullptr;
}
