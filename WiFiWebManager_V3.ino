#include <WiFiWebManager.h>

WiFiWebManager wifiWebManager;

void setup() {
    Serial.begin(115200);
    wifiWebManager.begin();

    // Beispielseite "Startseite" (nur GET)
    wifiWebManager.addPage(
        "Home", "/",
        [](AsyncWebServerRequest *request) {
            return "<h1>Info</h1><p>ESP32 Webmodul: Das ist die editierbare Startseite!</p>";
        }
    );

    // Beispielseite "Info" (nur GET)
    wifiWebManager.addPage(
        "Info", "/info",
        [](AsyncWebServerRequest *request) {
            return "<h1>Info</h1><p>ESP32 Webmodul: Alles läuft – auch mit Umlauten: äöüß!</p>";
        }
    );

    // Beispielseite "Zähler" (GET und POST)
    wifiWebManager.addPage(
        "Zähler", "/counter",
        [](AsyncWebServerRequest *request) {
            String val = wifiWebManager.loadCustomData("counter", "0");
            return "<h1>Zähler</h1><form method='POST' action='/counter'>"
                   "<input name='counter' type='number' value='" + val + "'>"
                   "<input type='submit' value='Speichern'></form>"
                   "<p>Aktueller Wert: " + val + "</p>";
        },
        [](AsyncWebServerRequest *request) {
            if (request->hasParam("counter", true)) {
                wifiWebManager.saveCustomData("counter", request->getParam("counter", true)->value());
                return String("<p>Gespeichert!</p><a href='/counter'>Zurück</a>");
            }
            return String("<a href='/counter'>Zurück</a>");
        }
    );
}

void loop() {
    wifiWebManager.loop();

    // Optional: Hardware-Reset (z.B. GPIO 0 gegen GND)
    // if (digitalRead(0) == LOW) {
    //     wifiWebManager.reset();
    //     delay(1000); // Entprellen
    // }
}
