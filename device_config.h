#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include <ArduinoJson.h>

void deviceConfigSetup();
void onDeviceConfigReceived(byte* payload, unsigned int length);
JsonDocument& getDeviceConfig();

#endif