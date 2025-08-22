/*
 * WiFiWebManager Library v2.1.1 - Light Sleep Example
 *
 * Demonstriert die neuen Light Sleep Features für maximale Energieeffizienz.
 * Reduziert Stromverbrauch von ~150mA auf ~20-40mA bei voller WiFi-Funktionalität.
 *
 * Features:
 * - Light Sleep Mode mit automatischem WiFi Wake-up
 * - GPIO Wake-up für sofortige Sensor-Reaktion
 * - Timer Wake-up für regelmäßige Tasks
 * - Wake-up Grund Analyse für intelligente Reaktionen
 * - Wake-up Statistiken
 */

#include <WiFiWebManager.h>

WiFiWebManager wwm;

// GPIO Pins für Sensoren/Schalter
const int DOOR_SENSOR_PIN = 2;  // Reed-Kontakt für Tür
const int ALERT_SENSOR_PIN = 5; // Sensor mit Alert-Ausgang
const int LED_PIN = 23;         // Status-LED

// Variablen für Anwendungslogik
bool lastDoorState = true;
unsigned long lastSensorRead = 0;
int sensorReadings = 0;

void setup()
{
    Serial.begin(115200);
    Serial.println("WiFiWebManager Light Sleep Example v2.1.1");

    // GPIO Setup
    pinMode(DOOR_SENSOR_PIN, INPUT_PULLUP);
    pinMode(ALERT_SENSOR_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // WiFiWebManager Basis-Konfiguration
    wwm.setDebugMode(true);
    wwm.setDefaultHostname("ESP32-LightSleep");
    wwm.setDefaultNTP(true, "pool.ntp.org");

    // Light Sleep Konfiguration (v2.1.1)
    wwm.setDefaultLightSleep(true);  // Light Sleep aktivieren
    wwm.setLightSleepTimer(5000000); // 5 Sekunden Timer für regelmäßige Tasks

    // GPIO Wake-up Sources konfigurieren
    wwm.addWakeupGPIO(DOOR_SENSOR_PIN, GPIO_INTR_ANY_EDGE);   // Tür-Sensor (beide Flanken)
    wwm.addWakeupGPIO(ALERT_SENSOR_PIN, GPIO_INTR_LOW_LEVEL); // Alert-Sensor (Low-Pegel)

    // Wake-up Statistiken aktivieren
    wwm.enableWakeupLogging(true);

    wwm.begin();

    Serial.println("Light Sleep Setup abgeschlossen!");
    Serial.println("Energieverbrauch: ~20-40mA (statt ~150mA)");
    Serial.println("WiFi bleibt aktiv, Web-Interface reagiert sofort!");

    // Status-LED kurz blinken lassen
    for (int i = 0; i < 3; i++)
    {
        digitalWrite(LED_PIN, HIGH);
        delay(200);
        digitalWrite(LED_PIN, LOW);
        delay(200);
    }

    lastDoorState = digitalRead(DOOR_SENSOR_PIN);
}

void loop()
{
    // WiFiWebManager Loop - behandelt auch Wake-up Analyse
    wwm.loop();

    // Intelligente Reaktion basierend auf Wake-up Grund
    analyzeWakeupReason();

    // Regelmäßige Tasks (nur bei Timer Wake-up)
    if (wwm.wasWokenByTimer())
    {
        performRegularTasks();
    }

    // Anwendungslogik
    handleSensors();

    // Wake-up Statistiken alle 60 Sekunden ausgeben
    static unsigned long lastStatsOutput = 0;
    if (millis() - lastStatsOutput > 60000)
    {
        lastStatsOutput = millis();
        printWakeupStatistics();
    }
}

void analyzeWakeupReason()
{
    static bool firstRun = true;

    // Skip first run (startup)
    if (firstRun)
    {
        firstRun = false;
        return;
    }

    // Wake-up Grund analysieren
    String wakeupReason = wwm.getWakeupCauseString();

    if (wwm.wasWokenByGPIO())
    {
        int wakeupPin = wwm.getWakeupGPIO();
        Serial.printf("⚡ GPIO Wake-up von Pin %d (%s)\n", wakeupPin, wakeupReason.c_str());

        // LED blinken bei GPIO Wake-up
        digitalWrite(LED_PIN, HIGH);
        delay(50);
        digitalWrite(LED_PIN, LOW);

        if (wakeupPin == DOOR_SENSOR_PIN)
        {
            Serial.println("🚪 Tür-Sensor hat geweckt - sofortige Reaktion!");
            handleDoorSensorImmediate();
        }
        else if (wakeupPin == ALERT_SENSOR_PIN)
        {
            Serial.println("🚨 Alert-Sensor aktiv - kritischer Zustand!");
            handleAlertSensorImmediate();
        }
    }
    else if (wwm.wasWokenByTimer())
    {
        Serial.printf("⏰ Timer Wake-up (%s)\n", wakeupReason.c_str());
    }
    else if (wwm.wasWokenByWiFi())
    {
        Serial.printf("📡 WiFi Wake-up (%s) - Web-Request empfangen\n", wakeupReason.c_str());
    }
    else
    {
        Serial.printf("❓ Anderer Wake-up: %s\n", wakeupReason.c_str());
    }
}

void handleDoorSensorImmediate()
{
    bool currentState = digitalRead(DOOR_SENSOR_PIN);

    if (currentState != lastDoorState)
    {
        lastDoorState = currentState;

        if (currentState == LOW)
        {
            Serial.println("🔓 ALARM: Tür geöffnet!");
            // Hier würde Alarm-Logik stehen (Telegram, E-Mail, etc.)
        }
        else
        {
            Serial.println("🔒 Tür geschlossen");
        }
    }
}

void handleAlertSensorImmediate()
{
    Serial.println("🚨 KRITISCHER ALERT erkannt!");

    // LED schnell blinken lassen
    for (int i = 0; i < 5; i++)
    {
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        delay(100);
    }

    // Hier würde kritische Sensor-Behandlung stehen
}

void performRegularTasks()
{
    Serial.println("📋 Regelmäßige Tasks ausführen...");

    // Beispiel: Sensor-Werte lesen (I2C/SPI)
    readI2CSensors();

    // System-Status prüfen
    checkSystemHealth();

    // Heartbeat senden (falls konfiguriert)
    sendHeartbeat();
}

void handleSensors()
{
    // Normale Sensor-Behandlung (nicht zeitkritisch)
    // Diese läuft auch nach normalem Wake-up

    static unsigned long lastCheck = 0;
    if (millis() - lastCheck > 1000)
    { // Alle 1 Sekunde
        lastCheck = millis();

        // Tür-Status prüfen (falls nicht durch Wake-up getriggert)
        bool doorState = digitalRead(DOOR_SENSOR_PIN);
        if (doorState != lastDoorState)
        {
            handleDoorSensorImmediate();
        }
    }
}

void readI2CSensors()
{
    // Beispiel für I2C Sensor-Abfrage
    // Diese läuft nur bei Timer Wake-up (alle 5 Sekunden)

    sensorReadings++;
    Serial.printf("📊 I2C Sensor-Lesung #%d\n", sensorReadings);

    // Hier würde echte I2C-Kommunikation stehen:
    // Wire.beginTransmission(SENSOR_ADDRESS);
    // float temperature = sensor.readTemperature();
    // float humidity = sensor.readHumidity();

    // Simulierte Werte
    float temperature = 20.0 + (millis() % 10000) / 1000.0;
    float humidity = 50.0 + (millis() % 5000) / 1000.0;

    Serial.printf("🌡️  Temperatur: %.1f°C, Luftfeuchtigkeit: %.1f%%\n", temperature, humidity);
}

void checkSystemHealth()
{
    // System-Gesundheit prüfen
    uint32_t freeHeap = ESP.getFreeHeap();
    int wifiRSSI = WiFi.RSSI();

    Serial.printf("💾 Freier Heap: %d Bytes\n", freeHeap);
    Serial.printf("📶 WiFi Signal: %d dBm\n", wifiRSSI);

    if (freeHeap < 10000)
    {
        Serial.println("⚠️  WARNUNG: Wenig freier Speicher!");
    }

    if (wifiRSSI < -80)
    {
        Serial.println("⚠️  WARNUNG: Schwaches WiFi-Signal!");
    }
}

void sendHeartbeat()
{
    // Heartbeat-Signal senden (z.B. an Server)
    Serial.printf("💓 Heartbeat - Uptime: %lu Sekunden\n", millis() / 1000);
}

void printWakeupStatistics()
{
    auto stats = wwm.getWakeupStats();

    Serial.println("\n📈 === Wake-up Statistiken ===");
    Serial.printf("Timer Wake-ups: %d\n", stats.timerWakeups);
    Serial.printf("GPIO Wake-ups: %d\n", stats.gpioWakeups);
    Serial.printf("WiFi Wake-ups: %d\n", stats.wifiWakeups);
    Serial.printf("Andere Wake-ups: %d\n", stats.otherWakeups);
    Serial.printf("Gesamt Wake-ups: %d\n", stats.totalWakeups);

    // Effizienz berechnen
    if (stats.totalWakeups > 0)
    {
        float timerPercent = (float)stats.timerWakeups / stats.totalWakeups * 100;
        float gpioPercent = (float)stats.gpioWakeups / stats.totalWakeups * 100;
        float wifiPercent = (float)stats.wifiWakeups / stats.totalWakeups * 100;

        Serial.printf("Timer: %.1f%%, GPIO: %.1f%%, WiFi: %.1f%%\n",
                      timerPercent, gpioPercent, wifiPercent);
    }

    Serial.println("===============================\n");
}

/*
 * Light Sleep Energieeffizienz:
 *
 * Normaler Betrieb: ~150-300mA
 * Light Sleep:       ~20-40mA
 * Einsparung:        ~80-85%
 *
 * Wake-up Zeiten:
 * - GPIO Wake-up:    ~3ms (praktisch sofort)
 * - Timer Wake-up:   ~3ms
 * - WiFi Wake-up:    ~3ms (HTTP-Requests)
 *
 * GPIO Wake-up Modi:
 * - GPIO_INTR_ANY_EDGE:   Beide Flanken (Schalter)
 * - GPIO_INTR_LOW_LEVEL:  Low-Pegel (Alert-Sensoren)
 * - GPIO_INTR_HIGH_LEVEL: High-Pegel
 * - GPIO_INTR_POSEDGE:    Positive Flanke
 * - GPIO_INTR_NEGEDGE:    Negative Flanke
 *
 * Timer-Konfiguration:
 * - Mikrosekunden: 1000000 = 1 Sekunde
 * - Minimum: ~10ms (10000 µs)
 * - Empfohlen für I2C/SPI: 100ms - 10s
 *
 * WiFi-Verhalten:
 * - Bleibt verbunden während Light Sleep
 * - WebServer reagiert sofort auf Requests
 * - Automatischer Wake-up bei eingehenden Paketen
 * - Keine Verbindungsunterbrechung
 */