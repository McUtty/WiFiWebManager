/*
  WiFiWebManager - ServiceTaskWatchdog Example (v3.0.0)

  Zeigt die Neuerungen ab v3.0.0:
   - FreeRTOS-Service-Task (Default AN): Die Lib erledigt ihre Wartung selbst;
     wifiManager.loop() ist dann ein No-Op. Opt-out per setServiceTask(false).
   - Task-Watchdog (Default AN): ueberwacht die Service-Task; hier haengen wir
     zusaetzlich eine eigene Consumer-Task ein.
   - OTA-Selbstheilung: onUpdateStart() stoppt stoerende Peripherie vor dem Flash,
     setOtaStallTimeout() begrenzt abgebrochene Uploads.
   - "ESP neu starten"-Button auf der /reset-Seite (Neustart ohne Datenverlust).

  Hardware: beliebiges ESP32-Board.
  Nach dem Upload wie ueblich: AP "ESP32_SETUP" -> http://192.168.4.1 -> WLAN einrichten.

  Version: 3.0.0
  Autor: McUtty
*/

#include <WiFiWebManager.h>

WiFiWebManager wifiManager;

// Platzhalter fuer stoerende Peripherie (z. B. eine Kamera mit eigenem
// Treiber-Task/DMA), die VOR dem Flash gestoppt werden muss, sonst kann ein
// OTA-Update abstuerzen.
void stopPeripherals() {
    Serial.println("[OTA] Stoppe Peripherie vor dem Flash (z. B. camera.deinit())...");
    // camera.deinit(); digitalWrite(MOTOR_PIN, LOW); ...
}

// Beispiel-Consumer-Task, die eigene Arbeit macht und sich in den Lib-Watchdog
// einhaengt, damit sie mitueberwacht wird.
void workerTask(void* arg) {
    wifiManager.watchdogAddCurrentTask();       // von der Lib mitueberwachen lassen
    for (;;) {
        // ... eigene periodische Arbeit ...
        wifiManager.watchdogFeedCurrentTask();  // regelmaessig fuettern
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== WiFiWebManager ServiceTaskWatchdog Example (v3.0.0) ===");

    // App-Version (erscheint auf der /update-Seite)
    wifiManager.setFirmwareVersion("1.0.0");
    wifiManager.setDefaultHostname("ESP32-Advanced");
    wifiManager.setDebugMode(true);

    // --- v3.0.0-Optionen (alle VOR begin()) ---
    // Service-Task ist Default AN. Zum alten Verhalten (du rufst loop() selbst):
    //   wifiManager.setServiceTask(false);
    wifiManager.setServiceTask(true);

    // Task-Watchdog: Default AN (30 s, panic=true). Hier explizit gezeigt:
    wifiManager.enableWatchdog(true, 30, true);

    // OTA robust: Peripherie vor dem Flash stoppen + Stall-Timeout fuer
    // mittendrin abgebrochene Uploads.
    wifiManager.setOnUpdateStart(stopPeripherals);
    wifiManager.setOtaStallTimeout(8000);   // ms (Default)

    // Optional: WLAN-Status-LED (On-Board-WS2812), Pin je nach Board:
    //   wifiManager.enableStatusLed(48);

    wifiManager.begin();

    // Eigene Seite hinzufuegen (erscheint automatisch im Menue)
    wifiManager.addPage("Info", "/info", [](AsyncWebServerRequest* r) -> String {
        String h = "<h1>Info</h1>";
        h += "<p>Uptime: " + String(millis() / 1000) + " s</p>";
        h += "<p>Freier Heap: " + String(ESP.getFreeHeap()) + " Bytes</p>";
        return h;
    });

    // Eigene Consumer-Task starten, die vom Lib-Watchdog mitueberwacht wird.
    xTaskCreatePinnedToCore(workerTask, "worker", 4096, nullptr, 1, nullptr, 1);

    Serial.println("Setup fertig. Wartung laeuft in der Service-Task 'wfwm_svc'.");
}

void loop() {
    // Mit Default-AN-Service-Task ist wifiManager.loop() ein No-Op. Der Aufruf
    // bleibt fuer Rueckwaertskompatibilitaet erlaubt und schadet nicht.
    // (Bei setServiceTask(false) MUSS er wie bisher aufgerufen werden.)
    wifiManager.loop();

    // Eigener, unkritischer Code kann hier stehen. Der Consumer-loop() wird vom
    // Lib-Watchdog bewusst NICHT ueberwacht.
    delay(1000);
}
