#include "WiFiWebManager.h"

// WiFiWebManager Instanz
WiFiWebManager wifiManager;

// Test-Variablen für Custom Data
int sensorInterval = 5000;
bool debugMode = false;
float calibrationFactor = 1.0;
String deviceName = "TestDevice";

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== WiFiWebManager Test-Programm ===");
    
    // Debug-Modus aktivieren (nur über Code möglich)
    wifiManager.setDebugMode(true);  // Aktiviert alle Debug-Ausgaben
    
    // Standard-Hostname setzen (kann später über Web-GUI geändert werden)
    wifiManager.setDefaultHostname("ESP32-TestDevice");
    
    // Lade gespeicherte Custom Data
    loadCustomSettings();
    
    // Erstelle Test-Seiten
    setupCustomPages();
    
    // Starte WiFiWebManager
    wifiManager.begin();
    
    Serial.println("=== Setup abgeschlossen ===");
    Serial.println("Test-Features:");
    Serial.println("- Reset-Button an GPIO 0 (3s = WLAN reset, 10s = Vollreset)");
    Serial.println("- Custom Daten über /test Seite");
    Serial.println("- Sensor-Simulation über /sensor Seite");
    Serial.println("- Debug-Ausgaben im Serial Monitor");
}

void loop() {
    // WiFiWebManager Loop (wichtig!)
    wifiManager.loop();
    
    // Simuliere Sensor-Daten alle X Sekunden
    static unsigned long lastSensorRead = 0;
    if (millis() - lastSensorRead > sensorInterval) {
        lastSensorRead = millis();
        simulateSensorReading();
    }
    
    // Debug-Ausgaben falls aktiviert
    static unsigned long lastDebugOutput = 0;
    if (debugMode && millis() - lastDebugOutput > 10000) {
        lastDebugOutput = millis();
        printDebugInfo();
    }
    
    delay(100); // Kleine Pause
}

void loadCustomSettings() {
    // Lade Custom Data mit Default-Werten
    sensorInterval = wifiManager.loadCustomDataInt("sensorInterval", 5000);
    debugMode = wifiManager.loadCustomDataBool("debugMode", false);
    calibrationFactor = wifiManager.loadCustomDataFloat("calibration", 1.0);
    deviceName = wifiManager.loadCustomData("deviceName", "TestDevice");
    
    Serial.println("\n--- Geladene Einstellungen ---");
    Serial.printf("Sensor Interval: %d ms\n", sensorInterval);
    Serial.printf("Debug Mode: %s\n", debugMode ? "AN" : "AUS");
    Serial.printf("Calibration: %.2f\n", calibrationFactor);
    Serial.printf("Device Name: %s\n", deviceName.c_str());
    Serial.printf("Hostname: %s\n", wifiManager.getHostname().c_str());
}

void saveCustomSettings() {
    // Speichere alle Custom Data
    wifiManager.saveCustomData("sensorInterval", sensorInterval);
    wifiManager.saveCustomData("debugMode", debugMode);
    wifiManager.saveCustomData("calibration", calibrationFactor);
    wifiManager.saveCustomData("deviceName", deviceName);
    
    Serial.println("Custom Einstellungen gespeichert!");
}

