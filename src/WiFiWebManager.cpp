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

#include "WiFiWebManager.h"
#include "WiFiWebManagerVersion.h"

#include <cstdarg>
#include <cstdio>

namespace
{
    constexpr const char *CUSTOM_DATA_NAMESPACE = "customdata";
    constexpr const char *CUSTOM_DATA_KEYS_KEY = "__keys__";

    std::vector<String> splitKeyList(const String &raw)
    {
        std::vector<String> keys;
        int start = 0;
        while (start < raw.length())
        {
            int end = raw.indexOf(',', start);
            if (end < 0)
            {
                end = raw.length();
            }
            String key = raw.substring(start, end);
            key.trim();
            if (key.length() > 0)
            {
                keys.push_back(key);
            }
            start = end + 1;
        }
        return keys;
    }

    String joinKeyList(const std::vector<String> &keys)
    {
        String raw;
        for (size_t i = 0; i < keys.size(); ++i)
        {
            if (i > 0)
            {
                raw += ',';
            }
            raw += keys[i];
        }
        return raw;
    }

    std::vector<String> loadStoredKeys()
    {
        Preferences prefs;
        prefs.begin(CUSTOM_DATA_NAMESPACE, true);
        String raw = prefs.getString(CUSTOM_DATA_KEYS_KEY, "");
        prefs.end();
        return splitKeyList(raw);
    }

    void storeKeys(const std::vector<String> &keys)
    {
        Preferences prefs;
        prefs.begin(CUSTOM_DATA_NAMESPACE, false);
        if (keys.empty())
        {
            prefs.remove(CUSTOM_DATA_KEYS_KEY);
        }
        else
        {
            prefs.putString(CUSTOM_DATA_KEYS_KEY, joinKeyList(keys));
        }
        prefs.end();
    }

    void addKeyToStore(const String &key)
    {
        if (key.length() == 0)
            return;

        auto keys = loadStoredKeys();
        for (const auto &existing : keys)
        {
            if (existing == key)
            {
                return;
            }
        }
        keys.push_back(key);
        storeKeys(keys);
    }

    void removeKeyFromStore(const String &key)
    {
        if (key.length() == 0)
            return;

        auto keys = loadStoredKeys();
        bool modified = false;
        for (auto it = keys.begin(); it != keys.end();)
        {
            if (*it == key)
            {
                it = keys.erase(it);
                modified = true;
            }
            else
            {
                ++it;
            }
        }
        if (modified)
        {
            storeKeys(keys);
        }
    }

    String htmlEscape(const String &value)
    {
        String escaped;
        escaped.reserve(value.length());
        for (size_t i = 0; i < value.length(); ++i)
        {
            const char c = value[i];
            switch (c)
            {
            case '&':
                escaped += F("&amp;");
                break;
            case '<':
                escaped += F("&lt;");
                break;
            case '>':
                escaped += F("&gt;");
                break;
            case '\'':
                escaped += F("&#39;");
                break;
            case '"':
                escaped += F("&quot;");
                break;
            default:
                escaped += c;
                break;
            }
        }
        return escaped;
    }
}

WiFiWebManager::WiFiWebManager()
{
    // Reset-Button Pin als Input mit Pull-up konfigurieren
    pinMode(RESET_PIN, INPUT_PULLUP);
}

void WiFiWebManager::begin()
{
    debugPrintln("\n=== Starte WiFiWebManager v2.1.1 ===");
    loadConfig();

    // Prüfe Boot-Attempts und entscheide Verbindungsstrategie
    if (wifiBootAttempts >= MAX_BOOT_ATTEMPTS)
    {
        debugPrintf("Maximale Boot-Versuche erreicht (%d), starte AP-Modus\n", wifiBootAttempts);
        startAP();
    }
    else if (ssid.length() > 0 && password.length() > 0)
    {
        incrementBootAttempts();
        debugPrintf("WLAN-Verbindungsversuch %d/%d\n", wifiBootAttempts, MAX_BOOT_ATTEMPTS);

        if (connectToStoredWiFi())
        {
            debugPrintln("WLAN verbunden!");
            debugPrint("IP-Adresse: ");
            debugPrintln(WiFi.localIP().toString());
            resetBootAttempts(); // Erfolgreiche Verbindung - Counter zurücksetzen
        }
        else
        {
            debugPrintln("WLAN-Verbindung fehlgeschlagen");
            if (wifiBootAttempts < MAX_BOOT_ATTEMPTS)
            {
                debugPrintf("Neustart für Versuch %d/%d...\n", wifiBootAttempts + 1, MAX_BOOT_ATTEMPTS);
                delay(1000);
                ESP.restart();
            }
            else
            {
                debugPrintln("Alle Versuche fehlgeschlagen, starte AP-Modus");
                startAP();
            }
        }
    }
    else
    {
        debugPrintln("Keine WLAN-Daten gespeichert, starte AP-Modus");
        startAP();
    }

    handleNTP();
    setupWebServer();
    ArduinoOTA.begin();

    // Light Sleep konfigurieren (v2.1.1)
    if (lightSleepEnabled)
    {
        configureLightSleep();
        debugPrintln("Light Sleep Mode aktiviert");
    }
}

