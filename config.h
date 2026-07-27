#ifndef CONFIG_H
#define CONFIG_H

// #define DEVICE_HWMS
#define DEVICE_GS

#if defined(DEVICE_HWMS) && defined(DEVICE_GS)
    #error "Hanya boleh define SATU device. Comment salah satu: DEVICE_HWMS atau DEVICE_GS"
#endif

#if !defined(DEVICE_HWMS) && !defined(DEVICE_GS)
    #error "Belum ada device dipilih. Uncomment DEVICE_HWMS atau DEVICE_GS di config.h"
#endif

#if defined(DEVICE_HWMS)
    #define DEVICE_ID    "hwms-01"
    #define DEVICE_TYPE  "hwms"
#elif defined(DEVICE_GS)
    #define DEVICE_ID    "gs-01"
    #define DEVICE_TYPE  "gs"
#endif

#define WIFI_SSID       "PLANT"
#define WIFI_PASSWORD   "jrgnbabeuun"

#define MQTT_HOST       "20.189.125.90"
#define MQTT_PORT       1883
#define MQTT_USERNAME   "esp32client"
#define MQTT_PASSWORD   "hwmsiot"

#define TOPIC_SENSOR_DATA       "sensor/" DEVICE_ID "/data"
#define TOPIC_DEVICE_STATUS     "device/" DEVICE_ID "/status"
#define TOPIC_DEVICE_HEARTBEAT  "device/" DEVICE_ID "/heartbeat"
#define TOPIC_DEVICE_CONFIG     "device/" DEVICE_ID "/config"

#define HEARTBEAT_INTERVAL_MS       30000UL   // 30 detik
#define MQTT_RECONNECT_INTERVAL_MS  5000UL    // jeda antar percobaan reconnect

#define PIN_I2C_SDA   21
#define PIN_I2C_SCL   22

#if defined(DEVICE_HWMS)

    #define PIN_FLOW_YF      25   // flow sensor YF — pulse counting, pakai attachInterrupt()
    #define PIN_RAINDROP     34   // raindrop sensor — analog, GPIO34 input-only (sesuai)

    #define PIN_JSN1_TRIG    5    // ultrasonic tandon RWH
    #define PIN_JSN1_ECHO    18
    #define PIN_JSN2_TRIG    16   // ultrasonic tandon air tanah (aman, WROOM tidak pakai PSRAM)
    #define PIN_JSN2_ECHO    17

    #define PIN_RELAY_PUMP_RWH          26   // CH1 — pompa RWH
    #define PIN_RELAY_PUMP_GROUNDWATER  27   // CH2 — pompa air tanah
    #define PIN_RELAY_SOLENOID_1        14   // CH3 — solenoid 1

    #define HWMS_DUAL_SOLENOID_ENABLED

    #if defined(HWMS_DUAL_SOLENOID_ENABLED)
        #define PIN_RELAY_SOLENOID_2    12
    #endif

#endif // DEVICE_HWMS

#if defined(DEVICE_GS)

    #define PIN_DHT22            4

    #define PIN_SOIL_MOISTURE    34   

    #define PIN_RELAY_PUMP_NOZZLE   18   
    #define PIN_RELAY_2             19   

    // #define GS_BUFFER_TANK_ENABLED

    #if defined(GS_BUFFER_TANK_ENABLED)
        #define PIN_JSN_TRIG   25 
        #define PIN_JSN_ECHO   26

        // #define PIN_RELAY_PUMP_BUFFER   18
        // #define PIN_RELAY_RESERVED      19
    #endif

#endif

#endif