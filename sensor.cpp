#include "sensor.h"
#include "config.h"
#include "mqtt_handler.h"
#include "device_config.h"
#include "rtc_time.h"
#include "offline_log.h"
#include <ArduinoJson.h>

#if defined(DEVICE_GS)
#include <DHT.h>
static DHT dht(PIN_DHT22, DHT22);
#endif

static const unsigned long PUBLISH_INTERVAL_MS = 15000UL;
static unsigned long lastPublishMs = 0;

static float rawToPercent(int raw, int rawAtZeroPct, int rawAtHundredPct) {
    float pct = (float)(raw - rawAtZeroPct) / (float)(rawAtHundredPct - rawAtZeroPct) * 100.0;
    return constrain(pct, 0.0, 100.0);
}

static unsigned long readUltrasonicRaw(int trigPin, int echoPin) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    return pulseIn(echoPin, HIGH, 30000UL);
}

static float distanceToLevelCm(unsigned long durationUs, float emptyCm, float offsetCm) {
    if (durationUs == 0) return -1;
    float distanceCm = durationUs * 0.034 / 2.0;
    float level = emptyCm - distanceCm - offsetCm;
    return level < 0 ? 0 : level;
}

// ============================== HWMS ==============================
#if defined(DEVICE_HWMS)

static unsigned long lastRaindropMs = 0;
static unsigned long lastUltrasonicMs = 0;
static unsigned long lastFlowCalcMs = 0;

static bool latestRainDetected = false;
static float latestRainIntensityPct = 0;
static float latestTandonRwhCm = 0;
static float latestTandonGroundwaterCm = 0;
static float latestFlowRateLpm = 0;

static volatile unsigned long flowPulseCount = 0;

void IRAM_ATTR flowISR() {
    flowPulseCount++;
}

static void readRaindrop() {
    JsonDocument& cfg = getDeviceConfig();
    int dryRaw = cfg["calibration"]["raindrop"]["dry_raw"] | 4095;
    int wetRaw = cfg["calibration"]["raindrop"]["wet_raw"] | 1500;

    int raw = analogRead(PIN_RAINDROP);
    latestRainIntensityPct = rawToPercent(raw, dryRaw, wetRaw);
    latestRainDetected = latestRainIntensityPct >= 10;
}

static void readUltrasonic() {
    JsonDocument& cfg = getDeviceConfig();

    float emptyRwh = cfg["calibration"]["ultrasonic_rwh"]["empty_distance_cm"] | 100;
    float offsetRwh = cfg["calibration"]["ultrasonic_rwh"]["offset_cm"] | 0;
    unsigned long d1 = readUltrasonicRaw(PIN_JSN1_TRIG, PIN_JSN1_ECHO);
    float level1 = distanceToLevelCm(d1, emptyRwh, offsetRwh);
    if (level1 >= 0) latestTandonRwhCm = level1;
    else Serial.println("JSN1 timeout");

    float emptyGw = cfg["calibration"]["ultrasonic_groundwater"]["empty_distance_cm"] | 100;
    float offsetGw = cfg["calibration"]["ultrasonic_groundwater"]["offset_cm"] | 0;
    unsigned long d2 = readUltrasonicRaw(PIN_JSN2_TRIG, PIN_JSN2_ECHO);
    float level2 = distanceToLevelCm(d2, emptyGw, offsetGw);
    if (level2 >= 0) latestTandonGroundwaterCm = level2;
    else Serial.println("JSN2 timeout");
}

static void calcFlowRate(unsigned long elapsedMs) {
    JsonDocument& cfg = getDeviceConfig();
    float pulsesPerLiter = cfg["calibration"]["flow_yf"]["pulses_per_liter"] | 450;

    noInterrupts();
    unsigned long pulses = flowPulseCount;
    flowPulseCount = 0;
    interrupts();

    float liters = pulses / pulsesPerLiter;
    float minutes = elapsedMs / 60000.0;
    latestFlowRateLpm = minutes > 0 ? liters / minutes : 0;
}

static void hwmsSetup() {
    pinMode(PIN_RAINDROP, INPUT);
    pinMode(PIN_JSN1_TRIG, OUTPUT);
    pinMode(PIN_JSN1_ECHO, INPUT);
    pinMode(PIN_JSN2_TRIG, OUTPUT);
    pinMode(PIN_JSN2_ECHO, INPUT);
    pinMode(PIN_FLOW_YF, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_FLOW_YF), flowISR, RISING);
}

static void hwmsLoop() {
    JsonDocument& cfg = getDeviceConfig();
    unsigned long raindropInterval = (cfg["interval_seconds"]["raindrop"] | 10) * 1000UL;
    unsigned long ultrasonicInterval = (cfg["interval_seconds"]["ultrasonic"] | 60) * 1000UL;
    unsigned long flowInterval = (cfg["interval_seconds"]["flow"] | 2) * 1000UL;

    unsigned long now = millis();

    if (now - lastRaindropMs >= raindropInterval) {
        lastRaindropMs = now;
        readRaindrop();
    }
    if (now - lastUltrasonicMs >= ultrasonicInterval) {
        lastUltrasonicMs = now;
        readUltrasonic();
    }
    if (now - lastFlowCalcMs >= flowInterval) {
        calcFlowRate(now - lastFlowCalcMs);
        lastFlowCalcMs = now;
    }
}

