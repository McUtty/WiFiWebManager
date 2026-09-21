# WiFiWebManager

A comprehensive ESP32 framework for Wi-Fi management with a web interface, offering robust connection handling, extensible web UI, and persistent data storage.

---

## 🚀 Features

* **Smart Wi-Fi Connection:** 3-attempt system with automatic fallback to AP mode
* **Reset Button Support:** Hardware reset via GPIO 0 (3s = Wi-Fi reset, 10s = full factory reset)
* **Auto Reconnect:** Monitors and restores lost connections
* **Extensible Web Interface:** Easily add your own configuration pages
* **Custom Data API:** Persistent storage for multiple data types
* **Debug Mode:** Enable or disable debug output during development
* **OTA Updates:** Firmware updates via web interface, with the application version shown on the update page (`setFirmwareVersion`)
* **WiFi Status LED** (optional): an addressable on-board RGB LED (WS2812) reflects the connection status (green/yellow/red, AP = blinking red) — freely assignable pin, toggled on/off via a function call
* **Responsive Design:** Modern web UI for both desktop and mobile

---

## 📦 Installation

### Arduino IDE Library Manager

1. Open Arduino IDE
2. Go to **Sketch > Include Library > Manage Libraries**
3. Search for **“WiFiWebManager”**
4. Click **Install**

### Manual Installation