void setupCustomPages() {
    // Test-Seite für Custom Data
    wifiManager.addPage("Test", "/test", 
        [](AsyncWebServerRequest *request) -> String {
            String html = "<h1>Test-Einstellungen</h1>";
            
            html += "<form action='/test' method='POST'>";
            
            html += "<label>Sensor-Intervall (ms):</label>";
            html += "<input name='interval' type='number' value='" + String(sensorInterval) + "' min='1000' max='60000'>";
            
            html += "<label><input type='checkbox' name='debug' ";
            html += (debugMode ? "checked" : "");
            html += "> Debug-Modus aktivieren</label>";
            
            html += "<label>Kalibrationsfaktor:</label>";
            html += "<input name='calibration' type='number' step='0.01' value='" + String(calibrationFactor, 2) + "'>";
            
            html += "<label>Gerätename:</label>";
            html += "<input name='devicename' value='" + deviceName + "'>";
            
            html += "<input type='submit' value='Einstellungen speichern'>";
            html += "</form>";
            
            // Aktuelle Werte anzeigen
            html += "<h2>Aktuelle Werte</h2>";
            html += "<div class='status-box'>";
            html += "<p><strong>Sensor-Intervall:</strong> " + String(sensorInterval) + " ms</p>";
            html += "<p><strong>Debug-Modus:</strong> " + String(debugMode ? "AN" : "AUS") + "</p>";
            html += "<p><strong>Kalibration:</strong> " + String(calibrationFactor, 2) + "</p>";
            html += "<p><strong>Gerätename:</strong> " + deviceName + "</p>";
            html += "<p><strong>Hostname:</strong> " + wifiManager.getHostname() + "</p>";
            html += "</div>";
            
            return html;
        },
        [](AsyncWebServerRequest *request) -> String {
            // POST Handler - Einstellungen speichern
            if (request->hasParam("interval", true)) {
                sensorInterval = request->getParam("interval", true)->value().toInt();
            }
            debugMode = request->hasParam("debug", true);
            if (request->hasParam("calibration", true)) {
                calibrationFactor = request->getParam("calibration", true)->value().toFloat();
            }
            if (request->hasParam("devicename", true)) {
                deviceName = request->getParam("devicename", true)->value();
            }
            
            saveCustomSettings();
            
            return "<h1>Einstellungen gespeichert!</h1><p>Neue Werte sind aktiv.</p><a href='/test'>Zurück</a>";
        }
    );
    
    // Sensor-Status Seite
    wifiManager.addPage("Sensor", "/sensor", 
        [](AsyncWebServerRequest *request) -> String {
            String html = "<h1>Sensor-Status</h1>";
            
            // Simulierte Sensor-Werte
            float temperature = 20.0 + random(-50, 150) / 10.0;
            float humidity = 50.0 + random(-200, 200) / 10.0;
            int lightLevel = random(0, 1024);
            
            // Kalibrierte Werte
            temperature *= calibrationFactor;
            
            html += "<div class='status-box status-connected'>";
            html += "<h2>Aktuelle Messwerte</h2>";
            html += "<p><strong>Temperatur:</strong> " + String(temperature, 1) + " °C</p>";
            html += "<p><strong>Luftfeuchtigkeit:</strong> " + String(humidity, 1) + " %</p>";
            html += "<p><strong>Lichtstärke:</strong> " + String(lightLevel) + " lux</p>";
            html += "<p><strong>Letztes Update:</strong> " + String(millis()/1000) + "s</p>";
            html += "</div>";
            
            html += "<div class='status-box'>";
            html += "<h2>Konfiguration</h2>";
            html += "<p><strong>Messintervall:</strong> " + String(sensorInterval/1000.0, 1) + " s</p>";
            html += "<p><strong>Kalibrationsfaktor:</strong> " + String(calibrationFactor, 2) + "</p>";
            html += "<p><strong>Debug-Modus:</strong> " + String(debugMode ? "Aktiv" : "Inaktiv") + "</p>";
            html += "</div>";
            
            // Auto-Refresh
            html += "<script>setTimeout(function(){location.reload();}, " + String(sensorInterval) + ");</script>";
            
            return html;
        }
    );
    
    // Info-Seite
    wifiManager.addPage("Info", "/info", 
        [](AsyncWebServerRequest *request) -> String {
            String html = "<h1>System-Information</h1>";
            
            html += "<div class='status-box'>";
            html += "<h2>Hardware</h2>";
            html += "<p><strong>Chip Model:</strong> " + String(ESP.getChipModel()) + "</p>";
            html += "<p><strong>CPU Frequenz:</strong> " + String(ESP.getCpuFreqMHz()) + " MHz</p>";
            html += "<p><strong>Freier Heap:</strong> " + String(ESP.getFreeHeap()) + " Bytes</p>";
            html += "<p><strong>Flash Size:</strong> " + String(ESP.getFlashChipSize()) + " Bytes</p>";
            html += "</div>";
            
            html += "<div class='status-box'>";
            html += "<h2>Firmware</h2>";
            html += "<p><strong>Compile Date:</strong> " + String(__DATE__) + "</p>";
            html += "<p><strong>Compile Time:</strong> " + String(__TIME__) + "</p>";
            html += "<p><strong>SDK Version:</strong> " + String(ESP.getSdkVersion()) + "</p>";
            html += "</div>";
            
            html += "<div class='status-box'>";
            html += "<h2>Laufzeit</h2>";
            html += "<p><strong>Uptime:</strong> " + String(millis()/1000) + " Sekunden</p>";
            // Reboot Grund in lesbarer Form
            String resetReason;
            switch(esp_reset_reason()) {
                case ESP_RST_POWERON: resetReason = "Power On"; break;
                case ESP_RST_EXT: resetReason = "External Reset"; break;
                case ESP_RST_SW: resetReason = "Software Reset"; break;
                case ESP_RST_PANIC: resetReason = "Panic Reset"; break;
                case ESP_RST_INT_WDT: resetReason = "Interrupt Watchdog"; break;
                case ESP_RST_TASK_WDT: resetReason = "Task Watchdog"; break;
                case ESP_RST_WDT: resetReason = "Other Watchdog"; break;
                case ESP_RST_DEEPSLEEP: resetReason = "Deep Sleep"; break;
                case ESP_RST_BROWNOUT: resetReason = "Brownout"; break;
                case ESP_RST_SDIO: resetReason = "SDIO Reset"; break;
                default: resetReason = "Unbekannt (" + String(esp_reset_reason()) + ")"; break;
            }
            html += "<p><strong>Reboot Grund:</strong> " + resetReason + "</p>";
            html += "</div>";
            
            return html;
        }
    );
}

