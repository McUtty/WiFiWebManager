# WiFiWebManager

Eine umfassende ESP32-Bibliothek für WLAN-Management mit Web-Interface, die robuste Verbindungsverwaltung, erweiterbares Web-UI und persistente Datenspeicherung bietet.

## 🚀 Features

- **Intelligente WLAN-Verbindung**: 3-Versuch-System mit automatischem Fallback zu AP-Modus
- **Reset-Button Support**: Hardware-Reset über GPIO 0 (3s = WLAN Reset, 10s = Vollreset)
- **Automatischer Reconnect**: Überwachung und Wiederherstellung verlorener Verbindungen
- **Erweiterbares Web-Interface**: Einfaches Hinzufügen eigener Konfigurationsseiten
- **Custom Data API**: Persistente Speicherung verschiedener Datentypen
- **Debug-Modus**: Ein/ausschaltbare Debug-Ausgaben für Entwicklung
- **OTA-Updates**: Firmware-Updates über Web-Interface
- **Responsive Design**: Modernes Web-UI für Desktop und Mobile

## 📦 Installation

### Arduino IDE Library Manager
1. Öffnen Sie Arduino IDE
2. Gehen Sie zu **Sketch** > **Include Library** > **Manage Libraries**
3. Suchen Sie nach "WiFiWebManager"
4. Klicken Sie auf **Install**