1. Download the latest version from [Releases](https://github.com/McUtty/WiFiWebManager/releases)
2. Extract the ZIP file into your `Arduino/libraries` folder
3. Restart Arduino IDE

---

## 🛠️ Dependencies

This framework requires:

* **ESPAsyncWebServer** (installed automatically)
* **AsyncTCP** (dependency of ESPAsyncWebServer)

---

## 📖 Quick Start

```cpp
#include <WiFiWebManager.h>

WiFiWebManager wifiManager;

void setup() {
    Serial.begin(115200);
    
    // Optional: Enable debug mode
    wifiManager.setDebugMode(true);
    
    // Optional: Set default hostname
    wifiManager.setDefaultHostname("MyESP32");

    // Optional: Version shown on the /update page
    wifiManager.setFirmwareVersion("1.0.0");
    
    wifiManager.begin();
}

void loop() {
    wifiManager.loop(); // IMPORTANT: must be called inside loop()
}
```

---

## 🌐 Web Interface

After startup, the web interface is available at:

* **Wi-Fi mode:** ESP32’s assigned IP address
* **AP mode:** `http://192.168.4.1`

### Default Pages:

| Path      | Description                        |
| --------- | ---------------------------------- |
| `/`       | Status and overview (customizable) |
| `/wlan`   | Wi-Fi configuration (fixed)        |
| `/ntp`    | NTP time settings (fixed)          |
| `/update` | OTA firmware update; shows version (fixed) |
| `/reset`  | Reset options (fixed)              |

---

## 🔧 Advanced Usage

### Add Custom Pages

(Names of default pages are reserved)

**Simple GET page:**

```cpp
wifiManager.addPage("My Page", "/custom", 
    [](AsyncWebServerRequest *request) -> String {
        return "<h1>Custom Page</h1><p>Your content here</p>";
    }
);
```

**Page with GET and POST support:**

```cpp
wifiManager.addPage("Settings", "/settings", 
    // GET handler
    [](AsyncWebServerRequest *request) -> String {
        String html = "<h1>Settings</h1>";
        html += "<form action='/settings' method='POST'>";
        html += "<input name='value' placeholder='Enter value'>";
        html += "<input type='submit' value='Save'>";
        html += "</form>";
        return html;
    },
    // POST handler
    [](AsyncWebServerRequest *request) -> String {
        String value = request->getParam("value", true)->value();
        // Process value...
        return "<p>Saved: " + value + "</p><a href='/settings'>Back</a>";
    }
);
```

---

### Use Custom Data (key max. 13 characters)

```cpp
// Store different data types
wifiManager.saveCustomData("deviceName", "Sensor1");    // String
wifiManager.saveCustomData("interval", 5000);           // int
wifiManager.saveCustomData("enabled", true);            // bool
wifiManager.saveCustomData("calibration", 1.25f);       // float

// Load with default values
String name = wifiManager.loadCustomData("deviceName", "Default");
int interval = wifiManager.loadCustomDataInt("interval", 1000);
bool enabled = wifiManager.loadCustomDataBool("enabled", false);
float calib = wifiManager.loadCustomDataFloat("calibration", 1.0);

// Check existence and remove
if (wifiManager.hasCustomData("oldValue")) {
    wifiManager.removeCustomData("oldValue");
}
```

---

## 🔘 Reset Button (GPIO 0)

Connect a push button between **GPIO 0** and **GND**:

* Hold 3–10 seconds → erase only Wi-Fi data
* Hold >10 seconds → full factory reset

---

## 🐛 Debug Mode

```cpp
// Enable debug mode (only via code)
wifiManager.setDebugMode(true);

// Check status
bool isDebugActive = wifiManager.getDebugMode();
```

---

## 💡 WiFi Status LED (optional)

Many ESP32-S3 boards carry an addressable **on-board RGB LED (WS2812)**.
WiFiWebManager can mirror the WiFi status on it — **disabled by default**, enabled
via a function call with a freely assignable pin. No extra driver required (uses the
ESP32 core's `neopixelWrite()`).

| Color | Meaning |
|---|---|
| 🟢 green | connected, good signal (RSSI ≥ threshold, default −70 dBm) |
| 🟡 yellow | connected, weak signal |
| 🔴 red | connection lost / not (yet) connected as station |
| 🔴 blinking red | AP setup mode (`ESP32_SETUP`) |

```cpp
void setup() {
    wifiManager.enableStatusLed(48);       // enable + GPIO (e.g. 48 on the S3-DevKitC-1)
    // optional:
    wifiManager.setStatusLedBrightness(40);      // 0..255 (default 40)
    wifiManager.setStatusLedRssiThreshold(-70);  // good/weak threshold (dBm)
    wifiManager.setStatusLedSelfTest(true);      // boot self-test red/green/blue
    wifiManager.begin();
}
```

Pin and on/off can also be changed **at runtime**:

```cpp
wifiManager.disableStatusLed();     // LED off
wifiManager.enableStatusLed(38);    // back on, now on GPIO 38
```

---

## 🧵 FreeRTOS Service Task + Watchdog (since v3.0.0)

Since v3.0.0 the library runs its housekeeping (reset button, OTA handle, status
LED, WiFi scan/reconnect, OTA-stall check) in its **own FreeRTOS task `wfwm_svc`**
(core 1, away from WiFi/lwIP) by default. WiFi scan/reconnect no longer block your `loop()`.

**Backward compatible:** the public `loop()` remains and becomes a **no-op** while
the service task runs — existing sketches that call `loop()` keep working unchanged.

```cpp
// Optional, BEFORE begin():
wifiManager.setServiceTask(false);   // exact previous behavior (you call loop() yourself)
```

### Task Watchdog (on by default)
The library ships a task watchdog that watches **only the service task** — a long
consumer `loop()` will **not** cause a reset. The reset reason is logged at boot
(clearly flagged as `WATCHDOG-RESET`).

```cpp
// Optional, BEFORE begin():
wifiManager.enableWatchdog(true, 30, true);  // on, 30 s timeout, panic=true (default)
wifiManager.enableWatchdog(false);           // off

// Hook your own consumer tasks in (instead of using esp_task_wdt directly):
wifiManager.watchdogAddCurrentTask();
wifiManager.watchdogFeedCurrentTask();
wifiManager.watchdogRemoveCurrentTask();
```

### OTA self-healing on aborted uploads
If a `/update` upload is cut off mid-way after `setOnUpdateStart()` (flaky WiFi),
the `final` chunk never arrives — previously the peripheral stopped by the
callback (e.g. a camera) stayed dead until a power cycle. Now the service task
detects the stall and reboots cleanly after a timeout (intact old firmware +
peripherals restored).

```cpp
wifiManager.setOtaStallTimeout(8000);   // ms, default 8000
```

### "Restart ESP"
The `/reset` page now has a green **"ESP neu starten" (Restart ESP)** button at the
top — a plain reboot **without** data loss (e.g. to recover peripherals after an
aborted OTA).

---

## 📋 API Reference

### Basic Functions

| Function  | Description                   |
| --------- | ----------------------------- |
| `begin()` | Initialize WiFiWebManager     |
| `loop()`  | Must be called inside loop()  |
| `reset()` | Performs a full factory reset |

---

### Hostname Management

| Function                                     | Description               |
| -------------------------------------------- | ------------------------- |
| `setDefaultHostname(const String& hostname)` | Set default hostname      |
| `getHostname()`                              | Retrieve current hostname |

---

### Firmware Version

| Function                                    | Description                                              |
| ------------------------------------------- | -------------------------------------------------------- |
| `setFirmwareVersion(const String& version)` | Sets the application version shown on the `/update` page |

Call this once in `setup()` so the firmware version is visible in the web UI:

```cpp
wifiManager.setFirmwareVersion("1.0.0");
```

---

### OTA Callback (stop peripherals before flashing)

| Function                                       | Description                                                                          |
| ---------------------------------------------- | ------------------------------------------------------------------------------------ |
| `setOnUpdateStart(std::function<void()> cb)`   | Called right before the first flash write (OTA via `/update` AND ArduinoOTA/espota)  |

Use it to stop peripherals that interfere with flashing (e.g. a camera driver/DMA) before the write begins:

```cpp
wifiManager.setOnUpdateStart([]() { camera.deinit(); });
```

---

### Debug Functions

| Function                     | Description                 |
| ---------------------------- | --------------------------- |
| `setDebugMode(bool enabled)` | Enable/disable debug output |
| `getDebugMode()`             | Get debug mode state        |

---

### WiFi Status LED

| Function | Description |
| -------- | ----------- |
| `enableStatusLed(uint8_t pin, uint8_t brightness = 40)` | Enable and assign the LED pin |
| `disableStatusLed()`                    | Turn the LED off              |
| `setStatusLedPin(uint8_t pin)`          | Reassign the pin              |
| `setStatusLedBrightness(uint8_t)`       | Per-channel brightness 0..255 |
| `setStatusLedRssiThreshold(int dbm)`    | Good/weak threshold (default −70) |
| `setStatusLedSelfTest(bool)`            | Boot self-test red/green/blue |

---

### Page Management

```cpp
void addPage(const String& title, const String& path, 
             ContentHandler getHandler, 
             ContentHandler postHandler = nullptr);
void removePage(const String& path);
```

---

### Custom Data API

**Save:**

```cpp
void saveCustomData(const String& key, const String& value);
void saveCustomData(const String& key, int value);
void saveCustomData(const String& key, bool value);
void saveCustomData(const String& key, float value);
```

**Load:**

```cpp
String loadCustomData(const String& key, const String& defaultValue = "");
int loadCustomDataInt(const String& key, int defaultValue = 0);
bool loadCustomDataBool(const String& key, bool defaultValue = false);
float loadCustomDataFloat(const String& key, float defaultValue = 0.0);
```

**Manage:**

```cpp
bool hasCustomData(const String& key);
void removeCustomData(const String& key);
std::vector<String> getCustomDataKeys();  // list all stored custom keys
```

---

## ⚠️ Important Notes

* **wifiManager.loop()** must be called inside your `loop()` function
* **Custom Data:** avoid reserved keys (`ssid`, `pwd`, `hostname`, etc.)
* **Performance:** enable debug mode only when necessary
* **Reset Button:** GPIO 0 is the default boot button on most ESP32 boards

---

## 🔗 Examples

See the `/examples` folder for complete demos:

* **Basic** – minimal setup
* **Test** – custom pages, custom data and debug output (demo with simulated values)
* **ServiceTaskWatchdog** – v3.0.0 features: service task, watchdog, OTA self-healing, own watched task

---

## 📝 Changelog

| Version | Date | Description |
|---------|------|-------------|
| **3.0.2** | 2026-09-21 | OTA reception further hardened: no more device reboot on an ArduinoOTA error (espota previously could reboot during the handshake → "No response"); `ArduinoOTA.handle()` polled every **10 ms**.<br>**OTA self-healing off by default** (`setOtaStallTimeout(0)`): a running `/update` is **never** aborted on its own; recovery only when enabled (`setOtaStallTimeout(ms>0)`, recommended ≥ 20000).<br>`/update` progress diagnostics (every 64 KB) when debug is on. |
| **3.0.1** | 2026-09-21 | OTA receive fix (regression from 3.0.0): service task runs on **core 1** (away from WiFi/lwIP/AsyncTCP on core 0); the service task fully backs off during an OTA (no scan/reconnect/LED/reset button); watchdog fed during espota via `onProgress`; stall-timeout default **8 s → 20 s**. |
| **3.0.0** | 2026-09-17 | **FreeRTOS service task** (`wfwm_svc`, on by default; public `loop()` becomes a no-op, `setServiceTask(false)` for the old behavior); **task watchdog** (on by default) + reset-reason log at boot; OTA self-healing via stall timeout (`setOtaStallTimeout`); "Restart ESP" button on `/reset`; fix: checkboxes/radios to the left of the text. **MAJOR** (runtime behavior changes, source-compatible). |
| **2.2.0** | 2026-09-15 | `setOnUpdateStart()` — pre-flash callback to stop peripherals before an OTA. |
| **2.1.0** | 2026-09-03 | Optional WiFi status LED (WS2812) via `enableStatusLed()`. |

## 📄 License

MIT License – see LICENSE for details

---

## 🤝 Contributing

Contributions are welcome!
Please open an issue or a pull request on GitHub.

---

## 📞 Support

* **Issues:** [GitHub Issues](https://github.com/McUtty/WiFiWebManager/issues)
* **Discussions:** [GitHub Discussions](https://github.com/McUtty/WiFiWebManager/discussions)

---

## 📊 System Requirements

| Component        | Requirement                      |
| ---------------- | -------------------------------- |
| **Hardware**     | ESP32 (any variant)              |
| **RAM**          | ~50 KB for framework + webserver |
| **Flash**        | ~200 KB for code + web assets    |
| **Arduino Core** | ESP32 v2.0.0 or higher           |

---

## 📚 WiFiWebManager Framework – Function Reference

### 📋 Basic Methods

| Function           | Description                                              | Parameters | Return |
|--------------------|----------------------------------------------------------|------------|--------|
| `WiFiWebManager()` | Constructor – initializes reset button (GPIO 0)          | –          | –      |
| `begin()`          | Starts WiFiWebManager; connects to Wi-Fi or starts AP    | –          | `void` |
| `loop()`           | Must be called inside the main `loop()`                  | –          | `void` |
| `reset()`          | Performs a full factory reset                            | –          | `void` |

---

### 🌐 Network Configuration

| Function                        | Description                               | Parameters             | Return   |
|---------------------------------|-------------------------------------------|------------------------|----------|
| `setDefaultHostname(hostname)`  | Sets default hostname from code           | `String hostname`      | `void`   |
| `getHostname()`                 | Returns current hostname                  | –                      | `String` |
| `setFirmwareVersion(version)`   | App version shown on the `/update` page   | `String version`       | `void`   |

> Factory reset is performed with the public `reset()` method (see Basic Methods). Wi-Fi-only and full erase are also triggered by the hardware reset button (GPIO 0).

---

### 📄 Custom Pages (Web)

| Function                                              | Description                           | Parameters                                                                 | Return |
|-------------------------------------------------------|---------------------------------------|----------------------------------------------------------------------------|--------|
| `addPage(title, path, getHandler)`                    | Adds a GET-only page                  | `String title, String path, ContentHandler getHandler`                     | `void` |
| `addPage(title, path, getHandler, postHandler)`       | Adds a page with GET and POST         | `String title, String path, ContentHandler getHandler, ContentHandler postHandler` | `void` |
| `removePage(path)`                                    | Removes a custom page                 | `String path`                                                              | `void` |


---

### 💾 Custom Data API

#### Saving (Setters)

| Function                     | Description       | Parameters                 | Constraints           |
| ---------------------------- | ----------------- | -------------------------- | --------------------- |
| `saveCustomData(key, value)` | Stores a `String` | `String key, String value` | Key max. 13 characters |
| `saveCustomData(key, value)` | Stores an `int`   | `String key, int value`    | Key max. 13 characters |
| `saveCustomData(key, value)` | Stores a `bool`   | `String key, bool value`   | Key max. 13 characters |
| `saveCustomData(key, value)` | Stores a `float`  | `String key, float value`  | Key max. 13 characters |

#### Loading (Getters)

| Function                                 | Description    | Parameters                              | Return   |
| ---------------------------------------- | -------------- | --------------------------------------- | -------- |
| `loadCustomData(key, defaultValue)`      | Loads `String` | `String key, String defaultValue = ""`  | `String` |
| `loadCustomDataInt(key, defaultValue)`   | Loads `int`    | `String key, int defaultValue = 0`      | `int`    |
| `loadCustomDataBool(key, defaultValue)`  | Loads `bool`   | `String key, bool defaultValue = false` | `bool`   |
| `loadCustomDataFloat(key, defaultValue)` | Loads `float`  | `String key, float defaultValue = 0.0`  | `float`  |

#### Management

| Function                | Description             | Parameters   | Return                |
| ----------------------- | ----------------------- | ------------ | --------------------- |
| `hasCustomData(key)`    | Checks if key exists    | `String key` | `bool`                |
| `removeCustomData(key)` | Deletes stored value    | `String key` | `void`                |
| `getCustomDataKeys()`   | Returns all custom keys | –            | `std::vector<String>` |

---

### 🛠️ Debug & Utilities

| Function                | Description                   | Parameters     | Return |
| ----------------------- | ----------------------------- | -------------- | ------ |
| `setDebugMode(enabled)` | Enables/disables debug output | `bool enabled` | `void` |
| `getDebugMode()`        | Returns debug mode state      | –              | `bool` |

---

### 💡 WiFi Status LED (optional)

Disabled by default. Drives an addressable on-board RGB LED (WS2812) via the ESP32
core's `neopixelWrite()` — no extra library.

| Function                          | Description                             | Parameters                  | Return |
| --------------------------------- | --------------------------------------- | --------------------------- | ------ |
| `enableStatusLed(pin, brightness)`| Enable and assign the LED pin           | `uint8_t pin, uint8_t bri=40` | `void` |
| `disableStatusLed()`              | Turn the LED off                        | –                           | `void` |
| `setStatusLedPin(pin)`            | Reassign the pin                        | `uint8_t pin`               | `void` |
| `setStatusLedBrightness(b)`       | Per-channel brightness (0..255)         | `uint8_t b`                 | `void` |
| `setStatusLedRssiThreshold(dbm)`  | Good/weak threshold (default −70 dBm)   | `int dbm`                   | `void` |
| `setStatusLedSelfTest(enabled)`   | Boot self-test red/green/blue           | `bool enabled`              | `void` |

Colors: 🟢 green = connected (good), 🟡 yellow = connected (weak), 🔴 red = lost,
🔴 blinking red = AP setup mode.

---

### ⚠️ Important Constraints

#### 🔑 Key Constraints

* **Max length:** 13 characters
* **Reserved keys (do not use):**
  `ssid`, `pwd`, `hostname`,
  `useStaticIP`, `ip`, `gateway`, `subnet`, `dns`,
  `ntpEnable`, `ntpServer`, `bootAttempts`

#### 🔄 Boot-Attempt System

* Up to **3** connection attempts on Wi-Fi errors
* After 3 failures → automatic **AP mode**
* Successful connection resets the counter

#### 🔧 Hardware Reset Button (GPIO 0)

| Press Duration | Action                       |
| -------------- | ---------------------------- |
| 3–10 seconds   | Erase Wi-Fi credentials only |
| >10 seconds    | Full factory reset           |

---

### 📝 Example Code

```cpp
#include "WiFiWebManager.h"

WiFiWebManager wwm;

// Custom Page Handler
String handleMyPage(AsyncWebServerRequest *request) {
    return String("<h1>My Page</h1><p>Status: ") +
           wwm.loadCustomData("status", "OK") + "</p>";
}

void setup() {
    Serial.begin(115200);

    // Set hostname
    wwm.setDefaultHostname("MyESP32");

    // Enable debug mode
    wwm.setDebugMode(true);

    // Add custom page
    wwm.addPage("Status", "/status", handleMyPage);

    // Save custom data (key max. 13 chars!)
    wwm.saveCustomData("temp_max", 25.5f);
    wwm.saveCustomData("alerts", true);
    wwm.saveCustomData("count", 42);

    wwm.begin();
}

void loop() {
    wwm.loop();

    // Load custom data
    float maxTemp = wwm.loadCustomDataFloat("temp_max", 20.0f);
    bool alertsOn = wwm.loadCustomDataBool("alerts", false);
}
```

---

### 🌍 Default Web Pages

The framework automatically provides:

* `/` – Home / Status overview
* `/wlan` – Wi-Fi configuration
* `/ntp` – NTP time server settings
* `/update` – OTA firmware update
* `/reset` – Reset options

