#pragma once

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <ArduinoOTA.h>
#include <Update.h>

class WifiWebManager {
public:
    WifiWebManager();
    void begin();
    void loop();
    void reset();

private:
    Preferences prefs;
    AsyncWebServer server{80};

    String ssid, password, hostname;
    String ip, gateway, subnet, dns;
    bool useStaticIP = false;
    bool shouldReboot = false;

    // NTP
    bool ntpEnable = false;
    String ntpServer = "pool.ntp.org";

    void loadConfig();
    void saveConfig(const String& ssid, const String& pwd, const String& hostname,
                    bool useStatic, const String& ip, const String& gateway, const String& subnet, const String& dns,
                    bool ntpEnable, const String& ntpServer);
    void saveNtpConfig(bool ntpEnable, const String& ntpServer);
    void clearAllConfig();
    void startAP();
    void startSTA();
    String getAvailableSSIDs();
    void setupWebServer();
    bool parseIPString(const String& str, IPAddress& out);
    void handleNTP();
};
