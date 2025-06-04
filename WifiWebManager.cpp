#include "WifiWebManager.h"

WifiWebManager::WifiWebManager() {}

void WifiWebManager::begin() {
    Serial.println("\n=== Starte WifiWebManager ===");
    loadConfig();

    if (ssid.length() > 0 && password.length() > 0) {
        startSTA();
        Serial.print("Verbinde mit WLAN ");
        Serial.print(ssid);
        Serial.println(" ...");
        for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) {
            delay(500);
            Serial.print(".");
        }
        Serial.println();
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("WLAN verbunden!");
            Serial.print("IP-Adresse: ");
            Serial.println(WiFi.localIP());
        } else {
            Serial.println("Konnte nicht verbinden, starte AP-Modus.");
            startAP();
        }
    } else {
        Serial.println("Keine WLAN-Daten gespeichert, starte AP-Modus.");
        startAP();
    }
    handleNTP();
    setupWebServer();
    ArduinoOTA.begin();
}

void WifiWebManager::loop() {
    if (shouldReboot) {
        Serial.println("Reboot...");
        delay(500);
        ESP.restart();
    }
    ArduinoOTA.handle();
}

void WifiWebManager::loadConfig() {
    prefs.begin("netcfg", true);
    ssid = prefs.getString("ssid", "");
    password = prefs.getString("pwd", "");
    hostname = prefs.getString("hostname", "");
    useStaticIP = prefs.getBool("useStaticIP", false);
    ip = prefs.getString("ip", "");
    gateway = prefs.getString("gateway", "");
    subnet = prefs.getString("subnet", "");
    dns = prefs.getString("dns", "");
    ntpEnable = prefs.getBool("ntpEnable", false);
    ntpServer = prefs.getString("ntpServer", "pool.ntp.org");
    prefs.end();

    Serial.println("Geladene Einstellungen:");
    Serial.print("SSID: "); Serial.println(ssid);
    Serial.print("Hostname: "); Serial.println(hostname);
    Serial.print("Statische IP: "); Serial.println(useStaticIP ? "Ja" : "Nein");
    if (useStaticIP) {
        Serial.print("IP: "); Serial.println(ip);
        Serial.print("Gateway: "); Serial.println(gateway);
        Serial.print("Subnet: "); Serial.println(subnet);
        Serial.print("DNS: "); Serial.println(dns);
    }
    Serial.print("NTP aktiviert: "); Serial.println(ntpEnable ? "Ja" : "Nein");
    Serial.print("NTP Server: "); Serial.println(ntpServer);
}

void WifiWebManager::saveConfig(const String& s, const String& p, const String& h,
    bool us, const String& i, const String& g, const String& sub, const String& d,
    bool ntpEn, const String& ntpSrv) {
    prefs.begin("netcfg", false);
    prefs.putString("ssid", s);
    prefs.putString("pwd", p);
    prefs.putString("hostname", h);
    prefs.putBool("useStaticIP", us);
    prefs.putString("ip", i);
    prefs.putString("gateway", g);
    prefs.putString("subnet", sub);
    prefs.putString("dns", d);
    prefs.putBool("ntpEnable", ntpEn);
    prefs.putString("ntpServer", ntpSrv);
    prefs.end();

    Serial.println("Konfiguration gespeichert:");
    Serial.print("SSID: "); Serial.println(s);
    Serial.print("Hostname: "); Serial.println(h);
    Serial.print("Statische IP: "); Serial.println(us ? "Ja" : "Nein");
    if (us) {
        Serial.print("IP: "); Serial.println(i);
        Serial.print("Gateway: "); Serial.println(g);
        Serial.print("Subnet: "); Serial.println(sub);
        Serial.print("DNS: "); Serial.println(d);
    }
    Serial.print("NTP aktiviert: "); Serial.println(ntpEn ? "Ja" : "Nein");
    Serial.print("NTP Server: "); Serial.println(ntpSrv);
}

