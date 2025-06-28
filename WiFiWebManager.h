#pragma once

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <ArduinoOTA.h>
#include <Update.h>
#include <vector>
#include <functional>

class WiFiWebManager {
public:
    WiFiWebManager();
    void begin();
    void loop();

    using ContentHandler = std::function<String(AsyncWebServerRequest*)>;

    void addPage(const String& menutitle, const String& path, ContentHandler getHandler, ContentHandler postHandler = nullptr);
    void removePage(const String& path);

    void saveCustomData(const String& key, const String& value);
    String loadCustomData(const String& key, const String& defaultValue = "");

    void reset();

private:
    ContentHandler rootGetHandler = nullptr;
    ContentHandler rootPostHandler = nullptr;
    
    Preferences prefs;
    AsyncWebServer server{80};

    String ssid, password, hostname;
    String ip, gateway, subnet, dns;
    bool useStaticIP = false;
    bool shouldReboot = false;

    bool ntpEnable = false;
    String ntpServer = "pool.ntp.org";

    struct CustomPage {
        String title;
        String path;
        ContentHandler getHandler;
        ContentHandler postHandler;
    };
    std::vector<CustomPage> customPages;

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

    String renderMenu(const String& currentPath);
    String htmlWrap(const String& menutitle, const String& currentPath, const String& content);
};