### Manuelle Installation
1. Laden Sie die neueste Version von [Releases](https://github.com/IhrUsername/WiFiWebManager/releases)
2. Entpacken Sie die ZIP-Datei in Ihren Arduino/libraries Ordner
3. Starten Sie Arduino IDE neu

## 🛠️ Abhängigkeiten

Diese Bibliothek benötigt:
- **ESPAsyncWebServer** (wird automatisch installiert)
- **AsyncTCP** (Abhängigkeit von ESPAsyncWebServer)

## 📖 Quick Start

```cpp
#include <WiFiWebManager.h>

WiFiWebManager wifiManager;

void setup() {
    Serial.begin(115200);
    
    // Optional: Debug-Modus aktivieren
    wifiManager.setDebugMode(true);
    
    // Optional: Standard-Hostname setzen
    wifiManager.setDefaultHostname("MeinESP32");
    
    wifiManager.begin();
}

void loop() {
    wifiManager.loop(); // WICHTIG: Muss aufgerufen werden!
}
```

## 🌐 Web-Interface

Nach dem Start ist das Web-Interface verfügbar unter:
- **WLAN-Modus**: IP-Adresse des ESP32
- **AP-Modus**: http://192.168.4.1

### Standard-Seiten:
- `/` - Status und Übersicht
- `/wlan` - WLAN-Konfiguration
- `/ntp` - NTP-Einstellungen  
- `/update` - Firmware-Update
- `/reset` - Reset-Optionen

## 🔧 Erweiterte Nutzung

### Eigene Seiten hinzufügen

```cpp
// Einfache GET-Seite
wifiManager.addPage("Meine Seite", "/custom", 
    [](AsyncWebServerRequest *request) -> String {
        return "<h1>Benutzerdefinierte Seite</h1><p>Ihr Inhalt hier</p>";
    }
);

// Seite mit GET und POST
wifiManager.addPage("Einstellungen", "/settings", 
    // GET Handler
    [](AsyncWebServerRequest *request) -> String {
        String html = "<h1>Einstellungen</h1>";
        html += "<form action='/settings' method='POST'>";
        html += "<input name='wert' placeholder='Wert eingeben'>";
        html += "<input type='submit' value='Speichern'>";
        html += "</form>";
        return html;
    },
    // POST Handler
    [](AsyncWebServerRequest *request) -> String {
        String wert = request->getParam("wert", true)->value();
        // Wert verarbeiten...
        return "<p>Gespeichert: " + wert + "</p><a href='/settings'>Zurück</a>";
    }
);
```

### Custom Data verwenden

```cpp
// Verschiedene Datentypen speichern
wifiManager.saveCustomData("deviceName", "Sensor1");        // String
wifiManager.saveCustomData("interval", 5000);               // int
wifiManager.saveCustomData("enabled", true);                // bool
wifiManager.saveCustomData("calibration", 1.25f);           // float

// Daten laden mit Default-Werten
String name = wifiManager.loadCustomData("deviceName", "Standard");
int interval = wifiManager.loadCustomDataInt("interval", 1000);
bool enabled = wifiManager.loadCustomDataBool("enabled", false);
float calib = wifiManager.loadCustomDataFloat("calibration", 1.0);

// Existenz prüfen und löschen
if (wifiManager.hasCustomData("oldValue")) {
    wifiManager.removeCustomData("oldValue");
}
```

## 🔘 Reset-Button (GPIO 0)

Verbinden Sie einen Taster zwischen GPIO 0 und GND:
- **3-10 Sekunden drücken**: Nur WLAN-Daten löschen
- **>10 Sekunden drücken**: Kompletter Werks-Reset

## 🐛 Debug-Modus

```cpp
// Debug-Modus aktivieren (nur über Code möglich)
wifiManager.setDebugMode(true);

// Status abfragen
bool isDebugActive = wifiManager.getDebugMode();
```

## 📋 API-Referenz

### Basis-Funktionen
```cpp
void begin();                    // Initialisierung
void loop();                     // Hauptschleife (in loop() aufrufen!)
void reset();                    // Software-Reset
```

### Hostname-Management
```cpp
void setDefaultHostname(const String& hostname);  // Standard setzen
String getHostname();                             // Aktuellen abrufen
```

### Debug-Funktionen
```cpp
void setDebugMode(bool enabled);  // Debug ein/aus
bool getDebugMode();              // Status abrufen
```

### Seiten-Management
```cpp
void addPage(const String& titel, const String& pfad, 
             ContentHandler getHandler, 
             ContentHandler postHandler = nullptr);
void removePage(const String& pfad);
```

### Custom Data API
```cpp
// Speichern
void saveCustomData(const String& key, const String& value);
void saveCustomData(const String& key, int value);
void saveCustomData(const String& key, bool value);
void saveCustomData(const String& key, float value);

// Laden
String loadCustomData(const String& key, const String& defaultValue = "");
int loadCustomDataInt(const String& key, int defaultValue = 0);
bool loadCustomDataBool(const String& key, bool defaultValue = false);
float loadCustomDataFloat(const String& key, float defaultValue = 0.0);

// Verwaltung
bool hasCustomData(const String& key);
void removeCustomData(const String& key);
```

## ⚠️ Wichtige Hinweise

- **WICHTIG**: `wifiManager.loop()` muss in der `loop()` Funktion aufgerufen werden
- **Custom Data**: Verwenden Sie keine reservierten Schlüssel (`ssid`, `pwd`, `hostname`, etc.)
- **Performance**: Debug-Modus nur bei Bedarf aktivieren
- **Reset-Button**: GPIO 0 ist standardmäßig der Boot-Button auf den meisten ESP32-Boards

## 🔗 Beispiele

Siehe `/examples` Ordner für vollständige Beispiele:
- `Basic` - Grundlegende Nutzung
- `CustomPages` - Eigene Web-Seiten
- `SensorData` - IoT-Sensor mit Konfiguration
- `SmartSwitch` - Smart Home Gerät

## 📄 Lizenz

MIT License - siehe [LICENSE](LICENSE) für Details.

## 🤝 Beitragen

Beiträge sind willkommen! Bitte lesen Sie [CONTRIBUTING.md](CONTRIBUTING.md) für Details.

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/IhrUsername/WiFiWebManager/issues)
- **Diskussionen**: [GitHub Discussions](https://github.com/IhrUsername/WiFiWebManager/discussions)

## 📊 Systemanforderungen

- **Hardware**: ESP32 (alle Varianten)
- **RAM**: ~50KB für Bibliothek + Webserver
- **Flash**: ~200KB für Code + Web-Assets
- **Arduino Core**: ESP32 v2.0.0 oder höher