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
* **OTA Updates:** Firmware updates via web interface
* **Responsive Design:** Modern web UI for both desktop and mobile

---

## 📦 Installation

### Arduino IDE Library Manager

1. Open Arduino IDE
2. Go to **Sketch > Include Library > Manage Libraries**
3. Search for **“WiFiWebManager”**
4. Click **Install**

### Manual Installation

1. Download the latest release
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
| `/update` | Firmware update (fixed)            |
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

### Use Custom Data (key max 14 characters)

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

### Debug Functions

| Function                     | Description                 |
| ---------------------------- | --------------------------- |
| `setDebugMode(bool enabled)` | Enable/disable debug output |
| `getDebugMode()`             | Get debug mode state        |

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
* **CustomPages** – user-defined web pages
* **SensorData** – IoT sensor with configuration
* **SmartSwitch** – smart home device

---

## 📄 License

MIT License – see LICENSE for details

---

## 🤝 Contributing

Contributions are welcome!
Please read `CONTRIBUTING.md` for details.

---

## 📞 Support

* **Issues:** via GitHub Issues
* **Discussions:** via GitHub Discussions

---

## 📊 System Requirements

| Component        | Requirement                      |
| ---------------- | -------------------------------- |
| **Hardware**     | ESP32 (any variant)              |
| **RAM**          | ~50 KB for framework + webserver |
| **Flash**        | ~200 KB for code + web assets    |
| **Arduino Core** | ESP32 v2.0.0 or higher           |
