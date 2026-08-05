#include "offline_log.h"
#include "rtc_time.h"
#include "mqtt_handler.h"
#include <Preferences.h>

#define OFFLINE_LOG_SIZE 72
#define PAYLOAD_MAX_LEN 128

struct OfflineLogEntry {
    unsigned long epoch;
    char payload[PAYLOAD_MAX_LEN];
};

static Preferences prefs;
static OfflineLogEntry logBuf[OFFLINE_LOG_SIZE];
static int headIndex = 0;
static int count = 0;
static unsigned long lastLogEpoch = 0;
static bool wasConnectedLastCheck = false;

static void saveToNvs() {
    prefs.putBytes("buf", logBuf, sizeof(logBuf));
    prefs.putInt("head", headIndex);
    prefs.putInt("count", count);
}

void offlineLogSetup() {
    prefs.begin("offline_log", false);
    size_t len = prefs.getBytesLength("buf");
    if (len == sizeof(logBuf)) {
        prefs.getBytes("buf", logBuf, sizeof(logBuf));
        headIndex = prefs.getInt("head", 0);
        count = prefs.getInt("count", 0);
    }
}

void offlineLogRecordIfDue(const String& jsonPayload) {
    if (mqttIsConnected()) return;

    unsigned long nowEpoch = rtcGetEpochUtc();
    if (nowEpoch == 0) return;

    unsigned long currentBucket = nowEpoch / 3600;
    unsigned long lastBucket = lastLogEpoch / 3600;
    if (lastLogEpoch != 0 && currentBucket == lastBucket) return;

    int idx = (headIndex + count) % OFFLINE_LOG_SIZE;
    logBuf[idx].epoch = nowEpoch;
    jsonPayload.toCharArray(logBuf[idx].payload, PAYLOAD_MAX_LEN);

    if (count < OFFLINE_LOG_SIZE) count++;
    else headIndex = (headIndex + 1) % OFFLINE_LOG_SIZE;

    lastLogEpoch = nowEpoch;
    saveToNvs();
}

void offlineLogSyncIfConnected() {
    bool nowConnected = mqttIsConnected();
    if (nowConnected && !wasConnectedLastCheck && count > 0) {
        for (int i = 0; i < count; i++) {
            int idx = (headIndex + i) % OFFLINE_LOG_SIZE;
            mqttPublishSensorData(String(logBuf[idx].payload));
        }
        Serial.println("Offline log synced: " + String(count) + " entri");
        count = 0;
        headIndex = 0;
        saveToNvs();
    }
    wasConnectedLastCheck = nowConnected;
}