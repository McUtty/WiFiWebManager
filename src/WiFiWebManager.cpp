#include "WiFiWebManager.h"
#include "WiFiWebManagerFavicon.h"

// Namespace und Meta-Key für Custom Data (zentral definiert, damit
// Speichern/Laden und clearAllConfig garantiert denselben Namespace nutzen)
const char* WiFiWebManager::CUSTOM_NS    = "cdata";
const char* WiFiWebManager::CUSTOM_INDEX = "__idx";

WiFiWebManager::WiFiWebManager() {
    // Reset-Button Pin als Input mit Pull-up konfigurieren
    pinMode(RESET_PIN, INPUT_PULLUP);
    // Mutex für den geteilten Scan-Options-Puffer
    scanMutex = xSemaphoreCreateMutex();
}

void WiFiWebManager::begin() {
    debugPrintln("\n=== Starte WiFiWebManager ===");
    loadConfig();

    // Prüfe Boot-Attempts und entscheide Verbindungsstrategie
    if (wifiBootAttempts >= MAX_BOOT_ATTEMPTS) {
        debugPrintf("Maximale Boot-Versuche erreicht (%d), starte AP-Modus\n", wifiBootAttempts);
        startAP();
    } else if (ssid.length() > 0 && password.length() > 0) {
        incrementBootAttempts();
        debugPrintf("WLAN-Verbindungsversuch %d/%d\n", wifiBootAttempts, MAX_BOOT_ATTEMPTS);
        
        if (connectToStoredWiFi()) {
            debugPrintln("WLAN verbunden!");
            debugPrint("IP-Adresse: ");
            debugPrintln(WiFi.localIP().toString());
            resetBootAttempts(); // Erfolgreiche Verbindung - Counter zurücksetzen
        } else {
            debugPrintln("WLAN-Verbindung fehlgeschlagen");
            if (wifiBootAttempts < MAX_BOOT_ATTEMPTS) {
                debugPrintf("Neustart für Versuch %d/%d...\n", wifiBootAttempts + 1, MAX_BOOT_ATTEMPTS);
                delay(1000);
                ESP.restart();
            } else {
                debugPrintln("Alle Versuche fehlgeschlagen, starte AP-Modus");
                startAP();
            }
        }
    } else {
        debugPrintln("Keine WLAN-Daten gespeichert, starte AP-Modus");
        startAP();
    }
    
    handleNTP();
    setupWebServer();
    ArduinoOTA.begin();

    ledStarted = true;
    statusLedBegin();   // no-op, solange die LED nicht aktiviert wurde
}

void WiFiWebManager::loop() {
    if (shouldReboot) {
        debugPrintln("Reboot...");
        delay(500);
        ESP.restart();
    }
    
    handleResetButton();
    ArduinoOTA.handle();
    statusLedUpdate();   // no-op, solange die LED nicht aktiviert wurde

    // WLAN-Scan bei Bedarf (Web-Handler hat scanRequested gesetzt) oder im
    // AP-Modus periodisch ausführen. Läuft bewusst im loop()-Task – der Scan
    // darf hier kurz blockieren, aber NICHT in der AsyncTCP-Task des Servers.
    bool apish = (WiFi.getMode() != WIFI_STA);
    if (scanRequested || (apish && millis() - lastScanMs > 20000)) {
        updateScanCache();
    }

    // Überwachung der WLAN-Verbindung (alle 30 Sekunden)
    static unsigned long lastWiFiCheck = 0;
    if (millis() - lastWiFiCheck > 30000) {
        lastWiFiCheck = millis();

        if (WiFi.getMode() == WIFI_STA && WiFi.status() != WL_CONNECTED) {
            debugPrintln("WLAN-Verbindung verloren, versuche Reconnect...");
            // Nur versuchen wenn WLAN-Daten vorhanden sind
            if (ssid.length() > 0 && connectToStoredWiFi()) {
                debugPrintln("Reconnect erfolgreich!");
            } else {
                debugPrintln("Reconnect fehlgeschlagen oder keine WLAN-Daten vorhanden");
            }
        } else if (WiFi.getMode() != WIFI_STA && ssid.length() > 0) {
            // AP/AP_STA-Modus: Wenn der letzte (im loop-Task ausgeführte) Scan
            // das gespeicherte Netz zeigt, Wechsel zu STA versuchen. Bei
            // Fehlschlag wird der AP wiederhergestellt.
            if (storedSsidInRange) {
                debugPrintln("Gespeichertes WLAN in Reichweite - versuche Wechsel zu STA...");
                if (connectToStoredWiFi()) {
                    debugPrintln("Wechsel zu STA-Modus erfolgreich!");
                    resetBootAttempts();
                } else {
                    debugPrintln("Verbindung fehlgeschlagen - AP wird wiederhergestellt.");
                    startAP();
                }
            }
        }
    }
}