void simulateSensorReading() {
    if (debugMode) {
        // Simuliere Sensor-Daten
        float temp = 20.0 + random(-50, 150) / 10.0;
        float hum = 50.0 + random(-200, 200) / 10.0;
        int light = random(0, 1024);
        
        // Kalibrierung anwenden
        temp *= calibrationFactor;
        
        Serial.println("\n--- Sensor Reading ---");
        Serial.printf("Temperatur: %.1f °C\n", temp);
        Serial.printf("Luftfeuchtigkeit: %.1f %%\n", hum);
        Serial.printf("Lichtstärke: %d lux\n", light);
        Serial.printf("Interval: %d ms\n", sensorInterval);
    }
}

void printDebugInfo() {
    Serial.println("\n=== DEBUG INFO ===");
    Serial.printf("WiFi Status: %s\n", WiFi.status() == WL_CONNECTED ? "Verbunden" : "Getrennt");
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("SSID: %s\n", WiFi.SSID().c_str());
        Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
        Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
    }
    Serial.printf("Modus: %s\n", WiFi.getMode() == WIFI_STA ? "Station" : "Access Point");
    Serial.printf("Hostname: %s\n", wifiManager.getHostname().c_str());
    Serial.printf("Freier Heap: %d Bytes\n", ESP.getFreeHeap());
    Serial.printf("Uptime: %lu Sekunden\n", millis()/1000);
    Serial.printf("Device Name: %s\n", deviceName.c_str());
    Serial.println("==================");
}

// Interrupt-Handler für zusätzliche Tests (optional)
void IRAM_ATTR onButtonPress() {
    // Hier könnten zusätzliche Button-Handler implementiert werden
    // Der Reset-Button wird bereits vom WiFiWebManager verwaltet
}