void WiFiWebManager::loop()
{
    if (shouldReboot)
    {
        debugPrintln("Reboot...");
        delay(500);
        ESP.restart();
    }

    // Wake-up Analyse beim ersten Durchlauf oder nach Sleep (v2.1.1)
    if (firstLoopRun || lightSleepEnabled)
    {
        analyzeWakeupCause();
        firstLoopRun = false;
    }

    handleResetButton();
    ArduinoOTA.handle();

    // Überwachung der WLAN-Verbindung (alle 30 Sekunden)
    static unsigned long lastWiFiCheck = 0;
    if (millis() - lastWiFiCheck > 30000)
    {
        lastWiFiCheck = millis();

        if (WiFi.getMode() == WIFI_STA && WiFi.status() != WL_CONNECTED)
        {
            debugPrintln("WLAN-Verbindung verloren, versuche Reconnect...");
            // Nur versuchen wenn WLAN-Daten vorhanden sind
            if (ssid.length() > 0 && connectToStoredWiFi())
            {
                debugPrintln("Reconnect erfolgreich!");
            }
            else
            {
                debugPrintln("Reconnect fehlgeschlagen oder keine WLAN-Daten vorhanden");
            }
        }
        else if (WiFi.getMode() == WIFI_AP && ssid.length() > 0)
        {
            // Im AP-Modus: Prüfe ob gespeichertes WLAN verfügbar ist
            debugPrintln("AP-Modus: Prüfe ob gespeichertes WLAN verfügbar ist...");
            if (connectToStoredWiFi())
            {
                debugPrintln("Gespeichertes WLAN verfügbar - Wechsel zu STA-Modus!");
                resetBootAttempts(); // Erfolgreiche Verbindung nach AP-Modus
            }
        }
    }

    // Light Sleep am Ende der Loop (v2.1.1)
    if (lightSleepEnabled)
    {
        esp_light_sleep_start();
    }
}

void WiFiWebManager::handleResetButton()
{
    bool currentState = digitalRead(RESET_PIN) == LOW; // Active LOW

    if (currentState && !lastResetButtonState)
    {
        // Button wurde gedrückt
        resetButtonPressed = millis();
        resetButtonState = true;
        debugPrintln("Reset-Button gedrückt...");
    }
    else if (!currentState && lastResetButtonState)
    {
        // Button wurde losgelassen
        unsigned long pressTime = millis() - resetButtonPressed;

        if (pressTime >= WIFI_RESET_TIME && pressTime < FULL_RESET_TIME)
        {
            debugPrintln("Reset-Button 3-10 Sekunden gedrückt - Lösche WLAN-Daten!");
            clearWiFiConfig();
            debugPrintln("WLAN-Reset durchgeführt - Neustart...");
            delay(1000);
            shouldReboot = true;
        }
        else if (pressTime >= FULL_RESET_TIME)
        {
            debugPrintln("Reset-Button >10 Sekunden gedrückt - Werks-Reset!");
            clearAllConfig();
            debugPrintln("Werks-Reset durchgeführt - Neustart...");
            delay(1000);
            shouldReboot = true;
        }

        resetButtonState = false;
        resetButtonPressed = 0;
    }

    lastResetButtonState = currentState;
}

void WiFiWebManager::loadConfig()
{
    prefs.begin("netcfg", true);

    ssid = prefs.getString("ssid", "");
    password = prefs.getString("pwd", "");
    hostname = prefs.getString("hostname", "");

    // Wenn kein Hostname gesetzt und Default vorhanden, verwende Default
    if (hostname.length() == 0 && defaultHostname.length() > 0)
    {
        hostname = defaultHostname;
    }

    useStaticIP = prefs.getBool("useStaticIP", false);
    ip = prefs.getString("ip", "");
    gateway = prefs.getString("gateway", "");
    subnet = prefs.getString("subnet", "");
    dns = prefs.getString("dns", "");

    // NTP Konfiguration - verwende Code-Default falls nicht gespeichert (v2.1.1)
    ntpEnable = prefs.getBool("ntpEnable", defaultNtpEnable);
    ntpServer = prefs.getString("ntpServer", defaultNtpServer);

    // Light Sleep & Logging Einstellungen
    lightSleepEnabled = prefs.getBool("lightSleep", lightSleepEnabled);
    lightSleepTimer = prefs.getULong64("lightSleepTimer", lightSleepTimer);
    wakeupLoggingEnabled = prefs.getBool("wakeupLogging", wakeupLoggingEnabled);

    wifiBootAttempts = prefs.getInt("bootAttempts", 0);

    prefs.end();
    debugPrintln("Konfiguration geladen.");

    if (ssid.length() > 0)
    {
        debugPrintf("Gespeichertes WLAN: %s\n", ssid.c_str());
    }
    debugPrintf("Boot-Versuche: %d\n", wifiBootAttempts);
    debugPrintf("NTP: %s, Server: %s\n", ntpEnable ? "aktiviert" : "deaktiviert", ntpServer.c_str());
}

void WiFiWebManager::saveConfig()
{
    prefs.begin("netcfg", false);
    prefs.putString("ssid", ssid);
    prefs.putString("pwd", password);
    prefs.putString("hostname", hostname);
    prefs.putBool("useStaticIP", useStaticIP);
    prefs.putString("ip", ip);
    prefs.putString("gateway", gateway);
    prefs.putString("subnet", subnet);
    prefs.putString("dns", dns);
    prefs.putBool("ntpEnable", ntpEnable);
    prefs.putString("ntpServer", ntpServer);
    prefs.putBool("lightSleep", lightSleepEnabled);
    prefs.putULong64("lightSleepTimer", lightSleepTimer);
    prefs.putBool("wakeupLogging", wakeupLoggingEnabled);
    prefs.putInt("bootAttempts", wifiBootAttempts);
    prefs.end();
    debugPrintln("Konfiguration gespeichert.");
}

void WiFiWebManager::saveNtpConfig(bool ntpEn, const String &ntpSrv)
{
    prefs.begin("netcfg", false);
    prefs.putBool("ntpEnable", ntpEn);
    prefs.putString("ntpServer", ntpSrv);
    prefs.end();
    ntpEnable = ntpEn;
    ntpServer = ntpSrv;
    handleNTP();
}

void WiFiWebManager::clearWiFiConfig()
{
    // WICHTIG: Zuerst lokale Variablen löschen für sauberen Zustand
    ssid = "";
    password = "";
    wifiBootAttempts = 0;

    // Dann aus Preferences löschen
    prefs.begin("netcfg", false);
    prefs.remove("ssid");
    prefs.remove("pwd");
    prefs.remove("bootAttempts");
    prefs.end();

    debugPrintln("WLAN-Konfiguration gelöscht!");
}