// Führt einen (synchronen) WLAN-Scan im loop()-Task aus und baut daraus die
// <option>-Liste für die WLAN-Seite. Ergebnis wird gepuffert; der Web-Handler
// liest nur diesen Puffer und blockiert dadurch nie.
void WiFiWebManager::updateScanCache() {
    scanRequested = false;
    int n = WiFi.scanNetworks();  // synchron – blockiert nur loop(), nicht den Server
    String opts = "";
    bool inRange = false;
    for (int i = 0; i < n; ++i) {
        String s = WiFi.SSID(i);
        if (s == ssid) inRange = true;
        String sel = (ssid == s) ? " selected" : "";
        String cls = (ssid == s) ? " class='stored-network'" : "";
        opts += "<option value='" + s + "'" + sel + cls + ">" + s;
        if (ssid == s) opts += " (gespeichert)";
        opts += "</option>";
    }
    WiFi.scanDelete();

    // Fallback: gespeichertes Netz immer wählbar halten, auch bei leerem Scan
    if (opts.length() == 0 && ssid.length() > 0) {
        opts = "<option value='" + ssid + "' selected class='stored-network'>" + ssid + " (gespeichert)</option>";
    }

    if (scanMutex) xSemaphoreTake(scanMutex, portMAX_DELAY);
    cachedScanOptions = opts;
    if (scanMutex) xSemaphoreGive(scanMutex);

    storedSsidInRange = inRange;
    lastScanMs = millis();
}

