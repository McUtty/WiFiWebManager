/*
 * WiFiWebManager Library v2.1.1 - Custom Pages Example
 *
 * Demonstriert die Erstellung eigener Web-Seiten mit Custom Data Storage.
 * Zeigt Integration von Anwendungs-spezifischen Konfigurationsseiten
 * in das WiFiWebManager Web-Interface.
 *
 * Features:
 * - Custom Web-Seiten mit eigenen Formularen
 * - Sichere Speicherung von Konfigurationsdaten
 * - Integration in das Standard-Menü
 * - GET und POST Handler
 */

#include <WiFiWebManager.h>

WiFiWebManager wwm;

// Beispiel-Konfigurationsvariablen
String deviceName = "Mein Gerät";
int sensorInterval = 30;
bool alertsEnabled = true;
float temperatureThreshold = 25.0;
String serverURL = "";

void setup()
{
    Serial.begin(115200);
    Serial.println("WiFiWebManager Custom Pages Example v2.1.1");

    wwm.setDebugMode(true);
    wwm.setDefaultHostname("ESP32-CustomPages");
    wwm.setDefaultNTP(true);

    // Konfiguration laden
    loadConfiguration();

    // Custom Pages hinzufügen
    setupCustomPages();

    wwm.begin();

    Serial.println("Custom Pages Setup abgeschlossen!");
    Serial.println("Verfügbare Seiten:");
    Serial.println("- /device     (Geräte-Konfiguration)");
    Serial.println("- /sensors    (Sensor-Einstellungen)");
    Serial.println("- /alerts     (Alarm-Konfiguration)");
    Serial.println("- /status     (System-Status)");
}

void loop()
{
    wwm.loop();

    // Anwendungslogik basierend auf Konfiguration
    static unsigned long lastSensorRead = 0;
    if (millis() - lastSensorRead > (sensorInterval * 1000))
    {
        lastSensorRead = millis();

        // Sensor auslesen (simuliert)
        float temperature = readTemperatureSensor();

        Serial.printf("🌡️  %s: %.1f°C (Schwelle: %.1f°C)\n",
                      deviceName.c_str(), temperature, temperatureThreshold);

        // Alert prüfen
        if (alertsEnabled && temperature > temperatureThreshold)
        {
            Serial.println("🚨 Temperatur-Alarm!");
            if (serverURL.length() > 0)
            {
                Serial.printf("📡 Sende Alert an: %s\n", serverURL.c_str());
                // Hier würde HTTP-Request an Server stehen
            }
        }
    }

    delay(100);
}

void setupCustomPages()
{
    // 1. Geräte-Konfiguration
    wwm.addPage("Gerät", "/device", handleDevicePage, handleDevicePost);

    // 2. Sensor-Einstellungen
    wwm.addPage("Sensoren", "/sensors", handleSensorsPage, handleSensorsPost);

    // 3. Alarm-Konfiguration
    wwm.addPage("Alarme", "/alerts", handleAlertsPage, handleAlertsPost);

    // 4. System-Status (nur GET)
    wwm.addPage("Status", "/status", handleStatusPage);
}

// =============================================================================
// Geräte-Konfiguration
// =============================================================================

String handleDevicePage(AsyncWebServerRequest *request)
{
    String html = "<h1>Geräte-Konfiguration</h1>";

    html += "<form method='POST'>";
    html += "<label>Geräte-Name:</label>";
    html += "<input name='deviceName' value='" + deviceName + "' maxlength='50'>";
    html += "<label>Server-URL (optional):</label>";
    html += "<input name='serverURL' value='" + serverURL + "' placeholder='http://server.com/api'>";
    html += "<input type='submit' value='Speichern'>";
    html += "</form>";

    return html;
}

String handleDevicePost(AsyncWebServerRequest *request)
{
    bool changed = false;

    if (request->hasParam("deviceName", true))
    {
        String newName = request->getParam("deviceName", true)->value();
        if (newName != deviceName)
        {
            deviceName = newName;
            wwm.saveCustomData("device_name", deviceName);
            changed = true;
        }
    }

    if (request->hasParam("serverURL", true))
    {
        String newURL = request->getParam("serverURL", true)->value();
        if (newURL != serverURL)
        {
            serverURL = newURL;
            wwm.saveCustomData("server_url", serverURL);
            changed = true;
        }
    }

    String message = changed ? "✅ Konfiguration gespeichert!" : "ℹ️ Keine Änderungen";
    return "<h1>Gerät</h1><p>" + message + "</p><a href='/device'>Zurück</a>";
}

// =============================================================================
// Sensor-Einstellungen
// =============================================================================

