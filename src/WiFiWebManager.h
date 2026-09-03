#pragma once

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <ArduinoOTA.h>
#include <Update.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
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

    // Firmware-Version der Anwendung (wird auf der /update-Seite angezeigt)
    void setFirmwareVersion(const String& version);

    // Debug-Modus Management
    void setDebugMode(bool enabled);
    bool getDebugMode();

    // WLAN-Status-LED (optional): eine adressierbare On-Board-RGB-LED (WS2812)
    // spiegelt den Verbindungsstatus. Standardmäßig DEAKTIVIERT — per
    // enableStatusLed() mit Pin aktivieren (idealerweise vor begin(), geht aber
    // auch zur Laufzeit). Angesteuert über neopixelWrite() aus dem ESP32-Core,
    // keine zusätzliche Bibliothek.
    //   grün         = verbunden, gute Feldstärke (RSSI >= Schwelle)
    //   gelb         = verbunden, schwache Feldstärke
    //   rot          = Verbindung verloren / (noch) kein STA-Connect
    //   rot blinkend = AP-Setup-Modus
    void enableStatusLed(uint8_t pin, uint8_t brightness = 40);
    void disableStatusLed();
    void setStatusLedPin(uint8_t pin);
    void setStatusLedBrightness(uint8_t brightness);
    void setStatusLedRssiThreshold(int dbm);   // Grenze gut/schwach, Default -70
    void setStatusLedSelfTest(bool enabled);   // Boot-Selbsttest rot/grün/blau

    void reset();

private:
    ContentHandler rootGetHandler = nullptr;
    ContentHandler rootPostHandler = nullptr;
    
    Preferences prefs;
    AsyncWebServer server{80};

    String ssid, password, hostname;
    String defaultHostname = "";  // Standard-Hostname aus Code
    String firmwareVersion = "";  // App-Version (via setFirmwareVersion)
    String ip, gateway, subnet, dns;
    bool useStaticIP = false;
    bool shouldReboot = false;

    bool ntpEnable = false;
    String ntpServer = "pool.ntp.org";

    // Debug-Modus
    bool debugMode = false;

    // WLAN-Status-LED (WS2812) — siehe enableStatusLed()
    enum class LedMode : uint8_t { Connected, Weak, Lost, AccessPoint };
    bool          ledEnabled     = false;   // per enableStatusLed() aktiviert
    bool          ledStarted     = false;   // begin() bereits gelaufen
    bool          ledSelfTest    = false;   // Boot-Selbsttest ausführen
    uint8_t       ledPin         = 48;      // GPIO der WS2812
    uint8_t       ledBrightness  = 40;      // Deckelung je Kanal (0..255)
    int           ledRssiGoodDbm = -70;     // Grenze gut/schwach
    LedMode       ledMode        = LedMode::Lost;
    bool          ledBlinkOn     = false;   // Blink-Phase im AP-Modus
    bool          ledForce       = false;   // erzwingt Neuausgabe (Pin/Helligkeit geändert)
    unsigned long ledLastMs      = 0;
    static const unsigned long LED_UPDATE_MS = 500;  // Auswerte- und Blink-Takt
    void statusLedBegin();
    void statusLedUpdate();
    void statusLedApply(uint8_t r, uint8_t g, uint8_t b);

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

    // Entkoppelte WLAN-Scan-Verwaltung: Der Scan läuft im loop()-Task, der
    // Web-Handler liest nur den gepufferten Options-String. So blockiert
    // /wlan die AsyncTCP-Task NICHT. scanMutex schützt cachedScanOptions
    // gegen gleichzeitigen Zugriff aus loop()- und async_tcp-Task.
    String cachedScanOptions;
    volatile bool scanRequested = false;
    bool storedSsidInRange = false;
    unsigned long lastScanMs = 0;
    SemaphoreHandle_t scanMutex = nullptr;
    void updateScanCache();

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
    bool isValidCustomKey(const String& key);

    // Custom-Data Key-Registry (ESP32 Preferences kann Keys nicht auflisten)
    static const char* CUSTOM_NS;      // Namespace für Custom Data
    static const char* CUSTOM_INDEX;   // Meta-Key mit \n-getrennter Key-Liste
    void registerCustomKey(const String& key);
    void unregisterCustomKey(const String& key);

    // Debug-Hilfsfunktionen
    void debugPrint(const String& message);
    void debugPrintln(const String& message);
    void debugPrintln(); // Überladung für leere Zeile
    void debugPrintf(const char* format, ...);

    String renderMenu(const String& currentPath);
    String htmlWrap(const String& menutitle, const String& currentPath, const String& content);
};