void WiFiWebManager::clearAllConfig()
{
    // Zuerst alle lokalen Variablen zurücksetzen
    ssid = "";
    password = "";
    hostname = "";
    useStaticIP = false;
    ip = "";
    gateway = "";
    subnet = "";
    dns = "";
    ntpEnable = defaultNtpEnable; // Verwende Code-Default (v2.1.1)
    ntpServer = defaultNtpServer;
    lightSleepEnabled = false;
    lightSleepTimer = 100000;
    wakeupLoggingEnabled = false;
    wakeupStats = WakeupStats{};
    firstLoopRun = true;
    wifiBootAttempts = 0;

    // Dann alle Preferences löschen
    prefs.begin("netcfg", false);
    prefs.clear();
    prefs.end();

    // Custom Data auch löschen
    prefs.begin("customdata", false);
    prefs.clear();
    prefs.end();

    debugPrintln("Alle Einstellungen gelöscht!");
}

void WiFiWebManager::reset()
{
    clearAllConfig();
    shouldReboot = true;
}

void WiFiWebManager::resetBootAttempts()
{
    wifiBootAttempts = 0;
    prefs.begin("netcfg", false);
    prefs.putInt("bootAttempts", 0);
    prefs.end();
}

void WiFiWebManager::incrementBootAttempts()
{
    if (wifiBootAttempts < MAX_BOOT_ATTEMPTS)
    {
        wifiBootAttempts++;
    }
    prefs.begin("netcfg", false);
    prefs.putInt("bootAttempts", wifiBootAttempts);
    prefs.end();
}

bool WiFiWebManager::connectToStoredWiFi()
{
    if (ssid.length() == 0)
        return false;

    WiFi.mode(WIFI_STA);

    if (hostname.length() > 0)
    {
        WiFi.setHostname(hostname.c_str());
        debugPrint("Setze Hostname auf: ");
        debugPrintln(hostname);
    }

    if (useStaticIP)
    {
        IPAddress ip_, gateway_, subnet_, dns_;
        if (parseIPString(ip, ip_) && parseIPString(gateway, gateway_) &&
            parseIPString(subnet, subnet_) && parseIPString(dns, dns_))
        {
            WiFi.config(ip_, gateway_, subnet_, dns_);
            debugPrintln("Statische IP-Konfiguration gesetzt.");
        }
        else
        {
            debugPrintln("Warnung: Ungültige statische IP-Konfiguration!");
        }
    }

    debugPrintf("Verbinde mit WLAN: %s\n", ssid.c_str());
    WiFi.begin(ssid.c_str(), password.c_str());

    // 5 Sekunden Timeout
    for (int i = 0; i < 10 && WiFi.status() != WL_CONNECTED; i++)
    {
        delay(500);
        debugPrint(".");
    }
    debugPrintln("");

    if (WiFi.status() == WL_CONNECTED)
    {
        debugPrintf("Verbunden mit: %s\n", ssid.c_str());
        return true;
    }
    else
    {
        debugPrintf("Verbindung zu %s fehlgeschlagen\n", ssid.c_str());
        WiFi.disconnect();
        return false;
    }
}

void WiFiWebManager::startAP()
{
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP32_SETUP");
    debugPrintln("Access Point gestartet: ESP32_SETUP");
    debugPrintln("AP-IP: 192.168.4.1");
}

String WiFiWebManager::getAvailableSSIDs()
{
    int n = WiFi.scanNetworks();
    String options = "";
    for (int i = 0; i < n; ++i)
    {
        String ssidValue = htmlEscape(WiFi.SSID(i));
        options += "<option value=\"" + ssidValue + "\"";
        if (ssid == WiFi.SSID(i))
        {
            options += " label=\"" + ssidValue + " (gespeichert)\"";
        }
        options += "></option>";
    }
    return options;
}

bool WiFiWebManager::parseIPString(const String &str, IPAddress &out)
{
    int parts[4];
    if (sscanf(str.c_str(), "%d.%d.%d.%d", &parts[0], &parts[1], &parts[2], &parts[3]) == 4)
    {
        out = IPAddress(parts[0], parts[1], parts[2], parts[3]);
        return true;
    }
    return false;
}

void WiFiWebManager::handleNTP()
{
    if (ntpEnable)
    {
        configTime(0, 0, ntpServer.c_str());
        debugPrint("NTP aktiviert, Server: ");
        debugPrintln(ntpServer);
    }
}

// =============================================================================
// NTP Code-Default Konfiguration (v2.1.1)
// =============================================================================

void WiFiWebManager::setDefaultNTP(bool enabled, const String &server)
{
    defaultNtpEnable = enabled;
    defaultNtpServer = server;

    // Wenn noch keine NTP-Konfiguration gespeichert, verwende Defaults
    prefs.begin("netcfg", true);
    if (!prefs.isKey("ntpEnable"))
    {
        ntpEnable = defaultNtpEnable;
        ntpServer = defaultNtpServer;
    }
    prefs.end();

    debugPrintf("NTP Code-Default gesetzt: %s, Server: %s\n",
                enabled ? "aktiviert" : "deaktiviert", server.c_str());
}

// =============================================================================
// Light Sleep Mode Implementierung (v2.1.1)
// =============================================================================

void WiFiWebManager::setDefaultLightSleep(bool enabled)
{
    lightSleepEnabled = enabled;
    debugPrintf("Light Sleep Mode: %s\n", enabled ? "aktiviert" : "deaktiviert");
}

