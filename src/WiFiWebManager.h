#pragma once

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <ArduinoOTA.h>
#include <Update.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
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

    // Wird VOR dem ersten Flash-Schreiben aufgerufen (OTA via /update UND
    // ArduinoOTA/espota). Consumer stoppt hier stoerende Peripherie
    // (z. B. Kamera deinit, Ausgaenge sicher). Optional.
    void setOnUpdateStart(std::function<void()> cb);

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

    // FreeRTOS-Service-Task (Default AN): Die Lib erledigt ihre Wartung
    // (Reset-Button, OTA-Handle, Status-LED, WLAN-Scan/-Reconnect, OTA-Stall-
    // Check) in einer eigenen Task "wfwm_svc" statt im Consumer-loop(). Die
    // öffentliche loop() wird dann zum No-Op — bestehende Sketches, die loop()
    // aufrufen, laufen unverändert weiter. Vor begin() aufrufen.
    //   false = exakt bisheriges Verhalten (Consumer ruft loop() selbst).
    void setServiceTask(bool enabled);

    // Task-Watchdog (Default AN). Vor begin() aufrufen. Überwacht NUR die
    // Service-Task (und vom Consumer bewusst eingehängte Tasks), NICHT den
    // Consumer-loop(). timeoutS/panic wie IDF esp_task_wdt_init.
    void enableWatchdog(bool on, uint32_t timeoutS = 30, bool panic = true);
    void watchdogAddCurrentTask();      // esp_task_wdt_add(NULL)
    void watchdogFeedCurrentTask();     // esp_task_wdt_reset()
    void watchdogRemoveCurrentTask();   // esp_task_wdt_delete(NULL)

    // Stall-Timeout für abgebrochene OTA-Uploads (Default 8000 ms). Kommt nach
    // onUpdateStart kein Chunk mehr, wird nach dieser Zeit abgebrochen + neu
    // gestartet (siehe Service-Task).
    void setOtaStallTimeout(uint32_t ms);

    void reset();

private:
    ContentHandler rootGetHandler = nullptr;
    ContentHandler rootPostHandler = nullptr;
    
    Preferences prefs;
    AsyncWebServer server{80};

    String ssid, password, hostname;
    String defaultHostname = "";  // Standard-Hostname aus Code
    String firmwareVersion = "";  // App-Version (via setFirmwareVersion)
    std::function<void()> onUpdateStart = nullptr;

    // OTA-Stall-Selbstheilung (Teil 2): verfolgt den /update- bzw. ArduinoOTA-
    // Fortschritt; die Service-Task bricht einen mittendrin abgerissenen Upload
    // nach otaStallTimeoutMs ab und startet neu (stellt alte FW + Peripherie her).
    volatile bool          otaInProgress     = false;
    volatile unsigned long otaLastChunkMs    = 0;
    unsigned long          otaStallTimeoutMs = 8000;

    // FreeRTOS-Service-Task + Task-Watchdog (Teil 3)
    bool          serviceTaskEnabled = true;    // per setServiceTask() vor begin()
    volatile bool serviceTaskRunning = false;   // true -> öffentliche loop() = No-Op
    TaskHandle_t  serviceTaskHandle  = nullptr;
    bool          wdtEnabled  = true;           // per enableWatchdog() vor begin()
    uint32_t      wdtTimeoutS = 30;
    bool          wdtPanic    = true;
    void serviceIteration();                    // ein Wartungsdurchlauf (Task ODER loop())
    void serviceTaskLoop();                     // Endlosschleife der Service-Task
    static void serviceTaskTramp(void* arg);    // FreeRTOS-Einsprung
    void initWatchdog();
    void logResetReason();

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