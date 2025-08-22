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
    wifiBootAttempts++;
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
        String selected = (ssid == WiFi.SSID(i)) ? "selected" : "";
        String cssClass = (ssid == WiFi.SSID(i)) ? " class='stored-network'" : "";

        options += "<option value='" + WiFi.SSID(i) + "' " + selected + cssClass + ">" + WiFi.SSID(i);
        if (ssid == WiFi.SSID(i))
            options += " (gespeichert)";
        options += "</option>";
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

    // GPIO Wake-up aktivieren
    esp_sleep_enable_gpio_wakeup();

    // Reset-Button (Library-intern)
    gpio_wakeup_enable(GPIO_NUM_0, GPIO_INTR_LOW_LEVEL);
    debugPrintln("Reset-Button Wake-up aktiviert");

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