String handleSensorsPage(AsyncWebServerRequest *request)
{
    String html = "<h1>Sensor-Einstellungen</h1>";

    html += "<form method='POST'>";
    html += "<label>Mess-Intervall (Sekunden):</label>";
    html += "<input type='number' name='interval' value='" + String(sensorInterval) + "' min='5' max='3600'>";
    html += "<label>Temperatur-Schwelle (°C):</label>";
    html += "<input type='number' name='threshold' value='" + String(temperatureThreshold, 1) + "' step='0.1' min='-50' max='100'>";
    html += "<input type='submit' value='Speichern'>";
    html += "</form>";

    html += "<div class='status-box'>";
    html += "<strong>Aktuelle Einstellungen:</strong><br>";
    html += "Intervall: " + String(sensorInterval) + "s<br>";
    html += "Schwelle: " + String(temperatureThreshold, 1) + "°C";
    html += "</div>";

    return html;
}

String handleSensorsPost(AsyncWebServerRequest *request)
{
    bool changed = false;

    if (request->hasParam("interval", true))
    {
        int newInterval = request->getParam("interval", true)->value().toInt();
        if (newInterval >= 5 && newInterval <= 3600 && newInterval != sensorInterval)
        {
            sensorInterval = newInterval;
            wwm.saveCustomData("sensor_interval", sensorInterval);
            changed = true;
        }
    }

    if (request->hasParam("threshold", true))
    {
        float newThreshold = request->getParam("threshold", true)->value().toFloat();
        if (abs(newThreshold - temperatureThreshold) > 0.1)
        {
            temperatureThreshold = newThreshold;
            wwm.saveCustomData("temp_threshold", temperatureThreshold);
            changed = true;
        }
    }

    String message = changed ? "✅ Sensor-Einstellungen gespeichert!" : "ℹ️ Keine Änderungen";
    return "<h1>Sensoren</h1><p>" + message + "</p><a href='/sensors'>Zurück</a>";
}

// =============================================================================
// Alarm-Konfiguration
// =============================================================================

String handleAlertsPage(AsyncWebServerRequest *request)
{
    String html = "<h1>Alarm-Konfiguration</h1>";

    html += "<form method='POST'>";
    html += "<label><input type='checkbox' name='alertsEnabled' " + String(alertsEnabled ? "checked" : "") + "> Alarme aktivieren</label>";
    html += "<input type='submit' value='Speichern'>";
    html += "</form>";

    html += "<div class='status-box " + String(alertsEnabled ? "status-connected" : "status-error") + "'>";
    html += "<strong>Status:</strong> " + String(alertsEnabled ? "✅ Aktiviert" : "❌ Deaktiviert");
    html += "</div>";

    if (alertsEnabled)
    {
        html += "<h2>Alarm-Aktionen</h2>";
        html += "<p>📧 E-Mail: " + String(serverURL.length() > 0 ? "Konfiguriert" : "Nicht konfiguriert") + "</p>";
        html += "<p>🔊 Lokaler Alarm: Aktiviert</p>";
        html += "<p>📱 Push-Benachrichtigung: " + String(serverURL.length() > 0 ? "Aktiviert" : "Deaktiviert") + "</p>";
    }

    return html;
}

String handleAlertsPost(AsyncWebServerRequest *request)
{
    bool newAlertsEnabled = request->hasParam("alertsEnabled", true);
    bool changed = (newAlertsEnabled != alertsEnabled);

    if (changed)
    {
        alertsEnabled = newAlertsEnabled;
        wwm.saveCustomData("alerts_enabled", alertsEnabled);
    }

    String message = changed ? "✅ Alarm-Konfiguration gespeichert!" : "ℹ️ Keine Änderungen";
    return "<h1>Alarme</h1><p>" + message + "</p><a href='/alerts'>Zurück</a>";
}

// =============================================================================
// System-Status (nur Anzeige)
// =============================================================================

