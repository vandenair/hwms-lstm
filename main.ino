#include "config.h"
#include "mqtt_handler.h"
#include "device_config.h"
#include "sensor.h"

void setup() {
    Serial.begin(115200);
    deviceConfigSetup();
    mqttHandlerSetup();
    sensorSetup();
}

void loop() {
    mqttHandlerLoop();
    sensorLoop();
}
