# Changelog

Alle wichtigen Änderungen an der WiFiWebManager Library werden in dieser Datei dokumentiert.

Das Format basiert auf [Keep a Changelog](https://keepachangelog.com/de/1.0.0/),
und dieses Projekt folgt [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.1.1] - 2024-12-XX

### ✨ Neue Features

#### Light Sleep Mode
- **Energieeffizienz**: Reduzierung des Stromverbrauchs von ~150mA auf ~20-40mA
- **WiFi bleibt aktiv**: Volle Web-Interface Funktionalität während Light Sleep
- **Sofortige Reaktion**: Wake-up Zeit von nur ~3ms
- **Code-only Konfiguration**: Keine WebUI-Einstellungen, nur Entwickler-Kontrolle

#### GPIO Wake-up Management
- `addWakeupGPIO()`: Einfache Konfiguration von GPIO Wake-up Sources
- `removeWakeupGPIO()`: Entfernen einzelner GPIO Wake-ups
- `clearAllWakeupGPIOs()`: Alle GPIO Wake-ups auf einmal löschen
- Unterstützung für alle GPIO Interrupt-Modi (Edge/Level)

#### Wake-up Grund Analyse
- `getLastWakeupCause()`: ESP32 Wake-up Ursache abfragen
- `getWakeupCauseString()`: Human-readable Wake-up Grund
- `getWakeupGPIO()`: Welcher GPIO hat das Wake-up ausgelöst
- `wasWokenByTimer()`, `wasWokenByGPIO()`, `wasWokenByWiFi()`: Convenience-Methoden

#### Wake-up Statistiken
- `enableWakeupLogging()`: Optionale Statistik-Erfassung
- `getWakeupStats()`: Detaillierte Wake-up Statistiken
- `clearWakeupStats()`: Statistiken zurücksetzen
- Aufschlüsselung nach Wake-up Typen (Timer, GPIO, WiFi, etc.)

#### NTP Code-Defaults
- `setDefaultNTP()`: NTP standardmäßig über Code aktivierbar
- WebUI zeigt Code-Defaults an
- Nutzer kann weiterhin über WebUI überschreiben
- Fallback auf Code-Defaults nach Reset

### 🔧 API Erweiterungen

```cpp
// Light Sleep Konfiguration
void setDefaultLightSleep(bool enabled);
void setLightSleepTimer(uint64_t microseconds = 100000);

// GPIO Wake-up Management
void addWakeupGPIO(int pin, gpio_int_type_t mode = GPIO_INTR_ANY_EDGE);
void removeWakeupGPIO(int pin);
void clearAllWakeupGPIOs();

// Wake-up Analyse
esp_sleep_wakeup_cause_t getLastWakeupCause();
String getWakeupCauseString();
int getWakeupGPIO();
bool wasWokenByTimer();
bool wasWokenByGPIO();
bool wasWokenByWiFi();

// Wake-up Statistiken
void enableWakeupLogging(bool enabled = true);
void clearWakeupStats();
WakeupStats getWakeupStats();

// NTP Code-Defaults
void setDefaultNTP(bool enabled, const String& server = "pool.ntp.org");
```

### 🎯 Anwendungsfälle

#### IoT Sensoren mit Batteriebetrieb
- Drastisch verlängerte Batterielaufzeit durch Light Sleep
- GPIO Wake-up für sofortige Sensor-Reaktion
- Timer Wake-up für regelmäßige I2C/SPI Abfragen

#### Smart Home Geräte
- Tür-/Fenstersensoren mit Reed-Kontakten
- Bewegungsmelder mit Interrupt-Ausgang
- Temperatur-/Luftfeuchtigkeitssensoren mit Alert-Pins

#### Industrielle Überwachung
- Maschinenstatus-Überwachung
- Prozessalarm-Systeme
- Energieeffiziente Datenlogger

### 🌐 WebUI Verbesserungen
- **Home-Seite**: Anzeige des Light Sleep Status und Energieverbrauchs
- **NTP-Seite**: Code-Default Informationen werden angezeigt
- **Update-Seite**: Library-Version wird angezeigt
- **Wake-up Statistiken**: Optional auf Home-Seite sichtbar

### 📚 Neue Beispiele
- **Basic.ino**: Minimale Nutzung ohne neue Features
- **LightSleep.ino**: Umfassendes Light Sleep Beispiel mit allen Features
- **CustomPages.ino**: Erweiterte Custom Web-Pages mit persistenter Konfiguration

### 🔒 Kompatibilität
- **Vollständig rückwärtskompatibel**: Bestehender Code funktioniert unverändert
- **Keine Breaking Changes**: Alle v2.0.x APIs bleiben verfügbar
- **Optionale Features**: Neue Funktionen müssen explizit aktiviert werden

### 📋 Interne Verbesserungen
- Erweiterte Debug-Ausgaben für Light Sleep Funktionen
- Optimierte Memory-Nutzung für Wake-up Management
- Robuste Error-Behandlung bei ungültigen GPIO-Pins
- Verbesserte Code-Dokumentation und Kommentare

---

## [2.0.1] - 2024-XX-XX

### 🔧 Verbesserungen
- Verbesserte WiFi-Reconnect Logik
- Stabileres Boot-Attempt Management
- Optimierte Web-Interface Performance

### 🐛 Fehlerbehebungen
- WiFi Verbindungsprobleme nach Power-Cycle
- Reset-Button Timing-Probleme
- Memory Leaks bei längeren Laufzeiten

---

## [2.0.0] - 2024-XX-XX

### ✨ Neue Features
- **Responsive Web-Interface**: Optimiert für Desktop, Tablet und Mobile
- **Hardware Reset-Button**: GPIO 0 für physischen Reset
- **Boot-Attempt Management**: Automatischer Fallback nach fehlgeschlagenen Verbindungen
- **Custom Data API**: Sichere Speicherung anwendungsspezifischer Konfiguration
- **Custom Web-Pages**: Einfache Integration eigener Konfigurationsseiten
- **mDNS Support**: .local Domain-Zugriff
- **Erweiterte Netzwerk-Optionen**: Statische IP, DNS, Gateway Konfiguration

### 🌐 Web-Interface
- Modernes, responsives Design
- Übersichtliche Navigation
- Echtzeit WiFi-Netzwerk Scan
- OTA Firmware-Update Interface
- Umfassende System-Informationen

### 🔧 API-Verbesserungen
- Vereinfachte Initialisierung
- Callback-basierte Custom Pages
- Typsichere Custom Data Speicherung
- Umfassende Debug-Optionen

### 📦 Abhängigkeiten
- ESPAsyncWebServer für moderne Web-Funktionalität
- ArduinoOTA für Firmware-Updates
- Preferences für persistente Konfiguration

---

## [1.x.x] - Legacy Versionen

### Basis-Funktionalität
- Grundlegende WiFi-Verwaltung
- Einfaches Web-Interface
- AP-Fallback Modus
- Basis-Konfigurationsspeicherung

---

## 🔮 Geplante Features

### v2.2.0 (Geplant)
- **Deep Sleep Support**: Für ultra-niedrigen Stromverbrauch
- **Bluetooth Configuration**: Setup ohne WiFi
- **Cloud Integration**: MQTT, REST APIs
- **Advanced Scheduling**: Zeitbasierte Aktionen
- **Sensor Drivers**: Integrierte I2C/SPI Sensor-Unterstützung

### v3.0.0 (Zukunft)
- **Multi-Core Support**: Optimierung für ESP32 Dual-Core
- **Real-Time Analytics**: Live-Monitoring Dashboard
- **Plugin-System**: Modulare Erweiterungen
- **AI Integration**: Intelligente Automatisierung

---

## 📞 Unterstützung

- **GitHub Issues**: Bug-Reports und Feature-Requests
- **Dokumentation**: Siehe README.md und examples/
- **Community**: Arduino Forum ESP32 Sektion

## 📄 Lizenz

Alle Versionen stehen unter MIT Lizenz - siehe LICENSE Datei für Details.