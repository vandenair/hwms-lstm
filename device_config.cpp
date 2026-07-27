#include "device_config.h"
#include <Preferences.h>

static Preferences prefs;
static JsonDocument configDoc;

static const char* PREF_NAMESPACE = "device_cfg";
static const char* PREF_KEY = "cfg_json";

void deviceConfigSetup() {
    prefs.begin(PREF_NAMESPACE, false);
    String saved = prefs.getString(PREF_KEY, "{}");
    if (deserializeJson(configDoc, saved)) {
        configDoc.clear();
    }
}

void onDeviceConfigReceived(byte* payload, unsigned int length) {
    String json;
    json.reserve(length);
    for (unsigned int i = 0; i < length; i++) json += (char)payload[i];

    JsonDocument temp;
    if (deserializeJson(temp, json)) {
        Serial.println("Config invalid, diabaikan");
        return;
    }

    configDoc = temp;
    prefs.putString(PREF_KEY, json);
    Serial.println("Config tersimpan");
}

JsonDocument& getDeviceConfig() {
    return configDoc;
}