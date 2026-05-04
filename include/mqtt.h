#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <mqtt_client.h>

namespace MQTT
{
    static constexpr const char *BROKER_URI = "mqtt://www.maqiatto.com:1883";
    static constexpr const char *USERNAME = "tyyinxoxerhanedhac@fxavaj.com";
    static constexpr const char *PASSWORD = "DetteErVoresKodeTilEt12TalsProjekt";
    static constexpr const char *TOPIC_MOISTURE = "tyyinxoxerhanedhac@fxavaj.com/moisture";
    static constexpr const char *TOPIC_TEMPERATURE = "tyyinxoxerhanedhac@fxavaj.com/temperature";
    static constexpr const char *TOPIC_COMMAND = "tyyinxoxerhanedhac@fxavaj.com/fan";

    static constexpr size_t MESSAGE_TOPIC_SIZE = 96;
    static constexpr size_t MESSAGE_PAYLOAD_SIZE = 128;
    static constexpr size_t MESSAGE_QUEUE_LENGTH = 8;

    struct Message
    {
        char topic[MESSAGE_TOPIC_SIZE];
        char payload[MESSAGE_PAYLOAD_SIZE];
    };

    static esp_mqtt_client_handle_t &clientHandle()
    {
        static esp_mqtt_client_handle_t client = nullptr;
        return client;
    }

    static QueueHandle_t &rxQueue()
    {
        static QueueHandle_t queue = nullptr;
        return queue;
    }

    static bool &mqttConnected()
    {
        static bool connected = false; // just so we can return a pointer lol
        return connected;
    }

    static bool safeCopy(char *destination, size_t destinationSize, const char *source, size_t sourceSize)
    {
        if (destinationSize == 0)
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

    static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
    {
        esp_mqtt_client_handle_t client = event->client;

        switch (event->event_id)
        {
        case MQTT_EVENT_CONNECTED:
            mqttConnected() = true; // overwrite the
            esp_mqtt_client_subscribe(client, TOPIC_COMMAND, 0);
            break;
        case MQTT_EVENT_DISCONNECTED:
            mqttConnected() = false;
            break;
        case MQTT_EVENT_DATA:
        {
            Message message = {};
            safeCopy(message.topic, sizeof(message.topic), event->topic, event->topic_len);
            safeCopy(message.payload, sizeof(message.payload), event->data, event->data_len);

            if (rxQueue() != nullptr)
            {
                xQueueSend(rxQueue(), &message, 0);
            }
            break;
        }
        case MQTT_EVENT_ERROR:
            break;
        default:
            break;
        }
        return ESP_OK;
    }

    static void event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
    {
        mqtt_event_handler_cb((esp_mqtt_event_handle_t)event_data);
    }

    static void init(void)
    {
        if (rxQueue() == nullptr)
        {
            rxQueue() = xQueueCreate(MESSAGE_QUEUE_LENGTH, sizeof(Message)); // FreeRTOS queue because normal arduino loops are not good for waiting on mqtt messages
        }

        if (clientHandle() != nullptr)
        {
            return;
        }

        esp_mqtt_client_config_t mqtt_cfg = {
            .broker = {
                .address = {
                    .uri = BROKER_URI,
                }
            },
            .credentials = {
                .username = USERNAME,
                .authentication = {
                    .password = PASSWORD,
                }
            }
        };

        clientHandle() = esp_mqtt_client_init(&mqtt_cfg);
        esp_mqtt_client_register_event(clientHandle(), MQTT_EVENT_ANY, event_handler, clientHandle());
        esp_mqtt_client_start(clientHandle());
    }

    static bool isConnected()
    {
        return mqttConnected();
    }

    static bool subscribe(const char *topic, int qos = 0)
    {
        if (clientHandle() == nullptr)
        {
            return false;
        }

        return esp_mqtt_client_subscribe(clientHandle(), topic, qos) >= 0;
    }

    static bool publish(const char *topic, const char *payload, int qos = 1, bool retained = false)
    {
        if (clientHandle() == nullptr)
        {
            return false;
        }

        return esp_mqtt_client_publish(clientHandle(), topic, payload, 0, qos, retained ? 1 : 0) >= 0;
    }

    static bool publishMoisture(int moisturePercent, int rawValue)
    {
        char payload[80];
        snprintf(payload, sizeof(payload), "{\"moisture\":%d,\"raw\":%d}", moisturePercent, rawValue);
        return publish(TOPIC_MOISTURE, payload, 1, false);
    }

    static bool publishTemperature(float temperatureC)
    {
        char payload[80];
        snprintf(payload, sizeof(payload), "{\"temperature_c\":%.2f}", temperatureC);
        return publish(TOPIC_TEMPERATURE, payload, 1, false);
    }

    static bool poll(Message &message, uint32_t timeoutMs = 0)
    {
        if (rxQueue() == nullptr)
        {
            return false;
        }

        return xQueueReceive(rxQueue(), &message, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
    }
}