void WiFiWebManager::handleResetButton() {
    bool currentState = digitalRead(RESET_PIN) == LOW; // Active LOW
    
    if (currentState && !lastResetButtonState) {
        // Button wurde gedrückt
        resetButtonPressed = millis();
        resetButtonState = true;
        debugPrintln("Reset-Button gedrückt...");
    } else if (!currentState && lastResetButtonState) {
        // Button wurde losgelassen
        unsigned long pressTime = millis() - resetButtonPressed;
        
        if (pressTime >= WIFI_RESET_TIME && pressTime < FULL_RESET_TIME) {
            debugPrintln("Reset-Button 3-10 Sekunden gedrückt - Lösche WLAN-Daten!");
            clearWiFiConfig();
            debugPrintln("WLAN-Reset durchgeführt - Neustart...");
            delay(1000);
            shouldReboot = true;
        } else if (pressTime >= FULL_RESET_TIME) {
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

void WiFiWebManager::loadConfig() {
    prefs.begin("netcfg", true);
    
    ssid = prefs.getString("ssid", "");
    password = prefs.getString("pwd", "");
    hostname = prefs.getString("hostname", "");
    
    // Wenn kein Hostname gesetzt und Default vorhanden, verwende Default
    if (hostname.length() == 0 && defaultHostname.length() > 0) {
        hostname = defaultHostname;
    }
    
    useStaticIP = prefs.getBool("useStaticIP", false);
    ip = prefs.getString("ip", "");
    gateway = prefs.getString("gateway", "");
    subnet = prefs.getString("subnet", "");
    dns = prefs.getString("dns", "");
    ntpEnable = prefs.getBool("ntpEnable", false);
    ntpServer = prefs.getString("ntpServer", "pool.ntp.org");
    wifiBootAttempts = prefs.getInt("bootAttempts", 0);
    
    prefs.end();
    debugPrintln("Konfiguration geladen.");
    
    if (ssid.length() > 0) {
        debugPrintf("Gespeichertes WLAN: %s\n", ssid.c_str());
    }
    debugPrintf("Boot-Versuche: %d\n", wifiBootAttempts);
}

void WiFiWebManager::saveConfig() {
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
    prefs.putInt("bootAttempts", wifiBootAttempts);
    prefs.end();
    debugPrintln("Konfiguration gespeichert.");
}

void WiFiWebManager::saveNtpConfig(bool ntpEn, const String& ntpSrv) {
    prefs.begin("netcfg", false);
    prefs.putBool("ntpEnable", ntpEn);
    prefs.putString("ntpServer", ntpSrv);
    prefs.end();
    ntpEnable = ntpEn;
    ntpServer = ntpSrv;
    handleNTP();
}

void WiFiWebManager::clearWiFiConfig() {
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

void WiFiWebManager::clearAllConfig() {
    // Zuerst alle lokalen Variablen zurücksetzen
    ssid = "";
    password = "";
    hostname = "";
    useStaticIP = false;
    ip = "";
    gateway = "";
    subnet = "";
    dns = "";
    ntpEnable = false;
    ntpServer = "pool.ntp.org";
    wifiBootAttempts = 0;
    
    // Dann alle Preferences löschen
    prefs.begin("netcfg", false);
    prefs.clear();
    prefs.end();
    
    // Custom Data auch löschen (inkl. Key-Registry __idx)
    prefs.begin(CUSTOM_NS, false);
    prefs.clear();
    prefs.end();
    
    debugPrintln("Alle Einstellungen gelöscht!");
}

void WiFiWebManager::reset() {
    clearAllConfig();
    shouldReboot = true;
}

void WiFiWebManager::resetBootAttempts() {
    wifiBootAttempts = 0;
    prefs.begin("netcfg", false);
    prefs.putInt("bootAttempts", 0);
    prefs.end();
}

void WiFiWebManager::incrementBootAttempts() {
    wifiBootAttempts++;
    prefs.begin("netcfg", false);
    prefs.putInt("bootAttempts", wifiBootAttempts);
    prefs.end();
}

bool WiFiWebManager::connectToStoredWiFi() {
    if (ssid.length() == 0) return false;
    
    WiFi.mode(WIFI_STA);

    if (hostname.length() > 0) {
        WiFi.setHostname(hostname.c_str());
        debugPrint("Setze Hostname auf: "); debugPrintln(hostname);
    }

    if (useStaticIP) {
        IPAddress ip_, gateway_, subnet_, dns_;
        if (parseIPString(ip, ip_) && parseIPString(gateway, gateway_) &&
            parseIPString(subnet, subnet_) && parseIPString(dns, dns_)) {
            WiFi.config(ip_, gateway_, subnet_, dns_);
            debugPrintln("Statische IP-Konfiguration gesetzt.");
        } else {
            debugPrintln("Warnung: Ungültige statische IP-Konfiguration!");
        }
    }

    debugPrintf("Verbinde mit WLAN: %s\n", ssid.c_str());
    WiFi.begin(ssid.c_str(), password.c_str());
    
    // 5 Sekunden Timeout
    for (int i = 0; i < 10 && WiFi.status() != WL_CONNECTED; i++) {
        delay(500);
        debugPrint(".");
    }
    debugPrintln("");
    
    if (WiFi.status() == WL_CONNECTED) {
        debugPrintf("Verbunden mit: %s\n", ssid.c_str());
        return true;
    } else {
        debugPrintf("Verbindung zu %s fehlgeschlagen\n", ssid.c_str());
        WiFi.disconnect();
        return false;
    }
}

void WiFiWebManager::startAP() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP32_SETUP");
    debugPrintln("Access Point gestartet: ESP32_SETUP");
    debugPrintln("AP-IP: 192.168.4.1");
    // Ersten Netzwerk-Scan anfordern, damit die WLAN-Seite schnell eine Liste hat
    scanRequested = true;
}

String WiFiWebManager::getAvailableSSIDs() {
    // Läuft in der AsyncTCP-Task: NICHT scannen (das würde den Server blockieren),
    // sondern nur den im loop()-Task gefüllten Puffer auslesen und einen frischen
    // Scan anfordern.
    scanRequested = true;

    String opts;
    if (scanMutex) xSemaphoreTake(scanMutex, portMAX_DELAY);
    opts = cachedScanOptions;
    if (scanMutex) xSemaphoreGive(scanMutex);

    if (opts.length() == 0) {
        // Noch kein Scan-Ergebnis vorhanden
        if (ssid.length() > 0) {
            opts = "<option value='" + ssid + "' selected class='stored-network'>" + ssid + " (gespeichert)</option>";
        }
        opts += "<option value=''>Suche läuft… Seite bitte neu laden</option>";
    }
    return opts;
}

bool WiFiWebManager::parseIPString(const String& str, IPAddress& out) {
    int parts[4];
    if (sscanf(str.c_str(), "%d.%d.%d.%d", &parts[0], &parts[1], &parts[2], &parts[3]) == 4) {
        out = IPAddress(parts[0], parts[1], parts[2], parts[3]);
        return true;
    }
    return false;
}

void WiFiWebManager::handleNTP() {
    if (ntpEnable) {
        configTime(0, 0, ntpServer.c_str());
        debugPrint("NTP aktiviert, Server: ");
        debugPrintln(ntpServer);
    }
}

// Debug-Modus Management
void WiFiWebManager::setDebugMode(bool enabled) {
    debugMode = enabled;
}

bool WiFiWebManager::getDebugMode() {
    return debugMode;
}

// ---------------------------------------------------------------------------
// WLAN-Status-LED (WS2812) — optional, generisch. Ansteuerung über die
// Core-Funktion neopixelWrite(); keine zusätzliche Bibliothek.
// ---------------------------------------------------------------------------
void WiFiWebManager::enableStatusLed(uint8_t pin, uint8_t brightness) {
    ledEnabled    = true;
    ledPin        = pin;
    ledBrightness = brightness;
    if (ledStarted) statusLedBegin();   // zur Laufzeit sofort initialisieren
}

void WiFiWebManager::disableStatusLed() {
    if (ledEnabled && ledStarted) statusLedApply(0, 0, 0);  // LED ausschalten
    ledEnabled = false;
}

void WiFiWebManager::setStatusLedPin(uint8_t pin) {
    ledPin = pin;
    if (ledEnabled && ledStarted) statusLedBegin();
}

void WiFiWebManager::setStatusLedBrightness(uint8_t brightness) {
    ledBrightness = brightness;
    ledForce = true;   // aktuelle Farbe mit neuer Helligkeit neu ausgeben
}

void WiFiWebManager::setStatusLedRssiThreshold(int dbm) {
    ledRssiGoodDbm = dbm;
    ledForce = true;
}

void WiFiWebManager::setStatusLedSelfTest(bool enabled) {
    ledSelfTest = enabled;
}

void WiFiWebManager::statusLedBegin() {
    if (!ledEnabled) return;
    debugPrintf("StatusLed: aktiviert an GPIO %u (Helligkeit %u)\n", ledPin, ledBrightness);

    // Optionaler, sichtbarer Selbsttest (blockierend — nur hier, nicht im loop()):
    // rot -> grün -> blau -> aus. Leuchtet nichts, sitzt die LED nicht an diesem
    // Pin bzw. bekommt kein Signal.
    if (ledSelfTest) {
        statusLedApply(ledBrightness, 0, 0); delay(300);
        statusLedApply(0, ledBrightness, 0); delay(300);
        statusLedApply(0, 0, ledBrightness); delay(300);
        statusLedApply(0, 0, 0);             delay(150);
    }

    ledMode    = LedMode::Lost;
    ledBlinkOn = false;
    ledForce   = false;
    ledLastMs  = 0;
    statusLedApply(ledBrightness, 0, 0);   // Startzustand: rot
}

void WiFiWebManager::statusLedUpdate() {
    if (!ledEnabled) return;

    const unsigned long now = millis();
    if (!ledForce && now - ledLastMs < LED_UPDATE_MS) return;
    ledLastMs = now;

    // Zielzustand aus WLAN-Status/-Modus ableiten.
    LedMode target;
    if (WiFi.status() == WL_CONNECTED) {
        target = (WiFi.RSSI() >= ledRssiGoodDbm) ? LedMode::Connected : LedMode::Weak;
    } else if (WiFi.getMode() == WIFI_AP) {
        target = LedMode::AccessPoint;   // Setup-Portal -> rot blinkend
    } else {
        target = LedMode::Lost;          // STA verbindet (noch) nicht -> rot
    }

    // AP-Modus: rot blinkend — bei jedem Tick die Phase umschalten.
    if (target == LedMode::AccessPoint) {
        if (ledMode != LedMode::AccessPoint) {
            debugPrintln("StatusLed: AP (rot blinkend)");
            ledMode    = LedMode::AccessPoint;
            ledBlinkOn = true;
        } else {
            ledBlinkOn = !ledBlinkOn;
        }
        statusLedApply(ledBlinkOn ? ledBrightness : 0, 0, 0);
        ledForce = false;
        return;
    }

    // Statische Zustände: nur bei Wechsel (oder erzwungen) schreiben.
    if (target != ledMode || ledForce) {
        ledMode = target;
        switch (target) {
            case LedMode::Connected:
                debugPrintln("StatusLed: gruen");
                statusLedApply(0, ledBrightness, 0);
                break;
            case LedMode::Weak:
                debugPrintln("StatusLed: gelb");
                statusLedApply(ledBrightness, ledBrightness, 0);
                break;
            default:
                debugPrintln("StatusLed: rot");
                statusLedApply(ledBrightness, 0, 0);
                break;
        }
    }
    ledForce = false;
}

void WiFiWebManager::statusLedApply(uint8_t r, uint8_t g, uint8_t b) {
    neopixelWrite(ledPin, r, g, b);
}

// Debug-Hilfsfunktionen
void WiFiWebManager::debugPrint(const String& message) {
    if (debugMode) {
        Serial.print(message);
    }
}

void WiFiWebManager::debugPrintln(const String& message) {
    if (debugMode) {
        Serial.println(message);
    }
}

void WiFiWebManager::debugPrintln() {
    if (debugMode) {
        Serial.println();
    }
}

void WiFiWebManager::debugPrintf(const char* format, ...) {
    if (debugMode) {
        va_list args;
        va_start(args, format);
        char buf[256];
        vsnprintf(buf, sizeof(buf), format, args);
        va_end(args);
        Serial.print(buf);
    }
}

// Hostname-Management
void WiFiWebManager::setDefaultHostname(const String& hostname) {
    defaultHostname = hostname;
    // Wenn aktueller Hostname leer ist, verwende den Default
    if (this->hostname.length() == 0) {
        this->hostname = hostname;
    }
}

String WiFiWebManager::getHostname() {
    return hostname.length() > 0 ? hostname : defaultHostname;
}

void WiFiWebManager::setFirmwareVersion(const String& version) {
    firmwareVersion = version;
}

// Erweiterte Custom Data API
void WiFiWebManager::saveCustomData(const String& key, const String& value) {
    if (!isValidCustomKey(key)) return;

    prefs.begin(CUSTOM_NS, false);
    prefs.putString(("c_" + key).c_str(), value);
    prefs.end();
    registerCustomKey(key);
}

void WiFiWebManager::saveCustomData(const String& key, int value) {
    if (!isValidCustomKey(key)) return;

    prefs.begin(CUSTOM_NS, false);
    prefs.putInt(("c_" + key).c_str(), value);
    prefs.end();
    registerCustomKey(key);
}

void WiFiWebManager::saveCustomData(const String& key, bool value) {
    if (!isValidCustomKey(key)) return;

    prefs.begin(CUSTOM_NS, false);
    prefs.putBool(("c_" + key).c_str(), value);
    prefs.end();
    registerCustomKey(key);
}

void WiFiWebManager::saveCustomData(const String& key, float value) {
    if (!isValidCustomKey(key)) return;

    prefs.begin(CUSTOM_NS, false);
    prefs.putFloat(("c_" + key).c_str(), value);
    prefs.end();
    registerCustomKey(key);
}

String WiFiWebManager::loadCustomData(const String& key, const String& defaultValue) {
    prefs.begin(CUSTOM_NS, true);
    String v = prefs.getString(("c_" + key).c_str(), defaultValue);
    prefs.end();
    return v;
}

int WiFiWebManager::loadCustomDataInt(const String& key, int defaultValue) {
    prefs.begin(CUSTOM_NS, true);
    int v = prefs.getInt(("c_" + key).c_str(), defaultValue);
    prefs.end();
    return v;
}

bool WiFiWebManager::loadCustomDataBool(const String& key, bool defaultValue) {
    prefs.begin(CUSTOM_NS, true);
    bool v = prefs.getBool(("c_" + key).c_str(), defaultValue);
    prefs.end();
    return v;
}

float WiFiWebManager::loadCustomDataFloat(const String& key, float defaultValue) {
    prefs.begin(CUSTOM_NS, true);
    float v = prefs.getFloat(("c_" + key).c_str(), defaultValue);
    prefs.end();
    return v;
}

bool WiFiWebManager::hasCustomData(const String& key) {
    prefs.begin(CUSTOM_NS, true);
    bool exists = prefs.isKey(("c_" + key).c_str());
    prefs.end();
    return exists;
}

void WiFiWebManager::removeCustomData(const String& key) {
    prefs.begin(CUSTOM_NS, false);
    prefs.remove(("c_" + key).c_str());
    prefs.end();
    unregisterCustomKey(key);
}

std::vector<String> WiFiWebManager::getCustomDataKeys() {
    std::vector<String> keys;

    prefs.begin(CUSTOM_NS, true);
    String idx = prefs.getString(CUSTOM_INDEX, "");
    prefs.end();

    // idx ist eine \n-getrennte Liste aller gespeicherten Custom-Keys
    int start = 0;
    while (start < (int)idx.length()) {
        int nl = idx.indexOf('\n', start);
        if (nl < 0) nl = idx.length();
        String k = idx.substring(start, nl);
        if (k.length() > 0) keys.push_back(k);
        start = nl + 1;
    }
    return keys;
}

bool WiFiWebManager::isReservedKey(const String& key) {
    String reservedKeys[] = {"ssid", "pwd", "hostname", "useStaticIP", "ip",
                           "gateway", "subnet", "dns", "ntpEnable", "ntpServer", "bootAttempts"};

    for (const String& reserved : reservedKeys) {
        if (key == reserved) return true;
    }
    return false;
}

bool WiFiWebManager::isValidCustomKey(const String& key) {
    if (key.length() == 0) {
        debugPrintln("Warnung: Custom-Key darf nicht leer sein!");
        return false;
    }
    if (isReservedKey(key)) {
        debugPrintf("Warnung: Schlüssel '%s' ist reserviert!\n", key.c_str());
        return false;
    }
    // NVS-Keys sind auf 15 Zeichen begrenzt; das Präfix "c_" belegt 2 davon
    if (key.length() > 13) {
        debugPrintf("Warnung: Schlüssel '%s' zu lang (max. 13 Zeichen)!\n", key.c_str());
        return false;
    }
    return true;
}

// Fügt einen Key zur \n-getrennten Registry hinzu (idempotent)
void WiFiWebManager::registerCustomKey(const String& key) {
    prefs.begin(CUSTOM_NS, false);
    String idx = prefs.getString(CUSTOM_INDEX, "");
    // Exakter Zeilentreffer: "\n" + key + "\n" innerhalb von "\n" + idx
    if (("\n" + idx).indexOf("\n" + key + "\n") < 0) {
        idx += key + "\n";
        prefs.putString(CUSTOM_INDEX, idx);
    }
    prefs.end();
}

// Entfernt einen Key aus der Registry
void WiFiWebManager::unregisterCustomKey(const String& key) {
    prefs.begin(CUSTOM_NS, false);
    String idx = prefs.getString(CUSTOM_INDEX, "");
    String entry = key + "\n";
    int pos = ("\n" + idx).indexOf("\n" + entry);
    if (pos >= 0) {
        // pos zeigt in "\n"+idx; der Eintrag in idx beginnt bei pos (Offset -1 + 1)
        idx.remove(pos, entry.length());
        prefs.putString(CUSTOM_INDEX, idx);
    }
    prefs.end();
}

String WiFiWebManager::renderMenu(const String& currentPath) {
    String nav = "<div class='nav-main'>";
    // Standardseiten (erste Zeile)
    nav += "<nav class='nav-std'>";
    struct PageEntry { String title, path; };
    PageEntry stdpages[] = {
        {"Home", "/"}, {"WLAN", "/wlan"}, {"NTP", "/ntp"}, {"Firmware", "/update"}, {"Reset", "/reset"}
    };
    for (auto &p : stdpages) {
        nav += "<a href='" + p.path + "'";
        if (currentPath == p.path) nav += " class='selected'";
        nav += ">" + p.title + "</a>";
    }
    nav += "</nav>";
    // Custompages (zweite Zeile)
    if (!customPages.empty()) {
        nav += "<nav class='nav-custom'>";
        for (const auto& page : customPages) {
            nav += "<a href='" + page.path + "'";
            if (currentPath == page.path) nav += " class='selected'";
            nav += ">" + page.title + "</a>";
        }
        nav += "</nav>";
    }
    nav += "</div>";
    return nav;
}

String WiFiWebManager::htmlWrap(const String& menutitle, const String& currentPath, const String& content) {
    String css = R"rawliteral(
      <style>
      body{background:#f3f6fa;font-family:sans-serif;margin:0;}
      .centerbox{max-width:420px;margin:2.5em auto;padding:2em;background:#fff;
      border-radius:16px;box-shadow:0 0 24px #0002;display:flex;flex-direction:column;align-items:center;}
      h1{font-size:1.6em;margin-bottom:1em;}h2{font-size:1.3em;margin:1.5em 0 1em;color:#2584fc;}
      label{display:block;margin:1em 0 0.5em;font-weight:600;}
      input,select{width:100%;font-size:1.1em;padding:0.8em;margin-bottom:1em;border-radius:8px;
      border:1px solid #bbb;box-sizing:border-box;}button,input[type=submit]{width:100%;padding:1em;
      font-size:1.1em;border:none;border-radius:8px;background:#2584fc;color:#fff;margin-top:0.7em;font-weight:700;
      cursor:pointer;box-shadow:0 4px 8px #2584fc22;transition:background 0.2s;}
      button:hover,input[type=submit]:hover{background:#1064b0;}
      .nav-main{width:100%;margin-bottom:1.5em;}
      .nav-std, .nav-custom {
        display: flex;
        flex-wrap: wrap;
        justify-content: center;
        gap: 1em;
      }
      .nav-custom { margin-top:0.2em; }
      .nav-std a, .nav-custom a {
        text-decoration:none;color:#2584fc;font-weight:600;font-size:1.1em;
        padding-bottom:2px;border-bottom:2px solid transparent;transition:border-color 0.2s;
      }
      .nav-std a.selected, .nav-std a:hover,
      .nav-custom a.selected, .nav-custom a:hover { border-color:#2584fc; }
      .status-box{background:#f8f9fa;border:1px solid #e9ecef;border-radius:8px;padding:1em;margin:1em 0;}
      .status-connected{border-color:#28a745;background:#f1f8e9;}
      .status-ap{border-color:#ffc107;background:#fff3cd;}
      .status-error{border-color:#dc3545;background:#f8d7da;}
      option.stored-network{background-color:#e7f3ff;font-weight:bold;}
      small{color:#6c757d;font-size:0.9em;}
      @media (max-width:600px){
        .centerbox{max-width:99vw;padding:1em;}
        h1{font-size:1.2em;}
        .nav-std a, .nav-custom a {font-size:1em;}
      }
      </style>
    )rawliteral";
    String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>" + menutitle + "</title>";
    html += "<link rel='icon' type='image/png' sizes='32x32' href='/favicon-32.png'>";
    html += "<link rel='icon' type='image/png' sizes='16x16' href='/favicon-16.png'>";
    html += css;
    html += "</head><body><div class='centerbox'>";
    html += renderMenu(currentPath);
    html += content;
    html += "</div></body></html>";
    return html;
}

void WiFiWebManager::addPage(const String& menutitle, const String& path, ContentHandler getHandler, ContentHandler postHandler) {
    if (path == "/") {
        this->rootGetHandler = getHandler;
        this->rootPostHandler = postHandler;
        return;
    }
    
    for (auto it = customPages.begin(); it != customPages.end(); ++it) {
        if (it->path == path) { customPages.erase(it); break; }
    }
    customPages.push_back({menutitle, path, getHandler, postHandler});

    // GET
    server.on(path.c_str(), HTTP_GET, [this, path, menutitle, getHandler](AsyncWebServerRequest *request) {
        String content = getHandler ? getHandler(request) : "<p>(Keine Seite definiert)</p>";
        String html = htmlWrap(menutitle, path, content);
        request->send(200, "text/html; charset=utf-8", html);
    });
    
    // POST
    if (postHandler) {
        server.on(path.c_str(), HTTP_POST, [this, path, menutitle, postHandler](AsyncWebServerRequest *request){
            String content = postHandler(request);
            String html = htmlWrap(menutitle, path, content);
            request->send(200, "text/html; charset=utf-8", html);
        });
    }
}

void WiFiWebManager::removePage(const String& path) {
    for (auto it = customPages.begin(); it != customPages.end(); ++it) {
        if (it->path == path) { customPages.erase(it); break; }
    }
}

void WiFiWebManager::setupWebServer() {
    // Favicon (aus PROGMEM, mit Cache-Header). /favicon.ico wird von Browsern
    // automatisch angefragt; die 16/32-Varianten referenzieren die <link>-Tags.
    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
        AsyncWebServerResponse *r = request->beginResponse_P(200, "image/png", FAVICON_PNG_32, FAVICON_PNG_32_LEN);
        r->addHeader("Cache-Control", "max-age=86400");
        request->send(r);
    });
    server.on("/favicon-32.png", HTTP_GET, [](AsyncWebServerRequest *request){
        AsyncWebServerResponse *r = request->beginResponse_P(200, "image/png", FAVICON_PNG_32, FAVICON_PNG_32_LEN);
        r->addHeader("Cache-Control", "max-age=86400");
        request->send(r);
    });
    server.on("/favicon-16.png", HTTP_GET, [](AsyncWebServerRequest *request){
        AsyncWebServerResponse *r = request->beginResponse_P(200, "image/png", FAVICON_PNG_16, FAVICON_PNG_16_LEN);
        r->addHeader("Cache-Control", "max-age=86400");
        request->send(r);
    });

    // Home-Seite mit Status
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (rootGetHandler) {
            String content = rootGetHandler(request);
            request->send(200, "text/html; charset=utf-8", htmlWrap("Home", "/", content));
        } else {
            String content = "<h1>WiFi Status</h1>";
            
            if (WiFi.getMode() == WIFI_STA && WiFi.status() == WL_CONNECTED) {
                content += "<div class='status-box status-connected'>";
                content += "<strong>✓ Verbunden</strong><br>";
                content += "<strong>SSID:</strong> " + WiFi.SSID() + "<br>";
                content += "<strong>IP:</strong> " + WiFi.localIP().toString() + "<br>";
                content += "<strong>Signal:</strong> " + String(WiFi.RSSI()) + " dBm";
                content += "</div>";
            } else if (WiFi.getMode() == WIFI_AP) {
                content += "<div class='status-box status-ap'>";
                content += "<strong>⚠ Setup-Modus</strong><br>";
                if (wifiBootAttempts >= MAX_BOOT_ATTEMPTS) {
                    content += "Grund: " + String(MAX_BOOT_ATTEMPTS) + " Verbindungsversuche fehlgeschlagen<br>";
                } else {
                    content += "Grund: Kein WLAN konfiguriert<br>";
                }
                content += "<strong>SSID:</strong> ESP32_SETUP<br>";
                content += "<strong>IP:</strong> 192.168.4.1";
                content += "</div>";
            } else {
                content += "<div class='status-box status-error'>";
                content += "<strong>✗ Unbekannter Status</strong>";
                content += "</div>";
            }
            
            if (getHostname().length() > 0) {
                content += "<p><strong>Hostname:</strong> " + getHostname() + "</p>";
            }
            
            request->send(200, "text/html; charset=utf-8", htmlWrap("Home", "/", content));
        }
    });

    // WLAN-Konfiguration
    server.on("/wlan", HTTP_GET, [this](AsyncWebServerRequest *request){
        String html = "<h1>WLAN Konfiguration</h1>";
        
        // Aktuelle WLAN-Konfiguration anzeigen
        if (ssid.length() > 0) {
            html += "<div class='status-box'>";
            html += "<strong>Gespeichertes WLAN:</strong> " + ssid + "<br>";
            html += "<strong>Boot-Versuche:</strong> " + String(wifiBootAttempts) + "/" + String(MAX_BOOT_ATTEMPTS);
            html += "</div>";
        }
        
        html += "<form action='/wlan_save' method='POST'>";
        html += "<label>SSID:</label><select name='ssid'>";
        html += getAvailableSSIDs();
        html += "</select>";
        html += "<label>Passwort:</label>";
        html += "<input name='pwd' type='password' value='' autocomplete='off'>";
        html += "<input type='submit' value='WLAN speichern'>";
        html += "</form>";
        
        html += "<h2>Erweiterte Einstellungen</h2>";
        html += "<form action='/network_save' method='POST'>";
        html += "<label>Hostname:</label>";
        html += "<input name='hostname' value='" + hostname + "' placeholder='Standard: " + defaultHostname + "'>";
        if (defaultHostname.length() > 0) {
            html += "<small>Standard aus Code: " + defaultHostname + "</small>";
        }
        html += "<label><input type='checkbox' name='useStaticIP' ";
        html += (useStaticIP ? "checked" : "");
        html += "> Statische IP aktivieren</label>";
        html += "<input name='ip' placeholder='IP-Adresse' value='" + ip + "'>";
        html += "<input name='gateway' placeholder='Gateway' value='" + gateway + "'>";
        html += "<input name='subnet' placeholder='Subnetz' value='" + subnet + "'>";
        html += "<input name='dns' placeholder='DNS' value='" + dns + "'>";
        html += "<input type='submit' value='Netzwerk speichern'>";
        html += "</form>";
        
        request->send(200, "text/html; charset=utf-8", htmlWrap("WLAN Konfiguration", "/wlan", html));
    });

    // WLAN speichern
    server.on("/wlan_save", HTTP_POST, [this](AsyncWebServerRequest *request){
        String newSSID = request->hasParam("ssid", true) ? request->getParam("ssid", true)->value() : "";
        String newPWD = request->hasParam("pwd", true) ? request->getParam("pwd", true)->value() : "";
        
        if (newSSID.length() > 0) {
            ssid = newSSID;
            password = newPWD;
            resetBootAttempts(); // Reset der Versuche bei neuer Konfiguration
            saveConfig();
            shouldReboot = true;
            request->send(200, "text/html; charset=utf-8", 
                htmlWrap("WLAN gespeichert", "/wlan", "<p>WLAN-Daten gespeichert! Neustart...</p>"));
        } else {
            request->send(200, "text/html; charset=utf-8", 
                htmlWrap("Fehler", "/wlan", "<p>SSID darf nicht leer sein!</p><a href='/wlan'>Zurück</a>"));
        }
    });

    // Netzwerk-Einstellungen speichern
    server.on("/network_save", HTTP_POST, [this](AsyncWebServerRequest *request){
        String newHostname = request->hasParam("hostname", true) ? request->getParam("hostname", true)->value() : "";
        bool newUseStaticIP = request->hasParam("useStaticIP", true);
        String newIP = request->hasParam("ip", true) ? request->getParam("ip", true)->value() : "";
        String newGateway = request->hasParam("gateway", true) ? request->getParam("gateway", true)->value() : "";
        String newSubnet = request->hasParam("subnet", true) ? request->getParam("subnet", true)->value() : "";
        String newDNS = request->hasParam("dns", true) ? request->getParam("dns", true)->value() : "";
        
        hostname = newHostname;
        useStaticIP = newUseStaticIP;
        ip = newIP;
        gateway = newGateway;
        subnet = newSubnet;
        dns = newDNS;
        
        saveConfig();
        shouldReboot = true;
        request->send(200, "text/html; charset=utf-8", 
            htmlWrap("Netzwerk gespeichert", "/wlan", "<p>Netzwerk-Einstellungen gespeichert! Neustart...</p>"));
    });

    // Reset-Seite
    server.on("/reset", HTTP_GET, [this](AsyncWebServerRequest *request){
        String html = "<h1>Reset-Optionen</h1>";
        html += "<div class='status-box'>";
        html += "<p><strong>Hardware Reset-Button (GPIO 0):</strong></p>";
        html += "<p>• 3-10 Sekunden: Nur WLAN-Daten löschen</p>";
        html += "<p>• >10 Sekunden: Kompletter Werks-Reset</p>";
        html += "</div>";
        
        html += "<h2>Software-Reset</h2>";
        html += "<form action='/reset_wifi' method='POST'>";
        html += "<input type='submit' value='Nur WLAN-Daten löschen' style='background:#ffc107;'>";
        html += "</form>";
        
        html += "<form action='/reset_all' method='POST'>";
        html += "<input type='submit' value='Kompletter Werks-Reset' style='background:#dc3545;'>";
        html += "</form>";
        
        request->send(200, "text/html; charset=utf-8", htmlWrap("Reset", "/reset", html));
    });

    // WLAN-Reset
    server.on("/reset_wifi", HTTP_POST, [this](AsyncWebServerRequest *request){
        clearWiFiConfig();
        shouldReboot = true;
        request->send(200, "text/html; charset=utf-8", 
            htmlWrap("WLAN Reset", "/reset", "<p>WLAN-Daten gelöscht! Neustart...</p>"));
    });

    // Vollständiger Reset
    server.on("/reset_all", HTTP_POST, [this](AsyncWebServerRequest *request){
        clearAllConfig();
        shouldReboot = true;
        request->send(200, "text/html; charset=utf-8", 
            htmlWrap("Werks-Reset", "/reset", "<p>Werks-Reset durchgeführt! Neustart...</p>"));
    });

    // NTP-Konfiguration
    server.on("/ntp", HTTP_GET, [this](AsyncWebServerRequest *request){
        String html = "<h1>NTP Einstellungen</h1>";
        html += "<form action='/ntp_save' method='POST'>";
        html += "<label><input type='checkbox' name='ntpEnable' ";
        html += (ntpEnable ? "checked" : "");
        html += "> NTP aktivieren</label>";
        html += "<label>NTP Server:</label>";
        html += "<input name='ntpServer' value='" + ntpServer + "'>";
        html += "<input type='submit' value='Speichern'>";
        html += "</form>";
        
        request->send(200, "text/html; charset=utf-8", htmlWrap("NTP Einstellungen", "/ntp", html));
    });

    server.on("/ntp_save", HTTP_POST, [this](AsyncWebServerRequest *request){
        bool newNtpEnable = request->hasParam("ntpEnable", true);
        String newNtpServer = request->hasParam("ntpServer", true) ? request->getParam("ntpServer", true)->value() : "pool.ntp.org";
        
        saveNtpConfig(newNtpEnable, newNtpServer);
        request->send(200, "text/html; charset=utf-8", 
            htmlWrap("NTP Einstellungen", "/ntp", "<p>NTP-Einstellungen gespeichert!</p><a href='/ntp'>Zurück</a>"));
    });

    // OTA Firmware Update
    server.on("/update", HTTP_GET, [this](AsyncWebServerRequest *request){
        String html = "<h1>Firmware Update</h1>";
        html += "<div class='status-box'>";
        if (firmwareVersion.length() > 0) {
            html += "<p><strong>Firmware-Version:</strong> " + firmwareVersion + "</p>";
        }
        html += "<p><strong>Build:</strong> " + String(__DATE__) + " " + String(__TIME__) + "</p>";
        html += "<p><strong>Freier Speicher:</strong> " + String(ESP.getFreeHeap()) + " Bytes</p>";
        html += "</div>";
        
        html += "<form method='POST' action='/update' enctype='multipart/form-data'>";
        html += "<label>Firmware-Datei (.bin):</label>";
        html += "<input type='file' name='update' accept='.bin'>";
        html += "<input type='submit' value='Firmware Update starten'>";
        html += "</form>";
        
        html += "<p><small>Warnung: Unterbrechen Sie den Update-Vorgang nicht!</small></p>";
        
        request->send(200, "text/html; charset=utf-8", htmlWrap("Firmware Update", "/update", html));
    });

    server.on("/update", HTTP_POST,
        [this](AsyncWebServerRequest *request) {
            bool ok = !Update.hasError();
            const char* body = ok
                ? "<!DOCTYPE html><html><head><meta charset='utf-8'><title>Update</title></head><body>"
                  "<h1>Update erfolgreich</h1><p>Neustart in 3 Sekunden...</p>"
                  "<script>setTimeout(function(){window.location.href='/';}, 3000);</script>"
                  "</body></html>"
                : "<!DOCTYPE html><html><head><meta charset='utf-8'><title>Update</title></head><body>"
                  "<h1>Update fehlgeschlagen</h1><p>Die Firmware konnte nicht geschrieben werden. "
                  "Das Gerät läuft mit der bisherigen Firmware weiter.</p>"
                  "<a href='/update'>Zurück</a></body></html>";
            AsyncWebServerResponse *response = request->beginResponse(ok ? 200 : 500, "text/html; charset=utf-8", body);
            response->addHeader("Connection", "close");
            request->send(response);
            // Neustart erst nach dem Senden der Antwort (geordnet über loop()).
            if (ok) shouldReboot = true;
        },
        [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            if (!index) {
                debugPrintf("Update gestartet: %s\n", filename.c_str());
                if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                    if (debugMode) Update.printError(Serial);
                }
            }
            if (!Update.hasError()) {
                if (Update.write(data, len) != len) {
                    if (debugMode) Update.printError(Serial);
                }
            }
            if (final) {
                if (Update.end(true)) {
                    debugPrintf("Update erfolgreich: %uB\n", index + len);
                } else {
                    if (debugMode) Update.printError(Serial);
                    Update.abort();
                }
            }
        }
    );

    server.begin();
    debugPrintln("WebServer gestartet!");
}