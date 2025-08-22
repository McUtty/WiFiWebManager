/*
 * WiFiWebManager Library Version Information
 *
 * Diese Datei enthält alle Versionsinformationen und Build-Details
 * der WiFiWebManager Library für einfache Referenzierung.
 */

#ifndef WIFIWEBMANAGER_VERSION_H
#define WIFIWEBMANAGER_VERSION_H

// =============================================================================
// Version Information
// =============================================================================

#define WIFIWEBMANAGER_VERSION_MAJOR 2
#define WIFIWEBMANAGER_VERSION_MINOR 1
#define WIFIWEBMANAGER_VERSION_PATCH 1

#define WIFIWEBMANAGER_VERSION_STRING "2.1.1"
#define WIFIWEBMANAGER_VERSION_INT 20101 // Major*10000 + Minor*100 + Patch

// Build Information
#define WIFIWEBMANAGER_BUILD_DATE __DATE__
#define WIFIWEBMANAGER_BUILD_TIME __TIME__
#define WIFIWEBMANAGER_BUILD_TIMESTAMP __DATE__ " " __TIME__

// =============================================================================
// Feature Flags (v2.1.1)
// =============================================================================

#define WIFIWEBMANAGER_FEATURE_LIGHT_SLEEP 1       // Light Sleep Mode
#define WIFIWEBMANAGER_FEATURE_GPIO_WAKEUP 1       // GPIO Wake-up Helper
#define WIFIWEBMANAGER_FEATURE_WAKEUP_ANALYSIS 1   // Wake-up Grund Analyse
#define WIFIWEBMANAGER_FEATURE_WAKEUP_STATISTICS 1 // Wake-up Statistiken
#define WIFIWEBMANAGER_FEATURE_NTP_DEFAULTS 1      // NTP Code-Defaults
#define WIFIWEBMANAGER_FEATURE_CUSTOM_DATA 1       // Custom Data API
#define WIFIWEBMANAGER_FEATURE_CUSTOM_PAGES 1      // Custom Web-Pages
#define WIFIWEBMANAGER_FEATURE_OTA_UPDATE 1        // OTA Firmware Updates
#define WIFIWEBMANAGER_FEATURE_HARDWARE_RESET 1    // Hardware Reset-Button
#define WIFIWEBMANAGER_FEATURE_MDNS_SUPPORT 1      // mDNS .local Domains

// =============================================================================
// Compatibility Information
// =============================================================================

#define WIFIWEBMANAGER_MIN_ESP32_CORE_VERSION "2.0.0"
#define WIFIWEBMANAGER_MIN_ARDUINO_IDE_VERSION "1.8.19"
#define WIFIWEBMANAGER_TESTED_ESP32_CORE_VERSION "2.0.14"

// Supported ESP32 Variants
#define WIFIWEBMANAGER_SUPPORTS_ESP32 1
#define WIFIWEBMANAGER_SUPPORTS_ESP32_S2 1
#define WIFIWEBMANAGER_SUPPORTS_ESP32_S3 1
#define WIFIWEBMANAGER_SUPPORTS_ESP32_C3 1
#define WIFIWEBMANAGER_SUPPORTS_ESP32_C6 1
#define WIFIWEBMANAGER_SUPPORTS_ESP32_H2 0 // Not yet tested

// =============================================================================
// Changelog Highlights (v2.1.1)
// =============================================================================

/*
 * Version 2.1.1 - Major Energy Efficiency Update
 *
 * NEW FEATURES:
 * ✨ Light Sleep Mode - Reduce power consumption from ~150mA to ~20-40mA
 * ✨ GPIO Wake-up Helper - Easy configuration for sensors and switches
 * ✨ Wake-up Cause Analysis - Intelligent response to different wake-up events
 * ✨ NTP Code-Defaults - Enable NTP by default via code configuration
 * ✨ Wake-up Statistics - Optional logging for debugging and optimization
 *
 * ENERGY EFFICIENCY:
 * ⚡ 80-85% power reduction with Light Sleep Mode
 * ⚡ WiFi stays connected and responsive (~3ms wake-up time)
 * ⚡ WebServer continues to work seamlessly
 * ⚡ Perfect for battery-powered IoT devices
 *
 * GPIO WAKE-UP SUPPORT:
 * 🔌 addWakeupGPIO() for easy sensor integration
 * 🔌 Support for all GPIO interrupt modes (edge/level)
 * 🔌 Automatic wake-up on sensor events
 * 🔌 Compatible with I2C/SPI sensors via timer wake-up
 *
 * INTELLIGENT WAKE-UP:
 * 🧠 getWakeupCause() and wasWokenByGPIO() for smart responses
 * 🧠 Differentiate between timer, GPIO, and WiFi wake-ups
 * 🧠 Optimize application logic based on wake-up reason
 * 🧠 Optional statistics for performance analysis
 *
 * BACKWARD COMPATIBILITY:
 * ✅ 100% compatible with v2.0.x code
 * ✅ No breaking changes to existing APIs
 * ✅ New features are opt-in only
 * ✅ Existing projects work without modifications
 */

// =============================================================================
// API Version Compatibility Macros
// =============================================================================

// Check if specific features are available
#define WIFIWEBMANAGER_HAS_LIGHT_SLEEP() \
    (WIFIWEBMANAGER_VERSION_INT >= 20101 && WIFIWEBMANAGER_FEATURE_LIGHT_SLEEP)

