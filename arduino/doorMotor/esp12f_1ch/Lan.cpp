#include "Lan.h"

bool Lan::connectWifi(const String& ssid, const String& password) {
  Serial.println("Wifi Connecting");
  WiFi.begin(ssid, password);
  unsigned long prevTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - prevTime >= 10*1000) {
      Serial.println("WiFi not connected");
      return false;
    }
    delay(500);
    Serial.print(".");
  }
  Serial.print("\nWiFi connected");

  return true;
}

void Lan::createHotspot(const String& ssid, const String& password) {
  WiFi.softAP(ssid, password);
}

void Lan::disconnectHotspot() {
  WiFi.softAPdisconnect(true);
}
