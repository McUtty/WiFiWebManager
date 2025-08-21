/*
  WiFiWebManager - Basic Example
  
  Dieses Beispiel zeigt die grundlegende Nutzung des WiFiWebManager.
  
  Hardware:
  - ESP32 Board
  - Optional: Reset-Button zwischen GPIO 0 und GND
  
  Nach dem Upload:
  1. ESP32 startet im AP-Modus (SSID: ESP32_SETUP)
  2. Verbinden Sie sich mit diesem WLAN
  3. Öffnen Sie http://192.168.4.1
  4. Konfigurieren Sie Ihr WLAN über das Web-Interface
  
  Version: 1.2.1
  Autor: McUtty
*/

#include <WiFiWebManager.h>

// WiFiWebManager Instanz erstellen
WiFiWebManager wifiManager;

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println("=== WiFiWebManager Basic Example ===");
    
    // Debug-Modus aktivieren (optional)
    wifiManager.setDebugMode(true);
    
    // Standard-Hostname setzen (optional)
    wifiManager.setDefaultHostname("ESP32-Basic");
    
    // WiFiWebManager starten
    wifiManager.begin();
    
    Serial.println("Setup abgeschlossen!");
    Serial.println("Web-Interface verfügbar unter:");
    
    if (WiFi.getMode() == WIFI_STA && WiFi.status() == WL_CONNECTED) {
        Serial.print("WLAN-Modus: http://");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("AP-Modus: http://192.168.4.1");
    }
}

void loop() {
    // WICHTIG: WiFiWebManager Loop muss aufgerufen werden!
    wifiManager.loop();
    
    // Hier kann Ihr Code stehen...
    
    // Beispiel: Status alle 30 Sekunden ausgeben
    static unsigned long lastStatus = 0;
    if (millis() - lastStatus > 30000) {
        lastStatus = millis();
        
        Serial.println("\n--- Status ---");
        Serial.printf("Uptime: %lu Sekunden\n", millis() / 1000);
        Serial.printf("Freier Heap: %d Bytes\n", ESP.getFreeHeap());
        
        if (WiFi.getMode() == WIFI_STA && WiFi.status() == WL_CONNECTED) {
            Serial.printf("WLAN: %s (RSSI: %d dBm)\n", WiFi.SSID().c_str(), WiFi.RSSI());
            Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
        } else if (WiFi.getMode() == WIFI_AP) {
            Serial.println("Modus: Access Point (Setup)");
        }
        Serial.println("---------------\n");
    }
    
    delay(100);
}