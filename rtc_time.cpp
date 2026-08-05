#include "rtc_time.h"
#include "config.h"
#include <Wire.h>
#include <RTClib.h>
#include <WiFi.h>
#include <time.h>

static RTC_DS3231 rtc;
static bool rtcOk = false;

static const char* NTP_SERVER = "pool.ntp.org";
static const long WIB_OFFSET_SEC = 7L * 3600;

static unsigned long lastNtpSyncMs = 0;
static const unsigned long NTP_SYNC_INTERVAL_MS = 6UL * 60 * 60 * 1000;

static bool syncFromNtp() {
    if (WiFi.status() != WL_CONNECTED) return false;
    configTime(0, 0, NTP_SERVER);

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, 5000)) return false;

    time_t utcNow;
    time(&utcNow);

    if (rtcOk) rtc.adjust(DateTime((uint32_t)utcNow));
    Serial.println("NTP sync OK");
    return true;
}

void rtcTimeSetup() {
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    rtcOk = rtc.begin();
    if (!rtcOk) Serial.println("RTC DS3231 tidak terdeteksi");

    if (!syncFromNtp() && rtcOk) {
        struct timeval tv = { .tv_sec = (time_t)rtc.now().unixtime(), .tv_usec = 0 };
        settimeofday(&tv, nullptr);
        Serial.println("Pakai waktu dari RTC (offline)");
    }
    lastNtpSyncMs = millis();
}

void rtcTimeLoop() {
    unsigned long now = millis();
    if (now - lastNtpSyncMs >= NTP_SYNC_INTERVAL_MS) {
        lastNtpSyncMs = now;
        syncFromNtp();
    }
}

unsigned long rtcGetEpochUtc() {
    time_t now;
    time(&now);
    if (now > 1700000000) return (unsigned long)now;
    if (rtcOk) return rtc.now().unixtime();
    return 0;
}

int rtcGetHourLocal() {
    unsigned long localEpoch = rtcGetEpochUtc() + WIB_OFFSET_SEC;
    struct tm* t = gmtime((time_t*)&localEpoch);
    return t->tm_hour;
}

float rtcGetHourSin() {
    return sin(2.0 * PI * rtcGetHourLocal() / 24.0);
}

float rtcGetHourCos() {
    return cos(2.0 * PI * rtcGetHourLocal() / 24.0);
}