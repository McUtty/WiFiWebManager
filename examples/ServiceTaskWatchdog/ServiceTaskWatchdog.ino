/*
  WiFiWebManager - ServiceTaskWatchdog Example (v3.1.0)

  Zeigt die Neuerungen ab v3.0.0 sowie das OTA-quiesce-Muster aus v3.1.0:
   - FreeRTOS-Service-Task (Default AN): Die Lib erledigt ihre Wartung selbst;
     wifiManager.loop() ist dann ein No-Op. Opt-out per setServiceTask(false).
   - Task-Watchdog (Default AN): ueberwacht die Service-Task; hier haengen wir
     zusaetzlich eine eigene Consumer-Task ein.
   - OTA unter Last (v3.1.0): onUpdateStart() stoppt Peripherie/haelt eigene Tasks
     an, onUpdateEnd(success) gibt sie wieder frei. Die Lib-eigene Service-Task
     pausiert waehrend eines OTA automatisch.
   - OTA-Selbstheilung optional (setOtaStallTimeout, Default AUS).
   - "ESP neu starten"-Button auf der /reset-Seite (Neustart ohne Datenverlust).

  Hardware: beliebiges ESP32-Board.
  Nach dem Upload wie ueblich: AP "ESP32_SETUP" -> http://192.168.4.1 -> WLAN einrichten.

  Version: 3.1.0
  Autor: McUtty
*/

#include <WiFiWebManager.h>

WiFiWebManager wifiManager;

// quiesce-Flag: waehrend eines OTA pausiert die eigene Consumer-Task, damit sie
// nicht mit dem OTA-Empfang konkurriert.
volatile bool g_workerPaused = false;

// Platzhalter fuer stoerende Peripherie (z. B. eine Kamera mit eigenem
// Treiber-Task/DMA), die VOR dem Flash gestoppt werden muss.
void stopPeripherals() {
    Serial.println("[OTA] onUpdateStart: Peripherie stoppen + eigene Tasks anhalten...");
    g_workerPaused = true;
    // camera.deinit(); digitalWrite(MOTOR_PIN, LOW); ...
}

// Gegenstueck: nach dem OTA (Erfolg ODER Abbruch) wieder freigeben.
void resumePeripherals(bool success) {
    Serial.printf("[OTA] onUpdateEnd(success=%d): eigene Tasks wieder freigeben.\n", (int)success);
    g_workerPaused = false;
    // camera.init(); ...
}

// Beispiel-Consumer-Task, die eigene Arbeit macht und sich in den Lib-Watchdog
// einhaengt, damit sie mitueberwacht wird. Waehrend eines OTA haelt sie sich
// (ueber g_workerPaused) zurueck.
void workerTask(void* arg) {
    wifiManager.watchdogAddCurrentTask();       // von der Lib mitueberwachen lassen
    for (;;) {
        if (!g_workerPaused) {
            // ... eigene periodische Arbeit ...
        }
        wifiManager.watchdogFeedCurrentTask();  // regelmaessig fuettern
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== WiFiWebManager ServiceTaskWatchdog Example (v3.1.0) ===");

    // App-Version (erscheint auf der /update-Seite)
    wifiManager.setFirmwareVersion("1.0.0");
    wifiManager.setDefaultHostname("ESP32-Advanced");
    wifiManager.setDebugMode(true);

    // --- Optionen (alle VOR begin()) ---
    // Service-Task ist Default AN. Zum alten Verhalten (du rufst loop() selbst):
    //   wifiManager.setServiceTask(false);
    wifiManager.setServiceTask(true);

    // Task-Watchdog: Default AN (30 s, panic=true). Hier explizit gezeigt:
    wifiManager.enableWatchdog(true, 30, true);

    // OTA robust (quiesce-Muster): Peripherie/Tasks bei Start anhalten, bei
    // Ende/Abbruch wieder freigeben.
    wifiManager.setOnUpdateStart(stopPeripherals);
    wifiManager.setOnUpdateEnd(resumePeripherals);

    // Optionale Selbstheilung bei echt abgerissenem Upload (Default AUS = 0).
    // Falls gewuenscht aktivieren (Empfehlung >= 20000):
    //   wifiManager.setOtaStallTimeout(20000);

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
