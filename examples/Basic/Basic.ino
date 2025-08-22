/*
 * WiFiWebManager Library v2.1.1 - Basic Example
 *
 * Minimales Beispiel für die WiFiWebManager Library.
 * Zeigt grundlegende WiFi-Verwaltung und Web-Interface.
 *
 * Features:
 * - Automatische WiFi-Verbindung
 * - Web-Interface zur Konfiguration
 * - OTA Updates über Web-Interface
 * - Hardware Reset-Button Support
 */

#include <WiFiWebManager.h>

WiFiWebManager wwm;

void setup()
{
    Serial.begin(115200);
    Serial.println("WiFiWebManager Basic Example v2.1.1");

    // Optional: Debug-Modus aktivieren
    wwm.setDebugMode(true);

    // Optional: Hostname setzen
    wwm.setDefaultHostname("ESP32-Basic");

    // Optional: NTP aktivieren
    wwm.setDefaultNTP(true, "pool.ntp.org");

    // WiFiWebManager starten
    wwm.begin();

    Serial.println("Setup abgeschlossen!");
    Serial.println("Web-Interface verfügbar unter:");
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.print("http://");
        Serial.println(WiFi.localIP());
        Serial.print("http://");
        Serial.print(wwm.getHostname());
        Serial.println(".local");
    }
    else
    {
        Serial.println("http://192.168.4.1 (Setup-Modus)");
    }
}

void loop()
{
    // WiFiWebManager Loop - muss regelmäßig aufgerufen werden
    wwm.loop();

    // Hier kann Ihre Anwendungslogik stehen
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 30000)
    { // Alle 30 Sekunden
        lastPrint = millis();

        Serial.println("=== Status ===");
        Serial.printf("WiFi: %s\n", WiFi.status() == WL_CONNECTED ? "Verbunden" : "Getrennt");
        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.printf("SSID: %s\n", WiFi.SSID().c_str());
            Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
            Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
        }
        Serial.printf("Freier Heap: %d Bytes\n", ESP.getFreeHeap());
        Serial.println("===============");
    }

    // Kleine Pause
    delay(100);
}

/*
 * Hardware Reset-Button (GPIO 0):
 * - 3-10 Sekunden gedrückt: Nur WiFi-Daten löschen
 * - >10 Sekunden gedrückt: Kompletter Werks-Reset
 *
 * Web-Interface Funktionen:
 * - WiFi-Konfiguration
 * - NTP-Einstellungen
 * - Firmware-Update (OTA)
 * - System-Reset Optionen
 * - Netzwerk-Einstellungen (statische IP)
 */