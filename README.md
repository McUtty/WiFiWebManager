# WiFiWebManager Library v2.1.1

Eine umfassende ESP32-Library für WiFi-Verwaltung, Web-Interface und energieeffizientes Light Sleep Management.

## 🆕 Neue Features in v2.1.1

- **Light Sleep Mode**: Energieverbrauch von ~150mA auf ~20-40mA reduzieren
- **GPIO Wake-up Helper**: Einfache Konfiguration für Sensoren und Schalter
- **Wake-up Grund Analyse**: Intelligente Reaktion auf verschiedene Wake-up Events
- **NTP Code-Defaults**: NTP standardmäßig über Code aktivierbar
- **Wake-up Statistiken**: Optional verfügbare Logging-Funktionen

## ⚡ Light Sleep Mode

```cpp
WiFiWebManager wwm;

void setup() {
    // Light Sleep aktivieren (nur über Code, nicht WebUI)
    wwm.setDefaultLightSleep(true);
    wwm.setLightSleepTimer(100000); // 100ms Timer
    
    // GPIO Wake-ups für Sensoren
    wwm.addWakeupGPIO(2, GPIO_INTR_ANY_EDGE);  // Schalter
    wwm.addWakeupGPIO(5, GPIO_INTR_LOW_LEVEL);  // Sensor Alert
    
    wwm.begin();
}

void loop() {
    wwm.loop();
    
    // Intelligente Reaktion auf Wake-up Gründe
    if (wwm.wasWokenByGPIO()) {
        int pin = wwm.getWakeupGPIO();
        // Sofortige Reaktion auf GPIO-Events
    }
    
    if (wwm.wasWokenByTimer()) {
        // Regelmäßige Tasks (I2C/SPI Polling)
    }
}
```

## 🌐 Features

### WiFi Management
- Automatische WiFi-Verbindung mit Fallback auf Access Point
- Robuste Reconnect-Logik mit konfigurierbaren Versuchen
- Statische IP-Konfiguration
- Hostname-Management

### Web Interface
- Responsive Design für alle Geräte
- WiFi-Konfiguration über Browser
- NTP-Zeitkonfiguration
- OTA Firmware-Updates
- Reset-Funktionen (WiFi-only oder komplett)

### Erweiterte Features
- **Custom Data API**: Sichere Speicherung eigener Konfigurationsdaten
- **Debug-Modus**: Umfassendes Logging für Entwicklung
- **Hardware Reset-Button**: GPIO 0 für physischen Reset
- **mDNS Support**: .local Domain-Zugriff

### Energy Management (v2.1.1)
- **Light Sleep Mode**: WiFi bleibt aktiv, drastisch reduzierter Stromverbrauch
- **GPIO Wake-up**: Sofortige Reaktion auf externe Events
- **Timer Wake-up**: Konfigurierbare Intervalle für periodische Tasks
- **Wake-up Analyse**: Erkennung der Wake-up Ursache für intelligente Reaktionen

## 📦 Installation

### Arduino IDE
1. Library Manager öffnen (Sketch → Include Library → Manage Libraries)
2. Nach "WiFiWebManager" suchen
3. Version 2.1.1 installieren

### PlatformIO
```ini
[env:esp32]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps = 
    WiFiWebManager@^2.1.1
    ESPAsyncWebServer-esphome
```

## 🚀 Schnellstart

### Basis-Setup
```cpp
#include <WiFiWebManager.h>

WiFiWebManager wwm;

void setup() {
    Serial.begin(115200);
    
    // Optional: Hostname setzen
    wwm.setDefaultHostname("MeinESP32");
    
    // Optional: NTP aktivieren
    wwm.setDefaultNTP(true, "pool.ntp.org");
    
    wwm.begin();
}

void loop() {
    wwm.loop();
    
    // Ihre Anwendungslogik hier...
}
```

### Mit Light Sleep (v2.1.1)
```cpp
#include <WiFiWebManager.h>

WiFiWebManager wwm;

void setup() {
    Serial.begin(115200);
    
    // Energy-Efficient Setup
    wwm.setDefaultLightSleep(true);         // Light Sleep aktivieren
    wwm.setLightSleepTimer(500000);         // 500ms für I2C-Polling
    wwm.addWakeupGPIO(2, GPIO_INTR_ANY_EDGE); // Sensor-Pin
    
    wwm.begin();
}

void loop() {
    wwm.loop();
    
    // Wake-up basierte Logik
    if (wwm.wasWokenByGPIO()) {
        handleSensorEvent(wwm.getWakeupGPIO());
    }
    
    if (wwm.wasWokenByTimer()) {
        readSensors(); // Regelmäßige I2C/SPI Abfragen
    }
}
```

## 📖 API Referenz

### Basis-Methoden
```cpp
void begin();                                    // Library initialisieren
void loop();                                     // In main loop() aufrufen
void reset();                                    // Kompletter Reset
```

### WiFi & Netzwerk
```cpp
void setDefaultHostname(const String& hostname); // Standard-Hostname
String getHostname();                            // Aktueller Hostname
```

### NTP Konfiguration (v2.1.1)
```cpp
void setDefaultNTP(bool enabled, const String& server = "pool.ntp.org");
```

