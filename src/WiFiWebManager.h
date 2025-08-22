/*
 * WiFiWebManager Library v2.1.1
 *
 * Neue Features in v2.1.1:
 * - Light Sleep Mode Unterstützung für Energieeinsparung (~20-40mA)
 * - GPIO Wake-up Helper für externe Sensoren/Schalter
 * - Timer Wake-up Konfiguration für I2C/SPI Polling
 * - Wake-up Grund Analyse (esp_sleep_get_wakeup_cause)
 * - NTP Code-Default Konfiguration
 * - Wake-up Statistiken (optional)
 *
 * Breaking Changes: Keine
 * Kompatibilität: Vollständig rückwärtskompatibel
 */

#pragma once

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <ArduinoOTA.h>
#include <Update.h>
#include <vector>
#include <functional>
#include <esp_sleep.h>
#include <driver/gpio.h>

class WiFiWebManager
{
public:
    WiFiWebManager();
    void begin();
    void loop();

    using ContentHandler = std::function<String(AsyncWebServerRequest *)>;

    void addPage(const String &menutitle, const String &path, ContentHandler getHandler, ContentHandler postHandler = nullptr);
    void removePage(const String &path);

    // Erweiterte Custom Data API
    void saveCustomData(const String &key, const String &value);
    void saveCustomData(const String &key, int value);
    void saveCustomData(const String &key, bool value);
    void saveCustomData(const String &key, float value);

    String loadCustomData(const String &key, const String &defaultValue = "");
    int loadCustomDataInt(const String &key, int defaultValue = 0);
    bool loadCustomDataBool(const String &key, bool defaultValue = false);
    float loadCustomDataFloat(const String &key, float defaultValue = 0.0);

    bool hasCustomData(const String &key);
    void removeCustomData(const String &key);
    std::vector<String> getCustomDataKeys();

    // Hostname-Management
    void setDefaultHostname(const String &hostname);
    String getHostname();

    // NTP Code-Default Konfiguration (v2.1.1)
    void setDefaultNTP(bool enabled, const String &server = "pool.ntp.org");

    // Light Sleep Mode Konfiguration (v2.1.1)
    void setDefaultLightSleep(bool enabled);
    void setLightSleepTimer(uint64_t microseconds = 100000); // Default 100ms

    // GPIO Wake-up Helper (v2.1.1)
    void addWakeupGPIO(int pin, gpio_int_type_t mode = GPIO_INTR_ANY_EDGE);
    void removeWakeupGPIO(int pin);
    void clearAllWakeupGPIOs();

    // Wake-up Grund Analyse (v2.1.1)
    esp_sleep_wakeup_cause_t getLastWakeupCause();
    String getWakeupCauseString();
    int getWakeupGPIO(); // Welcher GPIO hat geweckt (falls GPIO wake-up)
    bool wasWokenByTimer();
    bool wasWokenByGPIO();
    bool wasWokenByWiFi();

    // Wake-up Statistiken (v2.1.1)
    void enableWakeupLogging(bool enabled = true);
    void clearWakeupStats();

    struct WakeupStats
    {
        uint32_t timerWakeups = 0;
        uint32_t gpioWakeups = 0;
        uint32_t wifiWakeups = 0;
        uint32_t otherWakeups = 0;
        uint32_t totalWakeups = 0;
    };
    WakeupStats getWakeupStats();

    // Debug-Modus Management
    void setDebugMode(bool enabled);
    bool getDebugMode();

    void reset();

private:
    ContentHandler rootGetHandler = nullptr;
    ContentHandler rootPostHandler = nullptr;

    Preferences prefs;
    AsyncWebServer server{80};

    String ssid, password, hostname;
    String defaultHostname = ""; // Standard-Hostname aus Code
    String ip, gateway, subnet, dns;
    bool useStaticIP = false;
    bool shouldReboot = false;

    // NTP Konfiguration
    bool ntpEnable = false;
    String ntpServer = "pool.ntp.org";
    bool defaultNtpEnable = false;
    String defaultNtpServer = "pool.ntp.org";

    // Light Sleep Konfiguration (v2.1.1)
    bool lightSleepEnabled = false;
    uint64_t lightSleepTimer = 100000; // 100ms default

    // GPIO Wake-up Management (v2.1.1)
    struct WakeupGPIO
    {
        int pin;
        gpio_int_type_t mode;
    };
    std::vector<WakeupGPIO> wakeupGPIOs;

    // Wake-up Analyse (v2.1.1)
    esp_sleep_wakeup_cause_t lastWakeupCause = ESP_SLEEP_WAKEUP_UNDEFINED;
    int lastWakeupGPIO = -1;
    bool wakeupLoggingEnabled = false;
    WakeupStats wakeupStats;
    bool firstLoopRun = true;

    // Debug-Modus
    bool debugMode = false;

    // Reset-Button Management
    static const int RESET_PIN = 0;
    static const unsigned long WIFI_RESET_TIME = 3000;  // 3 Sekunden für WLAN-Reset
    static const unsigned long FULL_RESET_TIME = 10000; // 10 Sekunden für Werks-Reset
    unsigned long resetButtonPressed = 0;
    bool resetButtonState = false;
    bool lastResetButtonState = false;

    // Boot-Attempt Management
    int wifiBootAttempts = 0;
    static const int MAX_BOOT_ATTEMPTS = 3;

    struct CustomPage
    {
        String title;
        String path;
        ContentHandler getHandler;
        ContentHandler postHandler;
    };
    std::vector<CustomPage> customPages;

    void loadConfig();
    void saveConfig();
    void saveNtpConfig(bool ntpEnable, const String &ntpServer);
    void clearAllConfig();
    void clearWiFiConfig();

    void startAP();
    bool connectToStoredWiFi();
    String getAvailableSSIDs();
    void setupWebServer();
    bool parseIPString(const String &str, IPAddress &out);
    void handleNTP();
    void handleResetButton();

    void resetBootAttempts();
    void incrementBootAttempts();
    bool isReservedKey(const String &key);

    // Light Sleep Management (v2.1.1)
    void configureLightSleep();
    void enableLibraryWakeups();
    void analyzeWakeupCause();
    void updateWakeupStats();
    void debugPrintWakeupCause();

    // Debug-Hilfsfunktionen
    void debugPrint(const String &message);
    void debugPrintln(const String &message);
    void debugPrintln(); // Überladung für leere Zeile
    void debugPrintf(const char *format, ...);

    String renderMenu(const String &currentPath);
    String htmlWrap(const String &menutitle, const String &currentPath, const String &content);
};