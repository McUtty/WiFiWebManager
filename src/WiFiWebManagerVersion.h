#pragma once

// WiFiWebManager Version Information
#define WIFIWEB_MANAGER_VERSION "2.0.1"
#define WIFIWEB_MANAGER_VERSION_MAJOR 1
#define WIFIWEB_MANAGER_VERSION_MINOR 0
#define WIFIWEB_MANAGER_VERSION_PATCH 0

// Build Information  
#define WIFIWEB_MANAGER_BUILD_DATE __DATE__
#define WIFIWEB_MANAGER_BUILD_TIME __TIME__

// Compatibility Information
#define WIFIWEB_MANAGER_MIN_ESP32_CORE "2.0.0"

// Feature Flags
#define WIFIWEB_MANAGER_FEATURE_DEBUG 1
#define WIFIWEB_MANAGER_FEATURE_OTA 1
#define WIFIWEB_MANAGER_FEATURE_NTP 1
#define WIFIWEB_MANAGER_FEATURE_CUSTOM_DATA 1

namespace WiFiWebManagerInfo {
    inline const char* getVersion() {
        return WIFIWEB_MANAGER_VERSION;
    }
    
    inline const char* getBuildDate() {
        return WIFIWEB_MANAGER_BUILD_DATE;
    }
    
    inline const char* getBuildTime() {
        return WIFIWEB_MANAGER_BUILD_TIME;
    }
    
    inline String getFullVersionString() {
        return String("WiFiWebManager v") + WIFIWEB_MANAGER_VERSION + 
               " (Build: " + WIFIWEB_MANAGER_BUILD_DATE + " " + WIFIWEB_MANAGER_BUILD_TIME + ")";
    }
}