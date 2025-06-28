#include "WiFiWebManager.h"

WiFiWebManager::WiFiWebManager() {}

void WiFiWebManager::begin() {
    Serial.println("\n=== Starte WiFiWebManager ===");
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

void WiFiWebManager::loop() {
    if (shouldReboot) {
        Serial.println("Reboot...");
        delay(500);
        ESP.restart();
    }
    ArduinoOTA.handle();
}

void WiFiWebManager::loadConfig() {
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
    Serial.println("Konfiguration geladen.");
}

void WiFiWebManager::saveConfig(const String& s, const String& p, const String& h,
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
    Serial.println("Konfiguration gespeichert.");
}

void WiFiWebManager::saveNtpConfig(bool ntpEn, const String& ntpSrv) {
    prefs.begin("netcfg", false);
    prefs.putBool("ntpEnable", ntpEn);
    prefs.putString("ntpServer", ntpSrv);
    prefs.end();
    ntpEnable = ntpEn;
    ntpServer = ntpSrv;
    handleNTP();
}

void WiFiWebManager::clearAllConfig() {
    prefs.begin("netcfg", false);
    prefs.clear();
    prefs.end();
    Serial.println("Alle Einstellungen gelöscht!");
}

void WiFiWebManager::reset() {
    clearAllConfig();
    shouldReboot = true;
}

void WiFiWebManager::startAP() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP32_SETUP");
    Serial.println("Access Point gestartet: ESP32_SETUP");
    Serial.println("AP-IP: 192.168.4.1");
}