### Light Sleep Management (v2.1.1)
```cpp
void setDefaultLightSleep(bool enabled);                    // Light Sleep aktivieren
void setLightSleepTimer(uint64_t microseconds = 100000);    // Timer-Intervall
void addWakeupGPIO(int pin, gpio_int_type_t mode);          // GPIO Wake-up
void removeWakeupGPIO(int pin);                             // GPIO entfernen
void clearAllWakeupGPIOs();                                 // Alle GPIOs löschen
```

### Wake-up Analyse (v2.1.1)
```cpp
esp_sleep_wakeup_cause_t getLastWakeupCause();  // Wake-up Ursache
String getWakeupCauseString();                  // Human-readable String
int getWakeupGPIO();                            // Welcher GPIO hat geweckt
bool wasWokenByTimer();                         // Timer Wake-up?
bool wasWokenByGPIO();                          // GPIO Wake-up?
bool wasWokenByWiFi();                          // WiFi Wake-up?
```

### Wake-up Statistiken (v2.1.1)
```cpp
void enableWakeupLogging(bool enabled = true);  // Logging aktivieren
void clearWakeupStats();                        // Statistiken zurücksetzen
WakeupStats getWakeupStats();                   // Statistiken abrufen
```

### Custom Data API
```cpp
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
```

### Custom Web Pages
```cpp
using ContentHandler = std::function<String(AsyncWebServerRequest*)>;

void addPage(const String& menutitle, const String& path, 
             ContentHandler getHandler, ContentHandler postHandler = nullptr);
void removePage(const String& path);
```

### Debug & Entwicklung
```cpp
void setDebugMode(bool enabled);    // Debug-Ausgaben aktivieren
bool getDebugMode();                // Debug-Status abfragen
```

## 🔧 Konfiguration

### Hardware Reset-Button
- **GPIO 0** (Boot-Button) als Reset-Pin
- **3-10 Sekunden**: Nur WiFi-Daten löschen
- **>10 Sekunden**: Kompletter Werks-Reset

### Web Interface
- **Setup-Modus**: `http://192.168.4.1` (Access Point)
- **Normal-Modus**: IP-Adresse des ESP32
- **mDNS**: `http://hostname.local` (falls Hostname gesetzt)

### Energieverbrauch (v2.1.1)
| Modus | Verbrauch | WiFi | WebServer | Wake-up Zeit |
|-------|-----------|------|-----------|--------------|
| Normal | 150-300mA | ✓ | ✓ | - |
| Light Sleep | 20-40mA | ✓ | ✓ | ~3ms |

## 🛠 Beispiele

### Einfacher Schalter-Monitor
```cpp
#include <WiFiWebManager.h>

WiFiWebManager wwm;
const int SWITCH_PIN = 2;

void setup() {
    Serial.begin(115200);
    pinMode(SWITCH_PIN, INPUT_PULLUP);
    
    // Energy-efficient setup
    wwm.setDefaultLightSleep(true);
    wwm.addWakeupGPIO(SWITCH_PIN, GPIO_INTR_ANY_EDGE);
    
    wwm.begin();
}

void loop() {
    wwm.loop();
    
    if (wwm.wasWokenByGPIO() && wwm.getWakeupGPIO() == SWITCH_PIN) {
        Serial.println("Schalter betätigt!");
        // Sofortige Reaktion ohne Sleep-Verzögerung
    }
}
```

### I2C Sensor mit Polling
```cpp
#include <WiFiWebManager.h>
#include <Wire.h>

WiFiWebManager wwm;

void setup() {
    Serial.begin(115200);
    Wire.begin();
    
    // 1-Sekunden Polling für I2C Sensor
    wwm.setDefaultLightSleep(true);
    wwm.setLightSleepTimer(1000000); // 1 Sekunde
    
    wwm.begin();
}

void loop() {
    wwm.loop();
    
    if (wwm.wasWokenByTimer()) {
        // I2C Sensor alle 1 Sekunde auslesen
        float temperature = readTemperatureSensor();
        Serial.printf("Temperatur: %.1f°C\n", temperature);
    }
}
```

## 📋 Kompatibilität

- **ESP32**: Alle Varianten (ESP32, ESP32-S2, ESP32-S3, ESP32-C3)
- **Arduino IDE**: 1.8.19+
- **ESP32 Core**: 2.0.0+
- **PlatformIO**: Vollständig unterstützt

## 🔄 Migration von v2.0.x

Version 2.1.1 ist **vollständig rückwärtskompatibel**. Bestehender Code funktioniert ohne Änderungen.

Neue Features sind optional und müssen explizit aktiviert werden:

```cpp
// Alt (funktioniert weiterhin):
wwm.begin();

// Neu (optional):
wwm.setDefaultLightSleep(true);  // Nur hinzufügen für Light Sleep
wwm.begin();
```

## 📄 Lizenz

MIT License - siehe LICENSE Datei für Details.

## 🤝 Beitragen

Contributions sind willkommen! Bitte erstellen Sie Issues oder Pull Requests auf GitHub.

## 📞 Support

- **GitHub Issues**: Für Bugs und Feature-Requests
- **Dokumentation**: Siehe examples/ Ordner
- **Community**: Arduino Forum ESP32 Sektion