void WiFiWebManager::setLightSleepTimer(uint64_t microseconds)
{
    lightSleepTimer = microseconds;
    debugPrintf("Light Sleep Timer: %llu µs\n", microseconds);

    // Wenn Light Sleep bereits konfiguriert, neu konfigurieren
    if (lightSleepEnabled)
    {
        configureLightSleep();
    }
}

void WiFiWebManager::addWakeupGPIO(int pin, gpio_int_type_t mode)
{
    // Prüfe ob GPIO bereits hinzugefügt
    for (auto &wgpio : wakeupGPIOs)
    {
        if (wgpio.pin == pin)
        {
            wgpio.mode = mode; // Aktualisiere Modus
            debugPrintf("GPIO %d Wake-up Modus aktualisiert\n", pin);
            if (lightSleepEnabled)
                configureLightSleep();
            return;
        }
    }

    // GPIO hinzufügen
    wakeupGPIOs.push_back({pin, mode});
    debugPrintf("GPIO %d als Wake-up Source hinzugefügt\n", pin);

    // Wenn Light Sleep bereits aktiv, neu konfigurieren
    if (lightSleepEnabled)
    {
        configureLightSleep();
    }
}

void WiFiWebManager::removeWakeupGPIO(int pin)
{
    for (auto it = wakeupGPIOs.begin(); it != wakeupGPIOs.end(); ++it)
    {
        if (it->pin == pin)
        {
            gpio_wakeup_disable((gpio_num_t)it->pin);
            wakeupGPIOs.erase(it);
            debugPrintf("GPIO %d Wake-up entfernt\n", pin);
            if (lightSleepEnabled)
                configureLightSleep();
            return;
        }
    }
}

void WiFiWebManager::clearAllWakeupGPIOs()
{
    for (const auto &wgpio : wakeupGPIOs)
    {
        gpio_wakeup_disable((gpio_num_t)wgpio.pin);
    }
    wakeupGPIOs.clear();
    debugPrintln("Alle GPIO Wake-ups entfernt");
    if (lightSleepEnabled)
        configureLightSleep();
}

void WiFiWebManager::configureLightSleep()
{
    if (!lightSleepEnabled)
        return;

    debugPrintln("Konfiguriere Light Sleep...");

    // Timer Wake-up (immer aktiv für Library-Tasks)
    esp_sleep_enable_timer_wakeup(lightSleepTimer);
    debugPrintf("Timer Wake-up: %llu µs\n", lightSleepTimer);

    // Vorhandene GPIO Wake-ups zurücksetzen, um verwaiste Quellen zu vermeiden
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_GPIO);
    for (const auto &wgpio : wakeupGPIOs)
    {
        gpio_wakeup_disable((gpio_num_t)wgpio.pin);
    }

    // Interne Wake-ups der Library aktivieren
    enableLibraryWakeups();

    // Benutzer-definierte GPIO Wake-ups
    for (const auto &wgpio : wakeupGPIOs)
    {
        gpio_wakeup_enable((gpio_num_t)wgpio.pin, wgpio.mode);
        debugPrintf("GPIO %d Wake-up aktiviert (Modus: %d)\n", wgpio.pin, wgpio.mode);
    }

    // WiFi Wake-up ist automatisch durch Framework aktiv
    debugPrintln("Light Sleep konfiguriert");
}

void WiFiWebManager::analyzeWakeupCause()
{
    lastWakeupCause = esp_sleep_get_wakeup_cause();
    lastWakeupGPIO = -1;

    if (lastWakeupCause == ESP_SLEEP_WAKEUP_GPIO)
    {
        lastWakeupGPIO = esp_sleep_get_gpio_wakeup_pin();
    }

    // Statistiken aktualisieren
    if (wakeupLoggingEnabled)
    {
        updateWakeupStats();
    }

    // Debug-Output
    debugPrintWakeupCause();
}

void WiFiWebManager::updateWakeupStats()
{
    wakeupStats.totalWakeups++;

    switch (lastWakeupCause)
    {
    case ESP_SLEEP_WAKEUP_TIMER:
        wakeupStats.timerWakeups++;
        break;
    case ESP_SLEEP_WAKEUP_GPIO:
        wakeupStats.gpioWakeups++;
        break;
    case ESP_SLEEP_WAKEUP_WIFI:
        wakeupStats.wifiWakeups++;
        break;
    default:
        wakeupStats.otherWakeups++;
        break;
    }
}

void WiFiWebManager::debugPrintWakeupCause()
{
    if (!debugMode)
        return;

    debugPrint("Wake-up: ");
    debugPrintln(getWakeupCauseString());

    if (lastWakeupGPIO >= 0)
    {
        debugPrintf("GPIO: %d\n", lastWakeupGPIO);
    }
}


esp_sleep_wakeup_cause_t WiFiWebManager::getLastWakeupCause()
{
    return lastWakeupCause;
}

String WiFiWebManager::getWakeupCauseString()
{
    switch (lastWakeupCause)
    {
    case ESP_SLEEP_WAKEUP_UNDEFINED:
        return "Undefined/Power-On";
    case ESP_SLEEP_WAKEUP_ALL:
        return "All";
    case ESP_SLEEP_WAKEUP_EXT0:
        return "External RTC_IO";
    case ESP_SLEEP_WAKEUP_EXT1:
        return "External RTC_CNTL";
    case ESP_SLEEP_WAKEUP_TIMER:
        return "Timer";
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
        return "Touchpad";
    case ESP_SLEEP_WAKEUP_ULP:
        return "ULP";
    case ESP_SLEEP_WAKEUP_GPIO:
        return "GPIO";
    case ESP_SLEEP_WAKEUP_UART:
        return "UART";
    case ESP_SLEEP_WAKEUP_WIFI:
        return "WiFi";
    case ESP_SLEEP_WAKEUP_COCPU:
        return "COCPU";
    case ESP_SLEEP_WAKEUP_COCPU_TRAP_TRIG:
        return "COCPU Trap";
    case ESP_SLEEP_WAKEUP_BT:
        return "Bluetooth";
    default:
        return "Unknown";
    }
}

