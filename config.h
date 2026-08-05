#ifndef CONFIG_H
#define CONFIG_H

// =============================================================================
// PILIH DEVICE — uncomment SATU saja sebelum compile/upload, comment yang lain
// =============================================================================
#define DEVICE_HWMS
// #define DEVICE_GS

// =============================================================================
// GUARD — mencegah build tanpa device terpilih, atau dua-duanya sekaligus
// Kalau ini nge-error saat compile, itu SENGAJA — cek dua baris di atas.
// =============================================================================
#if defined(DEVICE_HWMS) && defined(DEVICE_GS)
    #error "Hanya boleh define SATU device. Comment salah satu: DEVICE_HWMS atau DEVICE_GS"
#endif

#if !defined(DEVICE_HWMS) && !defined(DEVICE_GS)
    #error "Belum ada device dipilih. Uncomment DEVICE_HWMS atau DEVICE_GS di config.h"
#endif

// =============================================================================
// IDENTITAS DEVICE (dipakai otomatis untuk bentuk topic MQTT di bawah)
// =============================================================================
#if defined(DEVICE_HWMS)
    #define DEVICE_ID    "hwms-01"
    #define DEVICE_TYPE  "hwms"
#elif defined(DEVICE_GS)
    #define DEVICE_ID    "gs-01"
    #define DEVICE_TYPE  "gs"
#endif

// =============================================================================
// MODUL ESP32: WROOM-32U (dikonfirmasi) — TIDAK ada konflik PSRAM di GPIO16/17,
// aman dipakai sebagai GPIO biasa (beda kasus kalau nanti pindah ke WROVER).
// =============================================================================

// =============================================================================
// WIFI — ganti sesuai jaringan tempat device akan dipasang
// =============================================================================
#define WIFI_SSID       "GANTI_NAMA_WIFI"
#define WIFI_PASSWORD   "GANTI_PASSWORD_WIFI"

// =============================================================================
// MQTT BROKER — VPS Azure
// =============================================================================
#define MQTT_HOST       "20.189.125.90"   // IP langsung; MQTT tetap port 1883 biasa, tidak pakai domain HTTPS
#define MQTT_PORT       1883
#define MQTT_USERNAME   "esp32client"
#define MQTT_PASSWORD   "GANTI_SESUAI_PASSWORD_MQTT_YANG_SUDAH_ADA"

// =============================================================================
// TOPIK MQTT — dibentuk otomatis dari DEVICE_ID, TIDAK PERLU diubah manual
// =============================================================================
#define TOPIC_SENSOR_DATA       "sensor/" DEVICE_ID "/data"
#define TOPIC_DEVICE_STATUS     "device/" DEVICE_ID "/status"
#define TOPIC_DEVICE_HEARTBEAT  "device/" DEVICE_ID "/heartbeat"
#define TOPIC_DEVICE_CONFIG     "device/" DEVICE_ID "/config"
#define TOPIC_DEVICE_COMMAND    "device/" DEVICE_ID "/command"

// =============================================================================
// INTERVAL DEFAULT (nanti bisa di-override oleh config dari server via MQTT)
// Backend menandai device offline jika tidak ada heartbeat > 90 detik,
// jadi HEARTBEAT_INTERVAL_MS di bawah ini WAJIB lebih kecil dari 90000.
// =============================================================================
#define HEARTBEAT_INTERVAL_MS       30000UL   // 30 detik
#define MQTT_RECONNECT_INTERVAL_MS  5000UL    // jeda antar percobaan reconnect

// =============================================================================
// PIN MAPPING — I2C untuk RTC DS3231 (SAMA untuk HWMS & GS)
// =============================================================================
#define PIN_I2C_SDA   21
#define PIN_I2C_SCL   22

// =============================================================================
// PIN MAPPING — KHUSUS DEVICE HWMS
// =============================================================================
#if defined(DEVICE_HWMS)

    #define PIN_FLOW_YF      25   // flow sensor YF — pulse counting, pakai attachInterrupt()
    #define PIN_RAINDROP     34   // raindrop sensor — analog, GPIO34 input-only (sesuai)

    #define PIN_JSN1_TRIG    5    // ultrasonic tandon RWH
    #define PIN_JSN1_ECHO    18
    #define PIN_JSN2_TRIG    16   // ultrasonic tandon air tanah (aman, WROOM tidak pakai PSRAM)
    #define PIN_JSN2_ECHO    17

    // --- Relay (nama deskriptif sesuai fungsi aktual) ---------------------
    #define PIN_RELAY_PUMP_RWH          26   // CH1 — pompa RWH
    #define PIN_RELAY_PUMP_GROUNDWATER  27   // CH2 — pompa air tanah
    #define PIN_RELAY_SOLENOID_1        14   // CH3 — solenoid 1

    // Kontingensi GPIO12 (strapping pin, CH4/Solenoid 2):
    // Kalau nanti terbukti bikin masalah boot, COMMENT baris di bawah ini.
    // Kode lain (actuator.cpp) akan otomatis fallback ke skema 1-solenoid
    // (source switching cukup pakai PIN_RELAY_SOLENOID_1 saja).
    #define HWMS_DUAL_SOLENOID_ENABLED

    #if defined(HWMS_DUAL_SOLENOID_ENABLED)
        #define PIN_RELAY_SOLENOID_2    12   // CH4 — solenoid 2 (GPIO12, strapping pin — lihat catatan di atas)
    #endif

#endif // DEVICE_HWMS

// =============================================================================
// PIN MAPPING — KHUSUS DEVICE GS
// =============================================================================
#if defined(DEVICE_GS)

    #define PIN_DHT22            4
    // DHT_TYPE didefinisikan di sensor.h saat include library DHT (enum DHT22 dari library-nya)

    #define PIN_SOIL_MOISTURE    34   // capacitive soil moisture — analog, GPIO34 input-only (sesuai)

    #define PIN_RELAY_PUMP_NOZZLE   18   // CH1 — pompa nozzle irigasi (aktuator aktif SEKARANG)
    #define PIN_RELAY_2             19   // CH2 — TODO: label fungsi aktual (belum ditentukan)

    // -------------------------------------------------------------------
    // FITUR MASA DEPAN — Buffer tank GS
    // Saat ini TIDAK dipasang (keputusan: GS tanpa tandon sendiri, cuma
    // monitoring). Uncomment flag di bawah begitu hardware buffer tank
    // + pompa tambahan ditambahkan, supaya ultrasonic & relay CH3/CH4
    // aktif tanpa perlu restrukturisasi kode dari awal.
    // -------------------------------------------------------------------
    #define GS_BUFFER_TANK_ENABLED

    #if defined(GS_BUFFER_TANK_ENABLED)
        #define PIN_JSN_TRIG   25   // ultrasonic buffer tank GS
        #define PIN_JSN_ECHO   26
    #endif

    // Channel 2 (PIN_RELAY_2) sengaja belum dipakai — cadangan untuk
    // aktuator taman lain di update berikutnya.

#endif // DEVICE_GS

#endif // CONFIG_H