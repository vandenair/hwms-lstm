#include "actuator.h"
#include "config.h"
#include <ArduinoJson.h>

#define RELAY_ACTIVE_LOW true
#define MAX_ACTUATOR_DURATION_MS 300000UL
#define MAX_RELAYS 4

struct RelayState {
    int pin;
    bool active;
    unsigned long offAtMs;
};

static RelayState relays[MAX_RELAYS];
static int relayCount = 0;

static void relayWrite(int pin, bool on) {
    digitalWrite(pin, RELAY_ACTIVE_LOW ? !on : on);
}

static void registerRelay(int pin) {
    relays[relayCount].pin = pin;
    relays[relayCount].active = false;
    relays[relayCount].offAtMs = 0;
    relayCount++;
    pinMode(pin, OUTPUT);
    relayWrite(pin, false);
}

static RelayState* findRelay(int pin) {
    for (int i = 0; i < relayCount; i++) {
        if (relays[i].pin == pin) return &relays[i];
    }
    return nullptr;
}

static void activateRelay(int pin, unsigned long durationMs) {
    if (durationMs > MAX_ACTUATOR_DURATION_MS) durationMs = MAX_ACTUATOR_DURATION_MS;
    RelayState* r = findRelay(pin);
    if (r == nullptr) return;
    relayWrite(pin, true);
    r->active = true;
    r->offAtMs = millis() + durationMs;
}

static int resolveRelayPin(const char* target) {
#if defined(DEVICE_HWMS)
    if (strcmp(target, "pump_rwh") == 0) return PIN_RELAY_PUMP_RWH;
    if (strcmp(target, "pump_groundwater") == 0) return PIN_RELAY_PUMP_GROUNDWATER;
    if (strcmp(target, "solenoid_1") == 0) return PIN_RELAY_SOLENOID_1;
#if defined(HWMS_DUAL_SOLENOID_ENABLED)
    if (strcmp(target, "solenoid_2") == 0) return PIN_RELAY_SOLENOID_2;
#endif
#endif
#if defined(DEVICE_GS)
    if (strcmp(target, "pump_nozzle") == 0) return PIN_RELAY_PUMP_NOZZLE;
#endif
    return -1;
}

void actuatorSetup() {
#if defined(DEVICE_HWMS)
    registerRelay(PIN_RELAY_PUMP_RWH);
    registerRelay(PIN_RELAY_PUMP_GROUNDWATER);
    registerRelay(PIN_RELAY_SOLENOID_1);
#if defined(HWMS_DUAL_SOLENOID_ENABLED)
    registerRelay(PIN_RELAY_SOLENOID_2);
#endif
#endif
#if defined(DEVICE_GS)
    registerRelay(PIN_RELAY_PUMP_NOZZLE);
#endif
}

void actuatorLoop() {
    unsigned long now = millis();
    for (int i = 0; i < relayCount; i++) {
        if (relays[i].active && now >= relays[i].offAtMs) {
            relayWrite(relays[i].pin, false);
            relays[i].active = false;
        }
    }
}

void actuatorHandleCommand(byte* payload, unsigned int length) {
    JsonDocument doc;
    String json;
    json.reserve(length);
    for (unsigned int i = 0; i < length; i++) json += (char)payload[i];

    if (deserializeJson(doc, json)) {
        Serial.println("Command invalid");
        return;
    }

    const char* action = doc["action"] | "";
    const char* target = doc["target"] | "";
    long durationSec = doc["duration_seconds"] | 0;

    int pin = resolveRelayPin(target);
    if (pin < 0) {
        Serial.println("Target relay tidak dikenali");
        return;
    }

    if (strcmp(action, "activate") == 0) {
        activateRelay(pin, (unsigned long)durationSec * 1000UL);
        Serial.println("Relay ON: " + String(target));
    } else if (strcmp(action, "deactivate") == 0) {
        RelayState* r = findRelay(pin);
        if (r != nullptr) {
            relayWrite(pin, false);
            r->active = false;
        }
    }
}