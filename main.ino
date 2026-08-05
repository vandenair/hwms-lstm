#include "config.h"
#include "mqtt_handler.h"
#include "device_config.h"
#include "sensor.h"
#include "actuator.h"
#include "rtc_time.h"
#include "history_buffer.h"
#include "offline_log.h"

void setup() {
    Serial.begin(115200);
    deviceConfigSetup();
    mqttHandlerSetup();
    rtcTimeSetup();
    sensorSetup();
    actuatorSetup();
    historyBufferSetup();
    offlineLogSetup();
}

void loop() {
    mqttHandlerLoop();
    rtcTimeLoop();
    sensorLoop();
    actuatorLoop();
    historyBufferLoop();
    offlineLogSyncIfConnected();
}
