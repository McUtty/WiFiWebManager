# WiFiWebManager

Ein umfassendes ESP32-Framework für WLAN-Management mit Web-Interface, das robuste Verbindungsverwaltung, erweiterbares Web-UI und persistente Datenspeicherung bietet.

## 🚀 Features

- **Intelligente WLAN-Verbindung**: 3-Versuch-System mit automatischem Fallback zu AP-Modus
- **Reset-Button Support**: Hardware-Reset über GPIO 0 (3s = WLAN Reset, 10s = Vollreset)
- **Automatischer Reconnect**: Überwachung und Wiederherstellung verlorener Verbindungen
- **Erweiterbares Web-Interface**: Einfaches Hinzufügen eigener Konfigurationsseiten
- **Custom Data API**: Persistente Speicherung verschiedener Datentypen
- **Debug-Modus**: Ein/ausschaltbare Debug-Ausgaben für Entwicklung
- **OTA-Updates**: Firmware-Updates über Web-Interface, mit Anzeige der App-Version auf der Update-Seite (`setFirmwareVersion`)
- **Responsive Design**: Modernes Web-UI für Desktop und Mobile

## 📦 Installation

### Arduino IDE Library Manager
1. Öffnen Sie Arduino IDE
2. Gehen Sie zu **Sketch** > **Include Library** > **Manage Libraries**
3. Suchen Sie nach "WiFiWebManager"
4. Klicken Sie auf **Install**