void WifiWebManager::saveNtpConfig(bool ntpEn, const String& ntpSrv) {
    prefs.begin("netcfg", false);
    prefs.putBool("ntpEnable", ntpEn);
    prefs.putString("ntpServer", ntpSrv);
    prefs.end();

    ntpEnable = ntpEn;
    ntpServer = ntpSrv;

    Serial.print("NTP Einstellungen gespeichert: ");
    Serial.print("aktiviert: "); Serial.print(ntpEnable ? "Ja" : "Nein");
    Serial.print(", Server: "); Serial.println(ntpServer);

    handleNTP();
}

void WifiWebManager::clearAllConfig() {
    prefs.begin("netcfg", false);
    prefs.clear();
    prefs.end();
    Serial.println("Alle Einstellungen gelöscht!");
}

void WifiWebManager::startAP() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP32_SETUP");
    Serial.println("Access Point gestartet: ESP32_SETUP");
    Serial.println("AP-IP: 192.168.4.1");
}

void WifiWebManager::startSTA() {
    WiFi.mode(WIFI_STA);

    if (hostname.length() > 0) {
        WiFi.setHostname(hostname.c_str());
        Serial.print("Setze Hostname auf: "); Serial.println(hostname);
    }

    if (useStaticIP) {
        IPAddress ip_, gateway_, subnet_, dns_;
        if (parseIPString(ip, ip_) && parseIPString(gateway, gateway_) &&
            parseIPString(subnet, subnet_) && parseIPString(dns, dns_)) {
            WiFi.config(ip_, gateway_, subnet_, dns_);
            Serial.println("Statische IP-Konfiguration gesetzt.");
        } else {
            Serial.println("Achtung: Ungültige statische IP-Konfiguration!");
        }
    }
    WiFi.begin(ssid.c_str(), password.c_str());
}

String WifiWebManager::getAvailableSSIDs() {
    int n = WiFi.scanNetworks();
    String options = "";
    for (int i = 0; i < n; ++i) {
        String selected = (ssid == WiFi.SSID(i)) ? "selected" : "";
        options += "<option value='" + WiFi.SSID(i) + "' " + selected + ">" + WiFi.SSID(i) + "</option>";
    }
    return options;
}

bool WifiWebManager::parseIPString(const String& str, IPAddress& out) {
    int parts[4];
    if (sscanf(str.c_str(), "%d.%d.%d.%d", &parts[0], &parts[1], &parts[2], &parts[3]) == 4) {
        out = IPAddress(parts[0], parts[1], parts[2], parts[3]);
        return true;
    }
    return false;
}

void WifiWebManager::handleNTP() {
    if (ntpEnable) {
        configTime(0, 0, ntpServer.c_str());
        Serial.print("NTP aktiviert, Server: ");
        Serial.println(ntpServer);
    }
}