int WiFiWebManager::getWakeupGPIO()
{
    return lastWakeupGPIO;
}

bool WiFiWebManager::wasWokenByTimer()
{
    return lastWakeupCause == ESP_SLEEP_WAKEUP_TIMER;
}

bool WiFiWebManager::wasWokenByGPIO()
{
    return lastWakeupCause == ESP_SLEEP_WAKEUP_GPIO;
}

bool WiFiWebManager::wasWokenByWiFi()
{
    return lastWakeupCause == ESP_SLEEP_WAKEUP_WIFI;
}

void WiFiWebManager::enableWakeupLogging(bool enabled)
{
    wakeupLoggingEnabled = enabled;
    if (!enabled)
    {
        clearWakeupStats();
    }
}

void WiFiWebManager::clearWakeupStats()
{
    wakeupStats = WakeupStats{};
}

WiFiWebManager::WakeupStats WiFiWebManager::getWakeupStats()
{
    return wakeupStats;
}

void WiFiWebManager::setDebugMode(bool enabled)
{
    debugMode = enabled;
}

bool WiFiWebManager::getDebugMode()
{
    return debugMode;
}

void WiFiWebManager::setDefaultHostname(const String &hostnameValue)
{
    defaultHostname = hostnameValue;
    if (hostname.length() == 0)
    {
        hostname = defaultHostname;
    }
}

String WiFiWebManager::getHostname()
{
    if (hostname.length() > 0)
    {
        return hostname;
    }
    return defaultHostname;
}

void WiFiWebManager::saveCustomData(const String &key, const String &value)
{
    if (isReservedKey(key))
        return;

    Preferences customPrefs;
    customPrefs.begin(CUSTOM_DATA_NAMESPACE, false);
    customPrefs.putString(key.c_str(), value);
    customPrefs.end();
    addKeyToStore(key);
}

void WiFiWebManager::saveCustomData(const String &key, int value)
{
    saveCustomData(key, String(value));
}

void WiFiWebManager::saveCustomData(const String &key, bool value)
{
    saveCustomData(key, value ? String("true") : String("false"));
}

void WiFiWebManager::saveCustomData(const String &key, float value)
{
    saveCustomData(key, String(value, 6));
}

String WiFiWebManager::loadCustomData(const String &key, const String &defaultValue)
{
    Preferences customPrefs;
    customPrefs.begin(CUSTOM_DATA_NAMESPACE, true);
    String value = customPrefs.getString(key.c_str(), defaultValue);
    customPrefs.end();
    return value;
}

int WiFiWebManager::loadCustomDataInt(const String &key, int defaultValue)
{
    return loadCustomData(key, String(defaultValue)).toInt();
}

bool WiFiWebManager::loadCustomDataBool(const String &key, bool defaultValue)
{
    String value = loadCustomData(key, defaultValue ? String("true") : String("false"));
    value.toLowerCase();
    return value == "true" || value == "1" || value == "on";
}

float WiFiWebManager::loadCustomDataFloat(const String &key, float defaultValue)
{
    return loadCustomData(key, String(defaultValue, 6)).toFloat();
}

bool WiFiWebManager::hasCustomData(const String &key)
{
    Preferences customPrefs;
    customPrefs.begin(CUSTOM_DATA_NAMESPACE, true);
    bool result = customPrefs.isKey(key.c_str());
    customPrefs.end();
    return result;
}

void WiFiWebManager::removeCustomData(const String &key)
{
    Preferences customPrefs;
    customPrefs.begin(CUSTOM_DATA_NAMESPACE, false);
    if (customPrefs.isKey(key.c_str()))
    {
        customPrefs.remove(key.c_str());
    }
    customPrefs.end();
    removeKeyFromStore(key);
}

std::vector<String> WiFiWebManager::getCustomDataKeys()
{
    return loadStoredKeys();
}

void WiFiWebManager::addPage(const String &menutitle, const String &path, ContentHandler getHandler, ContentHandler postHandler)
{
    if (path == "/")
    {
        rootGetHandler = getHandler;
        rootPostHandler = postHandler;
        return;
    }

    removePage(path);

    CustomPage page;
    page.title = menutitle;
    page.path = path;
    page.getHandler = getHandler;
    page.postHandler = postHandler;

    page.getWebHandler = &server.on(path.c_str(), HTTP_GET, [this, path, menutitle, getHandler](AsyncWebServerRequest *request) {
        String innerContent;
        if (getHandler)
        {
            innerContent = getHandler(request);
        }
        if (innerContent.length() == 0)
        {
            innerContent = F("<p>Keine Inhalte verfügbar.</p>");
        }
        request->send(200, "text/html", htmlWrap(menutitle, path, innerContent));
    });

    if (postHandler)
    {
        page.postWebHandler = &server.on(path.c_str(), HTTP_POST, [this, path, menutitle, postHandler](AsyncWebServerRequest *request) {
            String response = postHandler(request);
            if (response.length() == 0)
            {
                request->redirect(path);
            }
            else
            {
                request->send(200, "text/html", htmlWrap(menutitle, path, response));
            }
        });
    }

    customPages.push_back(page);
}

void WiFiWebManager::removePage(const String &path)
{
    if (path == "/")
    {
        rootGetHandler = nullptr;
        rootPostHandler = nullptr;
        return;
    }

    for (auto it = customPages.begin(); it != customPages.end(); ++it)
    {
        if (it->path == path)
        {
            if (it->getWebHandler)
            {
                server.removeHandler(it->getWebHandler);
            }
            if (it->postWebHandler)
            {
                server.removeHandler(it->postWebHandler);
            }
            customPages.erase(it);
            break;
        }
    }
}

