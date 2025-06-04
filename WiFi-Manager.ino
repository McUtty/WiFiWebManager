#include "WifiWebManager.h"

WifiWebManager wifiWebManager;

void setup() {
    Serial.begin(115200);
    wifiWebManager.begin();
}

void loop() {
    wifiWebManager.loop();
}