### Manuelle Installation
1. Laden Sie die neueste Version von [Releases](https://github.com/McUtty/WiFiWebManager/releases)
2. Entpacken Sie die ZIP-Datei in Ihren Arduino/libraries Ordner
3. Starten Sie Arduino IDE neu

## 🛠️ Abhängigkeiten

Dieses Framework benötigt:
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

    // Optional: Version, die auf der /update-Seite angezeigt wird
    wifiManager.setFirmwareVersion("1.0.0");
    
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
- `/` - Status und Übersicht (kann vom Inhalt angepasst werden)
- `/wlan` - WLAN-Konfiguration (fix)
- `/ntp` - NTP-Einstellungen (fix)
- `/update` - OTA-Firmware-Update; zeigt Version (fix)
- `/reset` - Reset-Optionen (fix)

## 🔧 Erweiterte Nutzung

### Eigene Seiten hinzufügen (Die Namen für die Standard-Seiten sind reserviert)

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

### Custom Data verwenden (Key max. 13 Zeichen)

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

### Firmware-Version
```cpp
void setFirmwareVersion(const String& version);   // Version für /update-Seite setzen
```
Einmal in `setup()` aufrufen, damit die Firmware-Version im Web-UI sichtbar ist:
```cpp
wifiManager.setFirmwareVersion("1.0.0");
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
std::vector<String> getCustomDataKeys();  // Alle gespeicherten Custom-Keys auflisten
```

## ⚠️ Wichtige Hinweise

- **WICHTIG**: `wifiManager.loop()` muss in der `loop()` Funktion aufgerufen werden
- **Custom Data**: Verwenden Sie keine reservierten Schlüssel (`ssid`, `pwd`, `hostname`, etc.)
- **Performance**: Debug-Modus nur bei Bedarf aktivieren
- **Reset-Button**: GPIO 0 ist standardmäßig der Boot-Button auf den meisten ESP32-Boards

## 🔗 Beispiele

Siehe `/examples` Ordner für vollständige Beispiele:
- `Basic` - Grundlegende Nutzung
- `Test` - Custom Pages, Custom Data und Debug-Ausgaben (Demo mit simulierten Werten)

## 📄 Lizenz

MIT License - siehe [LICENSE](LICENSE) für Details.

## 🤝 Beitragen

Beiträge sind willkommen! Bitte eröffnen Sie ein Issue oder einen Pull Request auf GitHub.

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/McUtty/WiFiWebManager/issues)
- **Diskussionen**: [GitHub Discussions](https://github.com/McUtty/WiFiWebManager/discussions)

## 📊 Systemanforderungen

- **Hardware**: ESP32 (alle Varianten)
- **RAM**: ~50KB für Framework + Webserver
- **Flash**: ~200KB für Code + Web-Assets
- **Arduino Core**: ESP32 v2.0.0 oder höher

# WiFiWebManager Library - Funktions Referenz

## 📋 Grundlegende Methoden

| Funktion | Beschreibung | Parameter | Rückgabe |
|----------|--------------|-----------|----------|
| `WiFiWebManager()` | Konstruktor - initialisiert Reset-Button (GPIO 0) | - | - |
| `begin()` | Startet WiFiWebManager, verbindet WiFi oder startet AP | - | `void` |
| `loop()` | Muss in main loop() aufgerufen werden | - | `void` |
| `reset()` | Führt kompletten Werks-Reset durch | - | `void` |

## 🌐 Netzwerk-Konfiguration

| Funktion | Beschreibung | Parameter | Rückgabe |
|----------|--------------|-----------|----------|
| `setDefaultHostname(hostname)` | Setzt Standard-Hostname aus Code | `String hostname` | `void` |
| `getHostname()` | Gibt aktuellen Hostname zurück | - | `String` |
| `setFirmwareVersion(version)` | App-Version auf der `/update`-Seite | `String version` | `void` |

> Werks-Reset erfolgt über die öffentliche Methode `reset()` (siehe Grundlegende Methoden). WLAN-only- bzw. Voll-Reset lassen sich zusätzlich über den Hardware-Reset-Button (GPIO 0) auslösen.

## 📄 Custom Pages (Webseiten)

| Funktion | Beschreibung | Parameter | Rückgabe |
|----------|--------------|-----------|----------|
| `addPage(title, path, getHandler)` | Fügt GET-only Seite hinzu | `String title, String path, ContentHandler getHandler` | `void` |
| `addPage(title, path, getHandler, postHandler)` | Fügt Seite mit GET und POST hinzu | `String title, String path, ContentHandler getHandler, ContentHandler postHandler` | `void` |
| `removePage(path)` | Entfernt Custom Page | `String path` | `void` |


## 💾 Custom Data API

### Speichern (Setter)

| Funktion | Beschreibung | Parameter | Einschränkungen |
|----------|--------------|-----------|-----------------|
| `saveCustomData(key, value)` | Speichert String-Wert | `String key, String value` | **Key max. 13 Zeichen** |
| `saveCustomData(key, value)` | Speichert Integer-Wert | `String key, int value` | **Key max. 13 Zeichen** |
| `saveCustomData(key, value)` | Speichert Boolean-Wert | `String key, bool value` | **Key max. 13 Zeichen** |
| `saveCustomData(key, value)` | Speichert Float-Wert | `String key, float value` | **Key max. 13 Zeichen** |

### Laden (Getter)

| Funktion | Beschreibung | Parameter | Rückgabe |
|----------|--------------|-----------|----------|
| `loadCustomData(key, defaultValue)` | Lädt String-Wert | `String key, String defaultValue = ""` | `String` |
| `loadCustomDataInt(key, defaultValue)` | Lädt Integer-Wert | `String key, int defaultValue = 0` | `int` |
| `loadCustomDataBool(key, defaultValue)` | Lädt Boolean-Wert | `String key, bool defaultValue = false` | `bool` |
| `loadCustomDataFloat(key, defaultValue)` | Lädt Float-Wert | `String key, float defaultValue = 0.0` | `float` |

### Verwaltung

| Funktion | Beschreibung | Parameter | Rückgabe |
|----------|--------------|-----------|----------|
| `hasCustomData(key)` | Prüft ob Key existiert | `String key` | `bool` |
| `removeCustomData(key)` | Löscht gespeicherten Wert | `String key` | `void` |
| `getCustomDataKeys()` | Gibt alle Custom Keys zurück | - | `std::vector<String>` |

## 🛠️ Debug & Utilities

| Funktion | Beschreibung | Parameter | Rückgabe |
|----------|--------------|-----------|----------|
| `setDebugMode(enabled)` | Aktiviert/deaktiviert Debug-Ausgaben | `bool enabled` | `void` |
| `getDebugMode()` | Gibt Debug-Status zurück | - | `bool` |

## ⚠️ Wichtige Einschränkungen

### 🔑 Key-Einschränkungen
- **Maximale Länge: 13 Zeichen**
- **Reservierte Keys** (können nicht verwendet werden):
  - `ssid`, `pwd`, `hostname`
  - `useStaticIP`, `ip`, `gateway`, `subnet`, `dns`
  - `ntpEnable`, `ntpServer`, `bootAttempts`

### 🔄 Boot-Attempt System
- **Max. 3 Verbindungsversuche** bei WiFi-Fehlern
- Nach 3 fehlgeschlagenen Versuchen → automatischer AP-Modus
- Erfolgreiche Verbindung setzt Counter zurück

### 🔧 Hardware Reset-Button (GPIO 0)
| Druckdauer | Aktion |
|------------|--------|
| **3-10 Sekunden** | Nur WiFi-Daten löschen |
| **>10 Sekunden** | Kompletter Werks-Reset |

## 📝 Beispiel-Code

```cpp
#include "WiFiWebManager.h"

WiFiWebManager wwm;

// Custom Page Handler
String handleMyPage(AsyncWebServerRequest *request) {
    return "<h1>Meine Seite</h1><p>Status: " + 
           wwm.loadCustomData("status", "OK") + "</p>";
}

void setup() {
    Serial.begin(115200);
    
    // Hostname setzen
    wwm.setDefaultHostname("MeinESP32");
    
    // Debug aktivieren
    wwm.setDebugMode(true);
    
    // Custom Page hinzufügen
    wwm.addPage("Status", "/status", handleMyPage);
    
    // Custom Data speichern (Key max. 13 Zeichen!)
    wwm.saveCustomData("temp_max", 25.5f);
    wwm.saveCustomData("alerts", true);
    wwm.saveCustomData("count", 42);
    
    wwm.begin();
}

void loop() {
    wwm.loop();
    
    // Custom Data laden
    float maxTemp = wwm.loadCustomDataFloat("temp_max", 20.0f);
    bool alertsOn = wwm.loadCustomDataBool("alerts", false);
}
```

## 🌍 Standard-Webseiten

Das System stellt automatisch folgende Seiten bereit:

- **/** - Home/Status-Übersicht
- **/wlan** - WiFi-Konfiguration
- **/ntp** - NTP-Zeitserver Einstellungen
- **/update** - OTA Firmware-Update
- **/reset** - Reset-Optionen
