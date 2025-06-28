#include "WiFiWebManager.h"

WiFiWebManager wwm;

void setup() {
    Serial.begin(115200);
    wwm.begin();
}

void loop() {
    wwm.loop();
}