void WifiWebManager::setupWebServer() {
    // Stil für alle Seiten:
    const String css = R"rawliteral(
  <style>
    body {
      background: #f3f6fa;
      font-family: sans-serif;
      margin: 0;
    }
    .centerbox {
      max-width: 420px;
      margin: 2.5em auto;
      padding: 2em;
      background: #fff;
      border-radius: 16px;
      box-shadow: 0 0 24px #0002;
      display: flex;
      flex-direction: column;
      align-items: center;
    }
    h1 { font-size: 1.6em; margin-bottom: 1em; }
    label { display: block; margin: 1em 0 0.5em; font-weight: 600; }
    input, select {
      width: 100%;
      font-size: 1.1em;
      padding: 0.8em;
      margin-bottom: 1em;
      border-radius: 8px;
      border: 1px solid #bbb;
      box-sizing: border-box;
    }
    button, input[type=submit] {
      width: 100%;
      padding: 1em;
      font-size: 1.1em;
      border: none;
      border-radius: 8px;
      background: #2584fc;
      color: #fff;
      margin-top: 0.7em;
      font-weight: 700;
      cursor: pointer;
      box-shadow: 0 4px 8px #2584fc22;
      transition: background 0.2s;
    }
    button:hover, input[type=submit]:hover { background: #1064b0; }
    nav {
      width: 100%;
      margin-bottom: 1.5em;
      display: flex;
      justify-content: center;
      gap: 1em;
    }
    nav a {
      text-decoration: none;
      color: #2584fc;
      font-weight: 600;
      font-size: 1.1em;
      padding-bottom: 2px;
      border-bottom: 2px solid transparent;
      transition: border-color 0.2s;
    }
    nav a.selected, nav a:hover { border-color: #2584fc; }
    @media (max-width: 600px) {
      .centerbox { max-width: 99vw; padding: 1em;}
      h1 { font-size: 1.2em; }
    }
  </style>
)rawliteral";

    // Startseite: WLAN & Grundeinstellungen
    server.on("/", HTTP_GET, [this, css](AsyncWebServerRequest *request){
        String html = R"rawliteral(
<!DOCTYPE html><html><head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32 WebKonfig</title>
)rawliteral";
        html += css;
        html += R"rawliteral(
</head><body>
<div class="centerbox">
  <nav>
    <a href="/" class="selected">WLAN</a>
    <a href="/ntp">NTP</a>
    <a href="/update">Firmware</a>
    <a href="/reset">Reset</a>
  </nav>
  <h1>ESP32 Konfiguration</h1>
  <form action='/save' method='POST'>
    <label>SSID:</label>
    <select name='ssid'>
)rawliteral";
        html += getAvailableSSIDs();
        html += R"rawliteral(
    </select>
    <label>Passwort:</label>
    <input name='pwd' type='password' value='' autocomplete='off'>
    <label>Hostname:</label>
    <input name='hostname' value='
)rawliteral";
        html += hostname;
        html += R"rawliteral('>
    <label>
      <input type='checkbox' name='useStaticIP' 
)rawliteral";
        html += (useStaticIP ? "checked" : "");
        html += R"rawliteral(> Statische IP aktivieren
    </label>
    <input name='ip' placeholder='IP-Adresse' value='
)rawliteral";
        html += ip;
        html += R"rawliteral('>
    <input name='gateway' placeholder='Gateway' value='
)rawliteral";
        html += gateway;
        html += R"rawliteral('>
    <input name='subnet' placeholder='Subnetz' value='
)rawliteral";
        html += subnet;
        html += R"rawliteral('>
    <input name='dns' placeholder='DNS' value='
)rawliteral";
        html += dns;
        html += R"rawliteral('>
    <input type='submit' value='Speichern'>
  </form>