void WiFiWebManager::setupWebServer()
{
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        String content;

        auto wifiModeToString = [](wifi_mode_t mode) {
            switch (mode)
            {
            case WIFI_OFF:
                return String("OFF");
            case WIFI_STA:
                return String("Station");
            case WIFI_AP:
                return String("Access Point");
            case WIFI_AP_STA:
                return String("AP + STA");
            default:
                return String("Unbekannt");
            }
        };

        wifi_mode_t mode = WiFi.getMode();
        wl_status_t status = WiFi.status();
        String statusText = (status == WL_CONNECTED) ? String("Verbunden") : String("Getrennt");
        String ipText = (status == WL_CONNECTED) ? WiFi.localIP().toString() : (useStaticIP ? ip : String("DHCP"));
        String gwText = (status == WL_CONNECTED) ? WiFi.gatewayIP().toString() : (useStaticIP ? gateway : String("-"));
        String subnetText = (status == WL_CONNECTED) ? WiFi.subnetMask().toString() : (useStaticIP ? subnet : String("-"));
        String dnsText = (status == WL_CONNECTED) ? WiFi.dnsIP().toString() : (useStaticIP ? dns : String("-"));

        content += F("<section class='card'><h2>WLAN-Status</h2>");
        content += "<p><strong>Modus:</strong> " + htmlEscape(wifiModeToString(mode)) + "</p>";
        content += "<p><strong>Status:</strong> " + htmlEscape(statusText) + "</p>";
        if (status == WL_CONNECTED)
        {
            content += "<p><strong>Verbunden mit:</strong> " + htmlEscape(WiFi.SSID()) + "</p>";
            content += "<p><strong>Signal:</strong> " + String(WiFi.RSSI()) + " dBm</p>";
        }
        else if (ssid.length() > 0)
        {
            content += "<p><strong>Gespeichertes WLAN:</strong> " + htmlEscape(ssid) + "</p>";
        }
        content += "<p><strong>Hostname:</strong> " + htmlEscape(getHostname()) + "</p>";
        content += "<p><strong>IP:</strong> " + htmlEscape(ipText) + "</p>";
        content += "<p><strong>Gateway:</strong> " + htmlEscape(gwText) + "</p>";
        content += "<p><strong>Subnetz:</strong> " + htmlEscape(subnetText) + "</p>";
        content += "<p><strong>DNS:</strong> " + htmlEscape(dnsText) + "</p>";
        content += "</section>";

        String wifiOptions = getAvailableSSIDs();

        content += F("<section class='card'><h2>WLAN konfigurieren</h2>");
        content += F("<form method='POST' class='form-grid'>");
        content += F("<input type='hidden' name='action' value='wifi'>");
        content += "<label>SSID<input type='text' name='ssid' list='ssid-list' value='" + htmlEscape(ssid) + "'></label>";
        content += "<datalist id='ssid-list'>" + wifiOptions + "</datalist>";
        content += "<label>Passwort<input type='password' name='password' value='" + htmlEscape(password) + "' autocomplete='off'></label>";
        content += "<label>Hostname<input type='text' name='hostname' value='" + htmlEscape(getHostname()) + "'></label>";
        content += "<label class='checkbox'><input type='checkbox' name='useStaticIP' " + String(useStaticIP ? "checked" : "") + "> Statische IP verwenden</label>";
        content += "<label>IP-Adresse<input type='text' name='ip' value='" + htmlEscape(ip) + "'></label>";
        content += "<label>Gateway<input type='text' name='gateway' value='" + htmlEscape(gateway) + "'></label>";
        content += "<label>Subnetz<input type='text' name='subnet' value='" + htmlEscape(subnet) + "'></label>";
        content += "<label>DNS<input type='text' name='dns' value='" + htmlEscape(dns) + "'></label>";
        content += F("<button type='submit'>Speichern &amp; Neustarten</button>");
        content += F("</form></section>");

        content += F("<section class='card'><h2>NTP Einstellungen</h2>");
        content += F("<form method='POST' class='form-grid'>");
        content += F("<input type='hidden' name='action' value='ntp'>");
        content += "<label class='checkbox'><input type='checkbox' name='ntpEnable' " + String(ntpEnable ? "checked" : "") + "> NTP aktivieren</label>";
        content += "<label>NTP Server<input type='text' name='ntpServer' value='" + htmlEscape(ntpServer) + "'></label>";
        content += F("<button type='submit'>Übernehmen</button>");
        content += F("</form></section>");

        content += F("<section class='card'><h2>Light Sleep</h2>");
        content += F("<form method='POST' class='form-grid'>");
        content += F("<input type='hidden' name='action' value='sleep'>");
        content += "<label class='checkbox'><input type='checkbox' name='lightSleep' " + String(lightSleepEnabled ? "checked" : "") + "> Light Sleep aktivieren</label>";
        content += "<label>Wakeup Timer (ms)<input type='number' name='sleepTimer' min='10' max='3600000' value='" + String(lightSleepTimer / 1000) + "'></label>";
        content += F("<button type='submit'>Einstellungen anwenden</button>");
        content += F("</form></section>");

        WakeupStats stats = getWakeupStats();
        content += F("<section class='card'><h2>Wake-up Informationen</h2>");
        content += "<p><strong>Letzter Wake-up:</strong> " + htmlEscape(getWakeupCauseString()) + "</p>";
        if (lastWakeupGPIO >= 0)
        {
            content += "<p><strong>GPIO:</strong> " + String(lastWakeupGPIO) + "</p>";
        }
        content += "<p><strong>Wake-up Logging:</strong> " + String(wakeupLoggingEnabled ? "aktiv" : "inaktiv") + "</p>";
        content += "<ul class='stats'>";
        content += "<li>Timer: " + String(stats.timerWakeups) + "</li>";
        content += "<li>GPIO: " + String(stats.gpioWakeups) + "</li>";
        content += "<li>WiFi: " + String(stats.wifiWakeups) + "</li>";
        content += "<li>Andere: " + String(stats.otherWakeups) + "</li>";
        content += "<li>Gesamt: " + String(stats.totalWakeups) + "</li>";
        content += "</ul>";
        content += F("<form method='POST' class='inline-form'>");
        content += F("<input type='hidden' name='action' value='logging'>");
        content += "<label class='checkbox'><input type='checkbox' name='wakeupLogging' " + String(wakeupLoggingEnabled ? "checked" : "") + "> Logging aktivieren</label>";
        content += F("<button type='submit'>Speichern</button>");
        content += F("</form>");
        content += F("<form method='POST' class='inline-form'>");
        content += F("<input type='hidden' name='action' value='clearStats'>");
        content += F("<button type='submit' class='secondary'>Statistik zurücksetzen</button>");
        content += F("</form></section>");

        auto keys = getCustomDataKeys();
        content += F("<section class='card'><h2>Custom Data</h2>");
        if (keys.empty())
        {
            content += F("<p>Keine Custom Data gespeichert.</p>");
        }
        else
        {
            content += F("<table class='data-table'><thead><tr><th>Key</th><th>Value</th></tr></thead><tbody>");
            for (const auto &key : keys)
            {
                content += "<tr><td>" + htmlEscape(key) + "</td><td>" + htmlEscape(loadCustomData(key, "")) + "</td></tr>";
            }
            content += F("</tbody></table>");
        }
        content += F("</section>");

        content += F("<section class='card'><h2>Verwaltung</h2>");
        content += F("<form method='POST' class='inline-form'>");
        content += F("<input type='hidden' name='action' value='reboot'>");
        content += F("<button type='submit'>Gerät neu starten</button>");
        content += F("</form>");
        content += F("<form method='POST' class='inline-form'>");
        content += F("<input type='hidden' name='action' value='clearWifi'>");
        content += F("<button type='submit' class='warning'>WLAN Daten löschen</button>");
        content += F("</form>");
        content += F("<form method='POST' class='inline-form'>");
        content += F("<input type='hidden' name='action' value='factoryReset'>");
        content += F("<button type='submit' class='danger' onclick="return confirm('Alle Einstellungen wirklich löschen?');">Werksreset</button>");
        content += F("</form></section>");

        if (rootGetHandler)
        {
            String extra = rootGetHandler(request);
            if (extra.length() > 0)
            {
                content += extra;
            }
        }

        request->send(200, "text/html", htmlWrap("Übersicht", "/", content));
    });

    server.on("/", HTTP_POST, [this](AsyncWebServerRequest *request) {
        String action;
        if (request->hasParam("action", true))
        {
            action = request->getParam("action", true)->value();
        }

        bool persistConfig = false;

        if (action == "wifi")
        {
            String newSsid = request->hasParam("ssid", true) ? request->getParam("ssid", true)->value() : ssid;
            String newPassword = request->hasParam("password", true) ? request->getParam("password", true)->value() : password;
            String newHostname = request->hasParam("hostname", true) ? request->getParam("hostname", true)->value() : hostname;
            bool newStatic = request->hasParam("useStaticIP", true);
            String newIp = request->hasParam("ip", true) ? request->getParam("ip", true)->value() : ip;
            String newGateway = request->hasParam("gateway", true) ? request->getParam("gateway", true)->value() : gateway;
            String newSubnet = request->hasParam("subnet", true) ? request->getParam("subnet", true)->value() : subnet;
            String newDns = request->hasParam("dns", true) ? request->getParam("dns", true)->value() : dns;

            ssid = newSsid;
            password = newPassword;
            hostname = newHostname;
            useStaticIP = newStatic;
            ip = newIp;
            gateway = newGateway;
            subnet = newSubnet;
            dns = newDns;

            persistConfig = true;
            shouldReboot = true;
            resetBootAttempts();
        }
        else if (action == "ntp")
        {
            ntpEnable = request->hasParam("ntpEnable", true);
            ntpServer = request->hasParam("ntpServer", true) ? request->getParam("ntpServer", true)->value() : ntpServer;
            handleNTP();
            persistConfig = true;
        }
        else if (action == "sleep")
        {
            lightSleepEnabled = request->hasParam("lightSleep", true);
            uint64_t timerMs = lightSleepTimer / 1000;
            if (request->hasParam("sleepTimer", true))
            {
                timerMs = request->getParam("sleepTimer", true)->value().toInt();
                if (timerMs < 10)
                {
                    timerMs = 10;
                }
            }
            lightSleepTimer = timerMs * 1000ULL;
            if (lightSleepEnabled)
            {
                configureLightSleep();
            }
            persistConfig = true;
        }
        else if (action == "logging")
        {
            wakeupLoggingEnabled = request->hasParam("wakeupLogging", true);
            persistConfig = true;
        }
        else if (action == "clearStats")
        {
            clearWakeupStats();
        }
        else if (action == "clearWifi")
        {
            clearWiFiConfig();
            shouldReboot = true;
        }
        else if (action == "factoryReset")
        {
            clearAllConfig();
            shouldReboot = true;
        }
        else if (action == "reboot")
        {
            shouldReboot = true;
        }

        if (persistConfig)
        {
            saveConfig();
        }

        if (rootPostHandler)
        {
            rootPostHandler(request);
        }

        request->redirect("/");
    });

    server.onNotFound([this](AsyncWebServerRequest *request) {
        request->send(404, "text/html", htmlWrap("Nicht gefunden", request->url(), F("<p>Die angeforderte Seite wurde nicht gefunden.</p>")));
    });

    for (auto &page : customPages)
    {
        if (!page.getWebHandler && page.getHandler)
        {
            page.getWebHandler = &server.on(page.path.c_str(), HTTP_GET, [this, path = page.path, title = page.title, handler = page.getHandler](AsyncWebServerRequest *request) {
                String inner = handler ? handler(request) : String();
                if (inner.length() == 0)
                {
                    inner = F("<p>Keine Inhalte verfügbar.</p>");
                }
                request->send(200, "text/html", htmlWrap(title, path, inner));
            });
        }
        if (!page.postWebHandler && page.postHandler)
        {
            page.postWebHandler = &server.on(page.path.c_str(), HTTP_POST, [this, path = page.path, title = page.title, handler = page.postHandler](AsyncWebServerRequest *request) {
                String resp = handler ? handler(request) : String();
                if (resp.length() == 0)
                {
                    request->redirect(path);
                }
                else
                {
                    request->send(200, "text/html", htmlWrap(title, path, resp));
                }
            });
        }
    }

    server.begin();
    debugPrintln("WebServer gestartet.");
}