void WiFiWebManager::startSTA() {
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

String WiFiWebManager::getAvailableSSIDs() {
    int n = WiFi.scanNetworks();
    String options = "";
    for (int i = 0; i < n; ++i) {
        String selected = (ssid == WiFi.SSID(i)) ? "selected" : "";
        options += "<option value='" + WiFi.SSID(i) + "' " + selected + ">" + WiFi.SSID(i) + "</option>";
    }
    return options;
}

bool WiFiWebManager::parseIPString(const String& str, IPAddress& out) {
    int parts[4];
    if (sscanf(str.c_str(), "%d.%d.%d.%d", &parts[0], &parts[1], &parts[2], &parts[3]) == 4) {
        out = IPAddress(parts[0], parts[1], parts[2], parts[3]);
        return true;
    }
    return false;
}

void WiFiWebManager::handleNTP() {
    if (ntpEnable) {
        configTime(0, 0, ntpServer.c_str());
        Serial.print("NTP aktiviert, Server: ");
        Serial.println(ntpServer);
    }
}

// Zwei-Zeilen-Menü: Standard oben, Custom unten, beide wrap
String WiFiWebManager::renderMenu(const String& currentPath) {
    String nav = "<div class='nav-main'>";
    // Standardseiten (erste Zeile)
    nav += "<nav class='nav-std'>";
    struct PageEntry { String title, path; };
    PageEntry stdpages[] = {
        {"Home", "/"}, {"WLAN", "/wlan"}, {"NTP", "/ntp"}, {"Firmware", "/update"}, {"Reset", "/reset"}
    };
    for (auto &p : stdpages) {
        nav += "<a href='" + p.path + "'";
        if (currentPath == p.path) nav += " class='selected'";
        nav += ">" + p.title + "</a>";
    }
    nav += "</nav>";
    // Custompages (zweite Zeile)
    if (!customPages.empty()) {
        nav += "<nav class='nav-custom'>";
        for (const auto& page : customPages) {
            nav += "<a href='" + page.path + "'";
            if (currentPath == page.path) nav += " class='selected'";
            nav += ">" + page.title + "</a>";
        }
        nav += "</nav>";
    }
    nav += "</div>";
    return nav;
}

String WiFiWebManager::htmlWrap(const String& menutitle, const String& currentPath, const String& content) {
    String css = R"rawliteral(
      <style>
      body{background:#f3f6fa;font-family:sans-serif;margin:0;}
      .centerbox{max-width:420px;margin:2.5em auto;padding:2em;background:#fff;
      border-radius:16px;box-shadow:0 0 24px #0002;display:flex;flex-direction:column;align-items:center;}
      h1{font-size:1.6em;margin-bottom:1em;}label{display:block;margin:1em 0 0.5em;font-weight:600;}
      input,select{width:100%;font-size:1.1em;padding:0.8em;margin-bottom:1em;border-radius:8px;
      border:1px solid #bbb;box-sizing:border-box;}button,input[type=submit]{width:100%;padding:1em;
      font-size:1.1em;border:none;border-radius:8px;background:#2584fc;color:#fff;margin-top:0.7em;font-weight:700;
      cursor:pointer;box-shadow:0 4px 8px #2584fc22;transition:background 0.2s;}
      button:hover,input[type=submit]:hover{background:#1064b0;}
      .nav-main{width:100%;margin-bottom:1.5em;}
      .nav-std, .nav-custom {
        display: flex;
        flex-wrap: wrap;
        justify-content: center;
        gap: 1em;
      }
      .nav-custom { margin-top:0.2em; }
      .nav-std a, .nav-custom a {
        text-decoration:none;color:#2584fc;font-weight:600;font-size:1.1em;
        padding-bottom:2px;border-bottom:2px solid transparent;transition:border-color 0.2s;
      }
      .nav-std a.selected, .nav-std a:hover,
      .nav-custom a.selected, .nav-custom a:hover { border-color:#2584fc; }
      @media (max-width:600px){
        .centerbox{max-width:99vw;padding:1em;}
        h1{font-size:1.2em;}
        .nav-std a, .nav-custom a {font-size:1em;}
      }
      </style>
    )rawliteral";
    String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>" + menutitle + "</title>";
    html += css;
    html += "</head><body><div class='centerbox'>";
    html += renderMenu(currentPath);
    html += content;
    html += "</div></body></html>";
    return html;
}

void WiFiWebManager::addPage(const String& menutitle, const String& path, ContentHandler getHandler, ContentHandler postHandler) {
    if (path == "/") {
        // Überschreibe nur den Root-Handler
        this->rootGetHandler = getHandler;
        this->rootPostHandler = postHandler;
        // Nicht zu customPages hinzufügen!
        return;
    }
    // Doppelte entfernen
    for (auto it = customPages.begin(); it != customPages.end(); ++it) {
        if (it->path == path) { customPages.erase(it); break; }
    }
    customPages.push_back({menutitle, path, getHandler, postHandler});

    // GET
    server.on(path.c_str(), HTTP_GET, [this, path, menutitle, getHandler](AsyncWebServerRequest *request) {
        String content = getHandler ? getHandler(request) : "<p>(Keine Seite definiert)</p>";
        String html = htmlWrap(menutitle, path, content);
        request->send(200, "text/html; charset=utf-8", html);
    });
    // POST
    if (postHandler) {
        server.on(path.c_str(), HTTP_POST, [this, path, menutitle, postHandler](AsyncWebServerRequest *request){
            String content = postHandler(request);
            String html = htmlWrap(menutitle, path, content);
            request->send(200, "text/html; charset=utf-8", html);
        });
    }
}

void WiFiWebManager::removePage(const String& path) {
    for (auto it = customPages.begin(); it != customPages.end(); ++it) {
        if (it->path == path) { customPages.erase(it); break; }
    }
    // Route verschwindet nach Reboot (Server-Handler wird nicht deregistriert)
}

void WiFiWebManager::saveCustomData(const String& key, const String& value) {
    prefs.begin("customdata", false);
    prefs.putString(("custom_" + key).c_str(), value);
    prefs.end();
}
String WiFiWebManager::loadCustomData(const String& key, const String& defaultValue) {
    prefs.begin("customdata", true);
    String v = prefs.getString(("custom_" + key).c_str(), defaultValue);
    prefs.end();
    return v;
}

// ===== Webserver + Standardseiten wie bisher ===========

void WiFiWebManager::setupWebServer() {
    // Standardmäßige, leere Startseite
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (rootGetHandler) {
            String content = rootGetHandler(request);
            request->send(200, "text/html; charset=utf-8", htmlWrap("Home", "/", content));
        } else {
            request->send(200, "text/html; charset=utf-8", htmlWrap("Home", "/", ""));
        }
    });

    // WLAN
    server.on("/wlan", HTTP_GET, [this](AsyncWebServerRequest *request){
        String html;
        html = "<h1>ESP32 Konfiguration</h1>"
            "<form action='/save' method='POST'>"
            "<label>SSID:</label><select name='ssid'>";
        html += getAvailableSSIDs();
        html += "</select>"
            "<label>Passwort:</label>"
            "<input name='pwd' type='password' value='' autocomplete='off'>"
            "<label>Hostname:</label>"
            "<input name='hostname' value='" + hostname + "'>"
            "<label><input type='checkbox' name='useStaticIP' ";
        html += (useStaticIP ? "checked" : "");
        html += "> Statische IP aktivieren</label>"
            "<input name='ip' placeholder='IP-Adresse' value='" + ip + "'>"
            "<input name='gateway' placeholder='Gateway' value='" + gateway + "'>"
            "<input name='subnet' placeholder='Subnetz' value='" + subnet + "'>"
            "<input name='dns' placeholder='DNS' value='" + dns + "'>"
            "<input type='submit' value='Speichern'>"
            "</form>";
        request->send(200, "text/html; charset=utf-8", htmlWrap("ESP32 WebKonfig", "/wlan", html));
    });

    // Save WLAN
    server.on("/save", HTTP_POST, [this](AsyncWebServerRequest *request){
        String newSSID = request->hasParam("ssid", true) ? request->getParam("ssid", true)->value() : "";
        String newPWD = request->hasParam("pwd", true) ? request->getParam("pwd", true)->value() : "";
        String newHostname = request->hasParam("hostname", true) ? request->getParam("hostname", true)->value() : "";
        bool newUseStaticIP = request->hasParam("useStaticIP", true);
        String newIP = request->hasParam("ip", true) ? request->getParam("ip", true)->value() : "";
        String newGateway = request->hasParam("gateway", true) ? request->getParam("gateway", true)->value() : "";
        String newSubnet = request->hasParam("subnet", true) ? request->getParam("subnet", true)->value() : "";
        String newDNS = request->hasParam("dns", true) ? request->getParam("dns", true)->value() : "";
        saveConfig(newSSID, newPWD, newHostname, newUseStaticIP, newIP, newGateway, newSubnet, newDNS, ntpEnable, ntpServer);
        shouldReboot = true;
        request->send(200, "text/html; charset=utf-8", htmlWrap("Speichern", "/", "<p>Gespeichert! Neustart...</p><a href='/'>Zurück</a>"));
    });

    // Reset
    server.on("/reset", HTTP_GET, [this](AsyncWebServerRequest *request){
        String html = "<h1>Werksreset</h1><form action='/reset' method='POST'><input type='submit' value='Alle Einstellungen löschen & Neustart'></form>";
        request->send(200, "text/html; charset=utf-8", htmlWrap("Reset", "/reset", html));
    });
    server.on("/reset", HTTP_POST, [this](AsyncWebServerRequest *request){
        clearAllConfig();
        shouldReboot = true;
        request->send(200, "text/html; charset=utf-8", htmlWrap("Reset", "/reset", "<p>Werksreset ausgeführt. Neustart...</p>"));
    });

    // OTA
    server.on("/update", HTTP_GET, [this](AsyncWebServerRequest *request){
        String html = "<h1>Firmware Update</h1>"
            "<form method='POST' action='/update' enctype='multipart/form-data'>"
            "<input type='file' name='update'><input type='submit' value='Firmware Update'></form>";
        request->send(200, "text/html; charset=utf-8", htmlWrap("Firmware", "/update", html));
    });
    server.on("/update", HTTP_POST,
        [](AsyncWebServerRequest *request) { request->send(200, "text/html; charset=utf-8", "<p>Update fertig, Neustart...</p>"); },
        [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
            if (!index) Update.begin(UPDATE_SIZE_UNKNOWN);
            Update.write(data, len);
            if (final) {
                Update.end(true);
                ESP.restart();
            }
        }
    );

    // NTP
    server.on("/ntp", HTTP_GET, [this](AsyncWebServerRequest *request){
        String html = "<h1>NTP Einstellungen</h1>"
            "<form action='/ntp_save' method='POST'>"
            "<label><input type='checkbox' name='ntpEnable' ";
        html += (ntpEnable ? "checked" : "");
        html += "> NTP aktivieren</label>"
            "<label>NTP Server:</label>"
            "<input name='ntpServer' value='" + ntpServer + "'>"
            "<input type='submit' value='Speichern'></form>";
        request->send(200, "text/html; charset=utf-8", htmlWrap("NTP Einstellungen", "/ntp", html));
    });
    server.on("/ntp_save", HTTP_POST, [this](AsyncWebServerRequest *request){
        bool newNtpEnable = request->hasParam("ntpEnable", true);
        String newNtpServer = request->hasParam("ntpServer", true) ? request->getParam("ntpServer", true)->value() : "pool.ntp.org";
        saveNtpConfig(newNtpEnable, newNtpServer);
        request->send(200, "text/html; charset=utf-8", htmlWrap("NTP Einstellungen", "/ntp", "<p>NTP gespeichert!</p><a href='/ntp'>Zurück</a>"));
    });

    server.begin();
    Serial.println("WebServer gestartet!");
}
