# ESP32 WiFiWebManager

Ein vielseitiges, modernes ESP32-Webkonfigurationsmodul mit WLAN-, Hostname-, IP- und NTP-Verwaltung, OTA-Update und Werksreset.  
**Optimiert für die Integration in eigene Projekte – vollständig steuerbar per Webinterface.**

---

## Features

- **WLAN-Konfiguration** mit automatischer Umschaltung zwischen Access-Point (AP) und Client (STA)
- **SSID-Auswahl** (Dropdown im Webinterface)
- **Hostname (Gerätename) einstellbar**
- **Optionale statische IP-Konfiguration**
- **Responsives, modernes Webinterface** – optimiert für Mobilgeräte und Desktop, zentriert & mit Menü
- **Menüstruktur:** WLAN, NTP, Firmware-Update, Werksreset
- **NTP (Zeitsynchronisierung)** bequem im Webinterface einstellbar
- **OTA-Firmware-Update** direkt per Web (Dateiupload)
- **Werksreset per Webinterface**
- **Alle Statusmeldungen auf dem seriellen Monitor**
- **Sehr einfach zu integrieren:** Nur `begin()` und `loop()` im Hauptprogramm nötig
- **Erweiterbar für eigene Felder/Seiten**

---

## Getting Started

### 1. **Bibliotheken installieren**

- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
- [AsyncTCP](https://github.com/me-no-dev/AsyncTCP)
- [ArduinoOTA](https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA) (bei ESP32-Boards meist enthalten)

### 2. **Dateien einfügen**

Lege diese Dateien in deinen Sketch-Ordner:

- `WiFiWebManager.h`
- `WiFiWebManager.cpp`
- `main.ino` (siehe Beispiel)

---

### 3. **Hauptprogramm (`main.ino`)**

```cpp
#include "WiFiWebManager.h"

WiFiWebManager wwm;

void setup() {
    Serial.begin(115200);
    wwm.begin();  // Startet alles: WLAN, Webinterface, OTA etc.
}

void loop() {
    wwm.loop();   // Muss regelmäßig im Hauptloop laufen
}
```

# Webinterface

- **WLAN, Hostname, statische IP**: Alles bequem im Browser einstellen  
- **NTP-Server und -Aktivierung** separat im Menüpunkt „NTP“ konfigurieren  
- **Firmware-Update** direkt per Web-Upload (Menüpunkt „Firmware“)  
- **Reset**: Setzt alle Einstellungen auf Werkseinstellungen zurück

**Erster Start:**  
ESP32 startet als Access-Point `ESP32_SETUP` (IP: `192.168.4.1`).  
Nach dem Speichern von WLAN-Daten verbindet sich das Modul automatisch als Client.

---

## Methodenübersicht (`WiFiWebManager`)

| Methode    | Beschreibung                                                |
|------------|-------------------------------------------------------------|
| `begin()`  | Startet das gesamte Modul, liest Einstellungen, startet Webserver  |
| `loop()`   | Muss im Arduino-Loop laufen, verarbeitet Systemfunktionen   |
| `reset()`   | Löscht alle Einstellungen und startet das Board neu   |


**Alles Weitere steuerst du über das Webinterface.**  
Wenn du Funktionen wie Werksreset, neue Seiten oder spezielle Einstellungen im Code triggern möchtest, kannst du die Klasse leicht erweitern.

---

## Typische Erweiterung (Beispiel: Werksreset per Taster)


So kannst du im Sketch z.B. mit einem Hardware-Button einen Reset auslösen:

```cpp
#define RESET_PIN  0  // z.B. GPIO0 (je nach Board anpassen)

#include "WiFiWebManager.h"

WiFiWebManager wwm;

void setup() {
    Serial.begin(115200);
    pinMode(RESET_PIN, INPUT_PULLUP);
    wwm.begin();
}

void loop() {
    wwm.loop();

    // Hardware-Reset: Taster gegen GND drücken
    if (digitalRead(RESET_PIN) == LOW) {
        Serial.println("Hardware-Reset ausgelöst!");
        wwm.reset();
        delay(1000);  // Entprellen & Reset nicht mehrfach auslösen
    }
}
```


## Alle enthaltenen Features im Überblick

- Automatische AP/STA-Umschaltung
- SSID-Auswahl
- Hostname und statische IP
- NTP-Zeitserver und Aktivierung
- OTA-Update (Firmware per Web hochladen)
- Werksreset
- Modernes, responsives Webdesign mit Menü
- Statusausgabe über Serial
- Preferences als Datenspeicher (für Erweiterungen vorbereitet)
- Einfache Integration in beliebige Projekte


# Lizenz

MIT License

Copyright (c) 2024

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