#define WIFIWEBMANAGER_HAS_GPIO_WAKEUP() \
    (WIFIWEBMANAGER_VERSION_INT >= 20101 && WIFIWEBMANAGER_FEATURE_GPIO_WAKEUP)

#define WIFIWEBMANAGER_HAS_WAKEUP_ANALYSIS() \
    (WIFIWEBMANAGER_VERSION_INT >= 20101 && WIFIWEBMANAGER_FEATURE_WAKEUP_ANALYSIS)

#define WIFIWEBMANAGER_HAS_NTP_DEFAULTS() \
    (WIFIWEBMANAGER_VERSION_INT >= 20101 && WIFIWEBMANAGER_FEATURE_NTP_DEFAULTS)

// Minimum version checks for applications
#define WIFIWEBMANAGER_VERSION_AT_LEAST(major, minor, patch) \
    (WIFIWEBMANAGER_VERSION_INT >= ((major) * 10000 + (minor) * 100 + (patch)))

// =============================================================================
// Debug and Information Macros
// =============================================================================

#define WIFIWEBMANAGER_PRINT_VERSION()                                                \
    do                                                                                \
    {                                                                                 \
        Serial.println("WiFiWebManager Library v" WIFIWEBMANAGER_VERSION_STRING);     \
        Serial.println("Build: " WIFIWEBMANAGER_BUILD_TIMESTAMP);                     \
        Serial.printf("Features: Light Sleep=%d, GPIO Wake-up=%d, NTP Defaults=%d\n", \
                      WIFIWEBMANAGER_FEATURE_LIGHT_SLEEP,                             \
                      WIFIWEBMANAGER_FEATURE_GPIO_WAKEUP,                             \
                      WIFIWEBMANAGER_FEATURE_NTP_DEFAULTS);                           \
    } while (0)

#define WIFIWEBMANAGER_PRINT_FEATURES()                                                                                      \
    do                                                                                                                       \
    {                                                                                                                        \
        Serial.println("=== WiFiWebManager Features ===");                                                                   \
        Serial.printf("Light Sleep Mode: %s\n", WIFIWEBMANAGER_FEATURE_LIGHT_SLEEP ? "Available" : "Not Available");         \
        Serial.printf("GPIO Wake-up: %s\n", WIFIWEBMANAGER_FEATURE_GPIO_WAKEUP ? "Available" : "Not Available");             \
        Serial.printf("Wake-up Analysis: %s\n", WIFIWEBMANAGER_FEATURE_WAKEUP_ANALYSIS ? "Available" : "Not Available");     \
        Serial.printf("Wake-up Statistics: %s\n", WIFIWEBMANAGER_FEATURE_WAKEUP_STATISTICS ? "Available" : "Not Available"); \
        Serial.printf("NTP Code-Defaults: %s\n", WIFIWEBMANAGER_FEATURE_NTP_DEFAULTS ? "Available" : "Not Available");       \
        Serial.printf("Custom Data API: %s\n", WIFIWEBMANAGER_FEATURE_CUSTOM_DATA ? "Available" : "Not Available");          \
        Serial.printf("Custom Web-Pages: %s\n", WIFIWEBMANAGER_FEATURE_CUSTOM_PAGES ? "Available" : "Not Available");        \
        Serial.printf("OTA Updates: %s\n", WIFIWEBMANAGER_FEATURE_OTA_UPDATE ? "Available" : "Not Available");               \
        Serial.printf("Hardware Reset: %s\n", WIFIWEBMANAGER_FEATURE_HARDWARE_RESET ? "Available" : "Not Available");        \
        Serial.printf("mDNS Support: %s\n", WIFIWEBMANAGER_FEATURE_MDNS_SUPPORT ? "Available" : "Not Available");            \
        Serial.println("==============================");                                                                    \
    } while (0)

// =============================================================================
// Deprecation Warnings (for future versions)
// =============================================================================

// Currently no deprecated features in v2.1.1
// Future versions may add deprecation warnings here

// =============================================================================
// Usage Examples in Comments
// =============================================================================

/*
 * USAGE EXAMPLES:
 *
 * 1. Version Check in Application:
 *
 *    #include <WiFiWebManagerVersion.h>
 *
 *    void setup() {
 *        Serial.begin(115200);
 *
 *        #if WIFIWEBMANAGER_VERSION_AT_LEAST(2, 1, 0)
 *            // Use v2.1.0+ features
 *            wwm.setDefaultLightSleep(true);
 *        #else
 *            Serial.println("Light Sleep requires v2.1.0+");
 *        #endif
 *    }
 *
 * 2. Feature Detection:
 *
 *    #if WIFIWEBMANAGER_HAS_LIGHT_SLEEP()
 *        wwm.setDefaultLightSleep(true);
 *        wwm.addWakeupGPIO(2, GPIO_INTR_ANY_EDGE);
 *    #endif
 *
 * 3. Runtime Version Info:
 *
 *    void printLibraryInfo() {
 *        WIFIWEBMANAGER_PRINT_VERSION();
 *        WIFIWEBMANAGER_PRINT_FEATURES();
 *    }
 *
 * 4. Conditional Compilation:
 *
 *    void loop() {
 *        wwm.loop();
 *
 *        #if WIFIWEBMANAGER_HAS_WAKEUP_ANALYSIS()
 *        if (wwm.wasWokenByGPIO()) {
 *            handleGPIOEvent(wwm.getWakeupGPIO());
 *        }
 *        #endif
 *    }
 */

#endif // WIFIWEBMANAGER_VERSION_H