bool WiFiWebManager::isReservedKey(const String &key)
{
    static const char *const reserved[] = {"ssid", "pwd", "hostname", "ip", "gateway", "subnet", "dns", "useStaticIP", "ntpEnable", "ntpServer", "bootAttempts", "lightSleep", "lightSleepTimer", "wakeupLogging"};
    for (auto entry : reserved)
    {
        if (key.equalsIgnoreCase(entry))
        {
            return true;
        }
    }
    return key.startsWith("__");
}

void WiFiWebManager::enableLibraryWakeups()
{
    esp_sleep_enable_gpio_wakeup();
    gpio_wakeup_enable(GPIO_NUM_0, GPIO_INTR_LOW_LEVEL);
    debugPrintln("Reset-Button Wake-up aktiviert");
}

void WiFiWebManager::debugPrint(const String &message)
{
    if (!debugMode)
        return;
    Serial.print(message);
}

void WiFiWebManager::debugPrintln(const String &message)
{
    if (!debugMode)
        return;
    Serial.println(message);
}

void WiFiWebManager::debugPrintln()
{
    if (!debugMode)
        return;
    Serial.println();
}

void WiFiWebManager::debugPrintf(const char *format, ...)
{
    if (!debugMode || format == nullptr)
        return;

    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    Serial.print(buffer);
}

