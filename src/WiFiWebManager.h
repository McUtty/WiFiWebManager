#pragma once

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <ArduinoOTA.h>
#include <Update.h>
#include <vector>
#include <functional>

class WiFiWebManager {
public:
    WiFiWebManager();
    void begin();
    void loop();

    using ContentHandler = std::function<String(AsyncWebServerRequest*)>;

    void addPage(const String& menutitle, const String& path, ContentHandler getHandler, ContentHandler postHandler = nullptr);
    void removePage(const String& path);

    // Erweiterte Custom Data API
    void saveCustomData(const String& key, const String& value);
    void saveCustomData(const String& key, int value);
    void saveCustomData(const String& key, bool value);
    void saveCustomData(const String& key, float value);
    
    String loadCustomData(const String& key, const String& defaultValue = "");
    int loadCustomDataInt(const String& key, int defaultValue = 0);
    bool loadCustomDataBool(const String& key, bool defaultValue = false);
    float loadCustomDataFloat(const String& key, float defaultValue = 0.0);
    
    bool hasCustomData(const String& key);
    void removeCustomData(const String& key);
    std::vector<String> getCustomDataKeys();

    // Hostname-Management
    void setDefaultHostname(const String& hostname);
    String getHostname();

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
    String defaultHostname = "";  // Standard-Hostname aus Code
    String ip, gateway, subnet, dns;
    bool useStaticIP = false;
    bool shouldReboot = false;

    bool ntpEnable = false;
    String ntpServer = "pool.ntp.org";

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

    struct CustomPage {
        String title;
        String path;
        ContentHandler getHandler;
        ContentHandler postHandler;
    };
    std::vector<CustomPage> customPages;

    void loadConfig();
    void saveConfig();
    void saveNtpConfig(bool ntpEnable, const String& ntpServer);
    void clearAllConfig();
    void clearWiFiConfig();
    
    void startAP();
    bool connectToStoredWiFi();
    String getAvailableSSIDs();
    void setupWebServer();
    bool parseIPString(const String& str, IPAddress& out);
    void handleNTP();
    void handleResetButton();
    
    void resetBootAttempts();
    void incrementBootAttempts();
    bool isReservedKey(const String& key);

    // Debug-Hilfsfunktionen
    void debugPrint(const String& message);
    void debugPrintln(const String& message);
    void debugPrintln(); // Überladung für leere Zeile
    void debugPrintf(const char* format, ...);

    String renderMenu(const String& currentPath);
    String htmlWrap(const String& menutitle, const String& currentPath, const String& content);
};