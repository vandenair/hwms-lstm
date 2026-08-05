// #include "mqtt_handler.h"
// #include "config.h"

// #define MQTT_MAX_PACKET_SIZE 512
// #include <WiFi.h>
// #include <PubSubClient.h>

// // Diimplementasikan di device_config.cpp
// extern void onDeviceConfigReceived(byte* payload, unsigned int length);

// static WiFiClient wifiClient;
// static PubSubClient mqttClient(wifiClient);

// static unsigned long lastHeartbeatMs = 0;
// static unsigned long lastReconnectAttemptMs = 0;

// static void onMqttMessage(char* topic, byte* payload, unsigned int length) {
//     if (strcmp(topic, TOPIC_DEVICE_CONFIG) == 0) {
//         onDeviceConfigReceived(payload, length);
//     }
// }

// static void connectWiFi() {
//     if (WiFi.status() == WL_CONNECTED) return;
//     Serial.println("WiFi disconnected, reconnecting...");
//     WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
//     while (WiFi.status() != WL_CONNECTED) {
//         delay(300);
//         Serial.print(".");
//     }
//     WiFi.setSleep(false);
//     Serial.println(" OK " + WiFi.localIP().toString());
// }

// static bool connectMqtt() {
//     bool ok = mqttClient.connect(
//         DEVICE_ID, MQTT_USERNAME, MQTT_PASSWORD,
//         TOPIC_DEVICE_STATUS, 1, true, "{\"status\":\"offline\"}"
//     );

//     if (ok) {
//         mqttClient.publish(TOPIC_DEVICE_STATUS, "{\"status\":\"online\"}", true);
//         mqttClient.subscribe(TOPIC_DEVICE_CONFIG);
//         Serial.println("MQTT connected");
//     } else {
//         Serial.print("MQTT failed rc=");
//         Serial.println(mqttClient.state());
//     }
//     return ok;
// }

// void mqttHandlerSetup() {
//     connectWiFi();
//     mqttClient.setServer(MQTT_HOST, MQTT_PORT);
//     mqttClient.setCallback(onMqttMessage);
//     mqttClient.setKeepAlive(60);
//     connectMqtt();
// }

// void mqttHandlerLoop() {
//     if (WiFi.status() != WL_CONNECTED) {
//         connectWiFi();
//     }

//     if (!mqttClient.connected()) {
//         unsigned long now = millis();
//         if (now - lastReconnectAttemptMs >= MQTT_RECONNECT_INTERVAL_MS) {
//             lastReconnectAttemptMs = now;
//             connectMqtt();
//         }
//         return;
//     }

//     mqttClient.loop();

//     unsigned long now = millis();
//     if (now - lastHeartbeatMs >= HEARTBEAT_INTERVAL_MS) {
//         lastHeartbeatMs = now;
//         bool sent = mqttClient.publish(TOPIC_DEVICE_HEARTBEAT, "{}");
//         Serial.println(sent ? "heartbeat OK" : "heartbeat FAILED");
//     }
// }

// bool mqttIsConnected() {
//     return mqttClient.connected();
// }

// void mqttPublishSensorData(const String& jsonPayload) {
//     if (mqttClient.connected()) {
//         mqttClient.publish(TOPIC_SENSOR_DATA, jsonPayload.c_str());
//     }
// }
#include "mqtt_handler.h"
#include "config.h"

#define MQTT_MAX_PACKET_SIZE 512
#include <WiFi.h>
#include <PubSubClient.h>

// Diimplementasikan di device_config.cpp
extern void onDeviceConfigReceived(byte* payload, unsigned int length);
// Diimplementasikan di actuator.cpp
extern void actuatorHandleCommand(byte* payload, unsigned int length);

static WiFiClient wifiClient;
static PubSubClient mqttClient(wifiClient);

static unsigned long lastHeartbeatMs = 0;
static unsigned long lastReconnectAttemptMs = 0;

static void onMqttMessage(char* topic, byte* payload, unsigned int length) {
    if (strcmp(topic, TOPIC_DEVICE_CONFIG) == 0) {
        onDeviceConfigReceived(payload, length);
    } else if (strcmp(topic, TOPIC_DEVICE_COMMAND) == 0) {
        actuatorHandleCommand(payload, length);
    }
}

static void connectWiFi() {
    if (WiFi.status() == WL_CONNECTED) return;
    Serial.println("WiFi disconnected, reconnecting...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(300);
        Serial.print(".");
    }
    WiFi.setSleep(false);
    Serial.println(" OK " + WiFi.localIP().toString());
}

static bool connectMqtt() {
    bool ok = mqttClient.connect(
        DEVICE_ID, MQTT_USERNAME, MQTT_PASSWORD,
        TOPIC_DEVICE_STATUS, 1, true, "{\"status\":\"offline\"}"
    );

    if (ok) {
        mqttClient.publish(TOPIC_DEVICE_STATUS, "{\"status\":\"online\"}", true);
        mqttClient.subscribe(TOPIC_DEVICE_CONFIG);
        mqttClient.subscribe(TOPIC_DEVICE_COMMAND);
        Serial.println("MQTT connected");
    } else {
        Serial.print("MQTT failed rc=");
        Serial.println(mqttClient.state());
    }
    return ok;
}

void mqttHandlerSetup() {
    connectWiFi();
    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
    mqttClient.setCallback(onMqttMessage);
    mqttClient.setKeepAlive(60);
    connectMqtt();
}

void mqttHandlerLoop() {
    if (WiFi.status() != WL_CONNECTED) {
        connectWiFi();
    }

    if (!mqttClient.connected()) {
        unsigned long now = millis();
        if (now - lastReconnectAttemptMs >= MQTT_RECONNECT_INTERVAL_MS) {
            lastReconnectAttemptMs = now;
            connectMqtt();
        }
        return;
    }

    mqttClient.loop();

    unsigned long now = millis();
    if (now - lastHeartbeatMs >= HEARTBEAT_INTERVAL_MS) {
        lastHeartbeatMs = now;
        bool sent = mqttClient.publish(TOPIC_DEVICE_HEARTBEAT, "{}");
        Serial.println(sent ? "heartbeat OK" : "heartbeat FAILED");
    }
}

bool mqttIsConnected() {
    return mqttClient.connected();
}

void mqttPublishSensorData(const String& jsonPayload) {
    if (mqttClient.connected()) {
        mqttClient.publish(TOPIC_SENSOR_DATA, jsonPayload.c_str());
    }
}