String WiFiWebManager::renderMenu(const String &currentPath)
{
    String menu = F("<nav class='menu'>");
    menu += "<a href='/'";
    if (currentPath == "/")
    {
        menu += F(" class='active'");
    }
    menu += F(">Übersicht</a>");

    for (const auto &page : customPages)
    {
        menu += "<a href='" + page.path + "'";
        if (page.path == currentPath)
        {
            menu += F(" class='active'");
        }
        menu += "'>" + htmlEscape(page.title) + "</a>";
    }

    menu += F("</nav>");
    return menu;
}

String WiFiWebManager::htmlWrap(const String &menutitle, const String &currentPath, const String &content)
{
    String html;
    html.reserve(2048 + content.length());
    html += F("<!DOCTYPE html><html lang='de'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>");
    html += "<title>WiFiWebManager - " + htmlEscape(menutitle) + "</title>";
    html += F("<style>");
    html += F("body{font-family:Arial,Helvetica,sans-serif;margin:0;padding:0;background:#f2f4f8;color:#1c1e21;}");
    html += F("header.hero{background:#1f6feb;color:#fff;padding:1.5rem;text-align:center;}");
    html += F("header.hero h1{margin:0;font-size:1.8rem;}");
    html += F("main{padding:1rem;display:flex;flex-direction:column;gap:1rem;}");
    html += F(".menu{display:flex;gap:.5rem;background:#0d1117;padding:.5rem 1rem;}");
    html += F(".menu a{color:#8ea2ff;text-decoration:none;padding:.4rem .8rem;border-radius:.4rem;}");
    html += F(".menu a.active,.menu a:hover{background:#1f6feb;color:#fff;}");
    html += F(".card{background:#fff;border-radius:.8rem;padding:1rem;box-shadow:0 2px 6px rgba(0,0,0,0.08);}");
    html += F(".form-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:.8rem;}");
    html += F(".form-grid label{display:flex;flex-direction:column;font-weight:600;gap:.3rem;font-size:.9rem;}");
    html += F("input[type='text'],input[type='password'],input[type='number'],select{padding:.6rem;border:1px solid #cbd5e1;border-radius:.4rem;font-size:1rem;}");
    html += F("button{padding:.6rem 1rem;border:none;border-radius:.4rem;background:#1f6feb;color:#fff;font-size:1rem;cursor:pointer;}");
    html += F("button.secondary{background:#64748b;}");
    html += F("button.warning{background:#f59e0b;}");
    html += F("button.danger{background:#dc2626;}");
    html += F(".checkbox{display:flex;align-items:center;gap:.5rem;font-weight:600;}");
    html += F(".stats{display:flex;flex-wrap:wrap;gap:1rem;padding:0;margin:.5rem 0 1rem 0;list-style:none;}");
    html += F(".stats li{background:#eef2ff;padding:.5rem .8rem;border-radius:.4rem;}");
    html += F(".inline-form{display:inline-block;margin-right:.5rem;}");
    html += F(".data-table{width:100%;border-collapse:collapse;margin-top:.5rem;}");
    html += F(".data-table th,.data-table td{border:1px solid #d1d5db;padding:.5rem;text-align:left;}");
    html += F(".footer{text-align:center;padding:1rem;color:#64748b;font-size:.85rem;}");
    html += F("@media(max-width:768px){.menu{flex-wrap:wrap;}.form-grid{grid-template-columns:1fr;}}");
    html += F("</style></head><body>");
    html += F("<header class='hero'><h1>WiFiWebManager</h1><p>Version ");
    html += WIFIWEBMANAGER_VERSION_STRING;
    html += F("</p></header>");
    html += renderMenu(currentPath);
    html += "<main>" + content + "</main>";
    html += F("<footer class='footer'>&copy; WiFiWebManager</footer></body></html>");
    return html;
}