static void hwmsBuildPayload(JsonDocument& doc) {
    doc["rain_detected"] = latestRainDetected;
    doc["flow_rate_lpm"] = latestFlowRateLpm;
    doc["rwh_level_cm"] = latestTandonRwhCm;
    doc["groundwater_level_cm"] = latestTandonGroundwaterCm;
}

#endif // DEVICE_HWMS

// ============================== GS ==============================
#if defined(DEVICE_GS)

static unsigned long lastDhtMs = 0;
static unsigned long lastSoilMs = 0;
static unsigned long lastBufferMs = 0;

static float latestTemperatureC = 0;
static float latestHumidityPct = 0;
static float latestSoilMoisturePct = 0;
static float latestBufferLevelCm = 0;

static void readDht() {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (isnan(t) || isnan(h)) {
        Serial.println("DHT22 read failed");
        return;
    }
    latestTemperatureC = t;
    latestHumidityPct = h;
}

static void readSoil() {
    JsonDocument& cfg = getDeviceConfig();
    int dryRaw = cfg["calibration"]["soil_moisture"]["dry_raw"] | 3200;
    int wetRaw = cfg["calibration"]["soil_moisture"]["wet_raw"] | 1200;

    int raw = analogRead(PIN_SOIL_MOISTURE);
    latestSoilMoisturePct = rawToPercent(raw, dryRaw, wetRaw);
}

#if defined(GS_BUFFER_TANK_ENABLED)
static void readBufferLevel() {
    JsonDocument& cfg = getDeviceConfig();
    float emptyCm = cfg["calibration"]["buffer_tank"]["empty_distance_cm"] | 100;
    float offsetCm = cfg["calibration"]["buffer_tank"]["offset_cm"] | 0;

    unsigned long d = readUltrasonicRaw(PIN_JSN_TRIG, PIN_JSN_ECHO);
    float level = distanceToLevelCm(d, emptyCm, offsetCm);
    if (level >= 0) latestBufferLevelCm = level;
    else Serial.println("Buffer ultrasonic timeout");
}
#endif

static void gsSetup() {
    dht.begin();
    pinMode(PIN_SOIL_MOISTURE, INPUT);
#if defined(GS_BUFFER_TANK_ENABLED)
    pinMode(PIN_JSN_TRIG, OUTPUT);
    pinMode(PIN_JSN_ECHO, INPUT);
#endif
}

static void gsLoop() {
    JsonDocument& cfg = getDeviceConfig();
    unsigned long dhtInterval = (cfg["interval_seconds"]["dht22"] | 60) * 1000UL;
    unsigned long soilInterval = (cfg["interval_seconds"]["soil_moisture"] | 30) * 1000UL;

    unsigned long now = millis();

    if (now - lastDhtMs >= dhtInterval) {
        lastDhtMs = now;
        readDht();
    }
    if (now - lastSoilMs >= soilInterval) {
        lastSoilMs = now;
        readSoil();
    }
#if defined(GS_BUFFER_TANK_ENABLED)
    unsigned long bufferInterval = (cfg["interval_seconds"]["buffer_tank"] | 60) * 1000UL;
    if (now - lastBufferMs >= bufferInterval) {
        lastBufferMs = now;
        readBufferLevel();
    }
#endif
}

static void gsBuildPayload(JsonDocument& doc) {
    doc["temperature_c"] = latestTemperatureC;
    doc["humidity_pct"] = latestHumidityPct;
    doc["soil_moisture_pct"] = latestSoilMoisturePct;
#if defined(GS_BUFFER_TANK_ENABLED)
    doc["buffer_level_cm"] = latestBufferLevelCm;
#endif
}

float sensorGetLatestTemperature() { return latestTemperatureC; }
float sensorGetLatestHumidity() { return latestHumidityPct; }

#endif // DEVICE_GS

// ============================== Umum ==============================

void sensorSetup() {
#if defined(DEVICE_HWMS)
    hwmsSetup();
#endif
#if defined(DEVICE_GS)
    gsSetup();
#endif
}

void sensorLoop() {
#if defined(DEVICE_HWMS)
    hwmsLoop();
#endif
#if defined(DEVICE_GS)
    gsLoop();
#endif

    unsigned long now = millis();
    if (now - lastPublishMs < PUBLISH_INTERVAL_MS) return;
    lastPublishMs = now;

    JsonDocument doc;
    doc["ts"] = rtcGetEpochUtc();
#if defined(DEVICE_HWMS)
    hwmsBuildPayload(doc);
#endif
#if defined(DEVICE_GS)
    gsBuildPayload(doc);
#endif

    String payload;
    serializeJson(doc, payload);

    if (mqttIsConnected()) {
        mqttPublishSensorData(payload);
    } else {
        offlineLogRecordIfDue(payload);
    }
    Serial.println(payload);
}