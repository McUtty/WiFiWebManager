Excellent. Here’s the **professionally formatted English `README.md` version** of your *WiFiWebManager* project — clean, consistent, and fully ready for GitHub publication:

---

````markdown
# 🧭 WiFiWebManager

**WiFiWebManager** is a comprehensive ESP32 framework for Wi-Fi management with an intuitive web interface.  
It provides robust connection handling, extensible web UI, persistent data storage, and OTA update capabilities.

---

## 🚀 Features

- **Smart Wi-Fi Connection** — 3-attempt system with automatic fallback to AP mode  
- **Reset Button Support** — Hardware reset via GPIO 0 (3s = Wi-Fi reset, 10s = full factory reset)  
- **Automatic Reconnect** — Monitors and restores lost connections  
- **Extensible Web Interface** — Easily add your own configuration pages  
- **Custom Data API** — Persistent storage for multiple data types  
- **Debug Mode** — Enable or disable debug output during development  
- **OTA Updates** — Firmware updates through the web interface  
- **Responsive Design** — Modern UI for desktop and mobile devices  

---

## 📦 Installation

### Using Arduino IDE Library Manager

1. Open **Arduino IDE**
2. Navigate to: `Sketch > Include Library > Manage Libraries`
3. Search for **WiFiWebManager**
4. Click **Install**

### Manual Installation

1. Download the latest release from **Releases**
2. Extract the ZIP file into your `Arduino/libraries` directory
3. Restart the Arduino IDE

---

## 🛠️ Dependencies

This framework requires the following libraries:

- [`ESPAsyncWebServer`](https://github.com/me-no-dev/ESPAsyncWebServer) *(installed automatically)*  
- [`AsyncTCP`](https://github.com/me-no-dev/AsyncTCP)

---

## 📖 Quick Start

```cpp
#include <WiFiWebManager.h>

WiFiWebManager wifiManager;

void setup() {
    Serial.begin(115200);
    
    // Optional: Enable debug mode
    wifiManager.setDebugMode(true);
    
    // Optional: Set a default hostname
    wifiManager.setDefaultHostname("MyESP32");
    
    wifiManager.begin();
}

void loop() {
    wifiManager.loop(); // IMPORTANT: must be called in loop()
}
````

---

## 🌐 Web Interface

After startup, the web interface is available at:

* **Wi-Fi Mode:** The IP address assigned to the ESP32
* **Access Point Mode:** `http://192.168.4.1`

### Default Pages

| Path      | Description                        |
| --------- | ---------------------------------- |
| `/`       | Status and overview (customizable) |
| `/wlan`   | Wi-Fi configuration (fixed)        |
| `/ntp`    | NTP time settings (fixed)          |
| `/update` | OTA firmware update (fixed)        |
| `/reset`  | Reset options (fixed)              |

---

## 🔧 Advanced Usage

### Add Custom Pages

**Note:** Default page names are reserved.

#### Simple GET Page

```cpp
wifiManager.addPage("My Page", "/custom", 
    [](AsyncWebServerRequest *request) -> String {
        return "<h1>Custom Page</h1><p>Your content here</p>";
    }
);
```

#### GET and POST Page

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

### Using Custom Data (max key length: 14 characters)

```cpp
// Save different data types
wifiManager.saveCustomData("deviceName", "Sensor1");    // String
wifiManager.saveCustomData("interval", 5000);           // int
wifiManager.saveCustomData("enabled", true);            // bool
wifiManager.saveCustomData("calibration", 1.25f);       // float

// Load with default values
String name = wifiManager.loadCustomData("deviceName", "Default");
int interval = wifiManager.loadCustomDataInt("interval", 1000);
bool enabled = wifiManager.loadCustomDataBool("enabled", false);
float calib = wifiManager.loadCustomDataFloat("calibration", 1.0);

// Check and remove
if (wifiManager.hasCustomData("oldValue")) {
    wifiManager.removeCustomData("oldValue");
}
```

---

## 🔘 Reset Button (GPIO 0)

Connect a push button between **GPIO 0** and **GND**:

| Duration     | Action                       |
| ------------ | ---------------------------- |
| 3–10 seconds | Erase Wi-Fi credentials only |
| >10 seconds  | Full factory reset           |

---

## 🐛 Debug Mode

```cpp
// Enable debug mode
wifiManager.setDebugMode(true);

// Query status
bool isDebugActive = wifiManager.getDebugMode();
```

---

## 📋 API Reference

### Basic Functions

| Function  | Description                         |
| --------- | ----------------------------------- |
| `begin()` | Initialize WiFiWebManager           |
| `loop()`  | Must be called inside the main loop |
| `reset()` | Perform full factory reset          |

### Hostname Management

| Function                                     | Description          |
| -------------------------------------------- | -------------------- |
| `setDefaultHostname(const String& hostname)` | Set default hostname |
| `getHostname()`                              | Get current hostname |

### Debug Functions

| Function                     | Description               |
| ---------------------------- | ------------------------- |
| `setDebugMode(bool enabled)` | Enable/disable debug mode |
| `getDebugMode()`             | Get debug mode state      |

### Page Management

```cpp
void addPage(const String& title, const String& path, 
             ContentHandler getHandler, 
             ContentHandler postHandler = nullptr);
void removePage(const String& path);
```

### Custom Data API

**Saving Data**

```cpp
void saveCustomData(const String& key, const String& value);
void saveCustomData(const String& key, int value);
void saveCustomData(const String& key, bool value);
void saveCustomData(const String& key, float value);
```

**Loading Data**

```cpp
String loadCustomData(const String& key, const String& defaultValue = "");
int loadCustomDataInt(const String& key, int defaultValue = 0);
bool loadCustomDataBool(const String& key, bool defaultValue = false);
float loadCustomDataFloat(const String& key, float defaultValue = 0.0);
```

**Data Management**

```cpp
bool hasCustomData(const String& key);
void removeCustomData(const String& key);
```

---

## ⚠️ Important Notes

* `wifiManager.loop()` **must** be called inside your `loop()` function.
* Avoid using reserved keys:
  `ssid`, `pwd`, `hostname`, `useStaticIP`, `ip`, `gateway`, `subnet`, `dns`, `ntpEnable`, `ntpServer`, `bootAttempts`.
* Enable debug mode **only** when needed.
* GPIO 0 is typically the **boot button** on most ESP32 boards.

---

## 🔗 Example Projects

See `/examples` for complete sketches:

* **Basic** – minimal setup example
* **CustomPages** – user-defined web pages
* **SensorData** – IoT sensor with configuration interface
* **SmartSwitch** – smart home device with Wi-Fi manager

---

## 📄 License

Licensed under the **MIT License**.
See [`LICENSE`](LICENSE) for details.

---

## 🤝 Contributing

Contributions are welcome!
Please read [`CONTRIBUTING.md`](CONTRIBUTING.md) before submitting pull requests.

---

## 📞 Support

* **Issues:** via [GitHub Issues](https://github.com/your-repo/issues)
* **Discussions:** via [GitHub Discussions](https://github.com/your-repo/discussions)

---

## 📊 System Requirements

| Component        | Requirement                  |
| ---------------- | ---------------------------- |
| **Hardware**     | ESP32 (any variant)          |
| **RAM**          | ~50 KB (library + webserver) |
| **Flash**        | ~200 KB (code + web assets)  |
| **Arduino Core** | ESP32 v2.0.0 or higher       |

---

⭐ **WiFiWebManager** — simple, robust, and extensible Wi-Fi management for ESP32.

```

---

Would you like me to include a **short “About” and “Architecture” section** at the beginning (useful for a GitHub landing page and documentation clarity)? It would give your README a more professional and complete structure.
```