String handleStatusPage(AsyncWebServerRequest *request)
{
    String html = "<h1>System-Status</h1>";

    // System-Information
    html += "<div class='status-box status-connected'>";
    html += "<h3>System</h3>";
    html += "<strong>Geräte-Name:</strong> " + deviceName + "<br>";
    html += "<strong>Uptime:</strong> " + String(millis() / 1000) + " Sekunden<br>";
    html += "<strong>Freier Heap:</strong> " + String(ESP.getFreeHeap()) + " Bytes<br>";
    html += "<strong>WiFi RSSI:</strong> " + String(WiFi.RSSI()) + " dBm";
    html += "</div>";

    // Sensor-Status
    html += "<div class='status-box'>";
    html += "<h3>Sensoren</h3>";
    html += "<strong>Mess-Intervall:</strong> " + String(sensorInterval) + "s<br>";
    html += "<strong>Temperatur-Schwelle:</strong> " + String(temperatureThreshold, 1) + "°C<br>";
    html += "<strong>Letzte Messung:</strong> " + String(readTemperatureSensor(), 1) + "°C";
    html += "</div>";

    // Alarm-Status
    html += "<div class='status-box " + String(alertsEnabled ? "status-connected" : "status-error") + "'>";
    html += "<h3>Alarme</h3>";
    html += "<strong>Status:</strong> " + String(alertsEnabled ? "✅ Aktiviert" : "❌ Deaktiviert") + "<br>";
    html += "<strong>Server-URL:</strong> " + String(serverURL.length() > 0 ? serverURL : "Nicht konfiguriert");
    html += "</div>";

    // Custom Data Übersicht
    html += "<div class='status-box'>";
    html += "<h3>Gespeicherte Konfiguration</h3>";
    html += "<small>";
    html += "device_name: " + String(wwm.hasCustomData("device_name") ? "✓" : "✗") + "<br>";
    html += "sensor_interval: " + String(wwm.hasCustomData("sensor_interval") ? "✓" : "✗") + "<br>";
    html += "temp_threshold: " + String(wwm.hasCustomData("temp_threshold") ? "✓" : "✗") + "<br>";
    html += "alerts_enabled: " + String(wwm.hasCustomData("alerts_enabled") ? "✓" : "✗") + "<br>";
    html += "server_url: " + String(wwm.hasCustomData("server_url") ? "✓" : "✗");
    html += "</small>";
    html += "</div>";

    // Auto-Refresh
    html += "<script>setTimeout(function(){location.reload();}, 30000);</script>";
    html += "<p><small>Seite aktualisiert sich alle 30 Sekunden automatisch.</small></p>";

    return html;
}

// =============================================================================
// Hilfsfunktionen
// =============================================================================

void loadConfiguration()
{
    // Konfiguration aus Custom Data laden
    deviceName = wwm.loadCustomData("device_name", "Mein Gerät");
    sensorInterval = wwm.loadCustomDataInt("sensor_interval", 30);
    temperatureThreshold = wwm.loadCustomDataFloat("temp_threshold", 25.0);
    alertsEnabled = wwm.loadCustomDataBool("alerts_enabled", true);
    serverURL = wwm.loadCustomData("server_url", "");

    Serial.println("Konfiguration geladen:");
    Serial.printf("- Gerät: %s\n", deviceName.c_str());
    Serial.printf("- Intervall: %ds\n", sensorInterval);
    Serial.printf("- Schwelle: %.1f°C\n", temperatureThreshold);
    Serial.printf("- Alarme: %s\n", alertsEnabled ? "aktiviert" : "deaktiviert");
    Serial.printf("- Server: %s\n", serverURL.length() > 0 ? serverURL.c_str() : "nicht konfiguriert");
}

float readTemperatureSensor()
{
    // Simulierter Temperatur-Sensor
    // In echten Anwendungen würde hier I2C/SPI/OneWire Code stehen
    static float baseTemp = 22.5;
    static unsigned long lastChange = 0;

    // Zufällige Schwankungen alle 10 Sekunden
    if (millis() - lastChange > 10000)
    {
        lastChange = millis();
        baseTemp += (random(-20, 21) / 10.0); // ±2°C Änderung
        baseTemp = constrain(baseTemp, 15.0, 35.0);
    }

    // Kleine zufällige Schwankungen
    return baseTemp + (random(-5, 6) / 10.0);
}

/*
 * Custom Pages Tipps:
 *
 * 1. HTML-Struktur:
 *    - Verwende die Standard-CSS-Klassen der Library
 *    - status-box, status-connected, status-error verfügbar
 *    - Responsive Design wird automatisch angewendet
 *
 * 2. Formulare:
 *    - POST-Handler für Daten-Speicherung
 *    - GET-Handler für Seiten-Anzeige
 *    - Input-Validierung im POST-Handler
 *
 * 3. Custom Data:
 *    - wwm.saveCustomData() für persistente Speicherung
 *    - Verschiedene Datentypen: String, int, bool, float
 *    - Automatische NVS-Speicherung (Flash)
 *
 * 4. Navigation:
 *    - Seiten werden automatisch ins Menü integriert
 *    - Titel wird als Menü-Text verwendet
 *    - URL-Pfad frei wählbar
 *
 * 5. JavaScript:
 *    - Einfache Scripts für Auto-Refresh möglich
 *    - AJAX-Requests für dynamische Inhalte
 *    - Lokale Validierung von Formularen
 */