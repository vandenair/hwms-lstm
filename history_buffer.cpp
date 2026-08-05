#include "history_buffer.h"
#include "config.h"

#if defined(DEVICE_GS)
#include <Preferences.h>
#include "rtc_time.h"
#include "sensor.h"

static Preferences prefs;
static HistoryEntry history[HISTORY_SIZE];
static int headIndex = 0;
static int count = 0;
static unsigned long lastRecordEpoch = 0;

static void saveToNvs() {
    prefs.putBytes("buf", history, sizeof(history));
    prefs.putInt("head", headIndex);
    prefs.putInt("count", count);
}

static void recordEntry(float temp, float hum) {
    int idx = (headIndex + count) % HISTORY_SIZE;
    history[idx].temperature_c = temp;
    history[idx].humidity_pct = hum;
    history[idx].hour_sin = rtcGetHourSin();
    history[idx].hour_cos = rtcGetHourCos();

    if (count < HISTORY_SIZE) {
        count++;
    } else {
        headIndex = (headIndex + 1) % HISTORY_SIZE;
    }
    saveToNvs();
}

void historyBufferSetup() {
    prefs.begin("gs_history", false);
    size_t len = prefs.getBytesLength("buf");
    if (len == sizeof(history)) {
        prefs.getBytes("buf", history, sizeof(history));
        headIndex = prefs.getInt("head", 0);
        count = prefs.getInt("count", 0);
    }
}

void historyBufferLoop() {
    unsigned long nowEpoch = rtcGetEpochUtc();
    if (nowEpoch == 0) return;

    unsigned long currentBucket = nowEpoch / 3600;
    unsigned long lastBucket = lastRecordEpoch / 3600;

    if (lastRecordEpoch == 0 || currentBucket != lastBucket) {
        recordEntry(sensorGetLatestTemperature(), sensorGetLatestHumidity());
        lastRecordEpoch = nowEpoch;
    }
}

int historyBufferCount() { return count; }
bool historyBufferIsFull() { return count >= HISTORY_SIZE; }

HistoryEntry historyBufferGet(int chronologicalIndex) {
    int idx = (headIndex + chronologicalIndex) % HISTORY_SIZE;
    return history[idx];
}

#else

void historyBufferSetup() {}
void historyBufferLoop() {}
int historyBufferCount() { return 0; }
bool historyBufferIsFull() { return false; }
HistoryEntry historyBufferGet(int chronologicalIndex) { return HistoryEntry{0, 0, 0, 0}; }

#endif