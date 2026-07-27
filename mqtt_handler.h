#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>

void mqttHandlerSetup();
void mqttHandlerLoop();
bool mqttIsConnected();
void mqttPublishSensorData(const String& jsonPayload);

#endif