</div>
</body></html>
)rawliteral";
        request->send(200, "text/html", html);
    });

    // Konfig speichern
    server.on("/save", HTTP_POST, [this](AsyncWebServerRequest *request){
        String newSSID = request->hasParam("ssid", true) ? request->getParam("ssid", true)->value() : "";
        String newPWD = request->hasParam("pwd", true) ? request->getParam("pwd", true)->value() : "";
        String newHostname = request->hasParam("hostname", true) ? request->getParam("hostname", true)->value() : "";
        bool newUseStaticIP = request->hasParam("useStaticIP", true);
        String newIP = request->hasParam("ip", true) ? request->getParam("ip", true)->value() : "";
        String newGateway = request->hasParam("gateway", true) ? request->getParam("gateway", true)->value() : "";
        String newSubnet = request->hasParam("subnet", true) ? request->getParam("subnet", true)->value() : "";
        String newDNS = request->hasParam("dns", true) ? request->getParam("dns", true)->value() : "";

        // NTP Einstellungen nicht überschreiben
        saveConfig(newSSID, newPWD, newHostname, newUseStaticIP, newIP, newGateway, newSubnet, newDNS, ntpEnable, ntpServer);
        shouldReboot = true;
        request->send(200, "text/html", "<p>Gespeichert! Neustart...</p><a href='/'>Zurück</a>");
    });

    // Werksreset
    server.on("/reset", HTTP_GET, [css](AsyncWebServerRequest *request){
        String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'><title>Reset</title>"
            + css + "</head><body><div class='centerbox'><nav>"
            "<a href='/'>WLAN</a><a href='/ntp'>NTP</a><a href='/update'>Firmware</a><a href='/reset' class='selected'>Reset</a>"
            "</nav><h1>Werksreset</h1><form action='/reset' method='POST'><input type='submit' value='Alle Einstellungen löschen & Neustart'></form></div></body></html>";
        request->send(200, "text/html", html);
    });
    server.on("/reset", HTTP_POST, [this](AsyncWebServerRequest *request){
        clearAllConfig();
        shouldReboot = true;
        request->send(200, "text/html", "<p>Werksreset ausgeführt. Neustart...</p>");
    });

    // OTA-Update-Seite
    server.on("/update", HTTP_GET, [css](AsyncWebServerRequest *request){
        String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'><title>Firmware Update</title>"
            + css + "</head><body><div class='centerbox'><nav>"
            "<a href='/'>WLAN</a><a href='/ntp'>NTP</a><a href='/update' class='selected'>Firmware</a><a href='/reset'>Reset</a>"
            "</nav><h1>Firmware Update</h1>"
            "<form method='POST' action='/update' enctype='multipart/form-data'>"
            "<input type='file' name='update'><input type='submit' value='Firmware Update'></form>"
            "</div></body></html>";
        request->send(200, "text/html", html);
    });

    // OTA-Upload
    server.on("/update", HTTP_POST,
        [](AsyncWebServerRequest *request) { request->send(200, "text/html", "<p>Update fertig, Neustart...</p>"); },
        [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            if (!index) Update.begin(UPDATE_SIZE_UNKNOWN);
            Update.write(data, len);
            if (final) {
                Update.end(true);
                ESP.restart();
            }
        }
    );

    // NTP-Seite
    server.on("/ntp", HTTP_GET, [this, css](AsyncWebServerRequest *request){
        String html = R"rawliteral(
<!DOCTYPE html><html><head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>NTP Einstellungen</title>
)rawliteral";
        html += css;
        html += R"rawliteral(
</head><body>
<div class="centerbox">
  <nav>
    <a href="/">WLAN</a>
    <a href="/ntp" class="selected">NTP</a>
    <a href="/update">Firmware</a>
    <a href="/reset">Reset</a>
  </nav>
  <h1>NTP Einstellungen</h1>
  <form action='/ntp_save' method='POST'>
    <label>
      <input type='checkbox' name='ntpEnable' 
)rawliteral";
        html += (ntpEnable ? "checked" : "");
        html += R"rawliteral(> NTP aktivieren
    </label>
    <label>NTP Server:</label>
    <input name='ntpServer' value='
)rawliteral";
        html += ntpServer;
        html += R"rawliteral('>
    <input type='submit' value='Speichern'>
  </form>
</div>
</body></html>
)rawliteral";
        request->send(200, "text/html", html);
    });
    server.on("/ntp_save", HTTP_POST, [this](AsyncWebServerRequest *request){
        bool newNtpEnable = request->hasParam("ntpEnable", true);
        String newNtpServer = request->hasParam("ntpServer", true) ? request->getParam("ntpServer", true)->value() : "pool.ntp.org";
        saveNtpConfig(newNtpEnable, newNtpServer);
        request->send(200, "text/html", "<p>NTP gespeichert!</p><a href='/ntp'>Zurück</a>");
    });

    server.begin();
    Serial.println("WebServer gestartet!");
}

void WifiWebManager::reset() {
    clearAllConfig();  // löscht alles in Preferences
    shouldReboot = true;
}
