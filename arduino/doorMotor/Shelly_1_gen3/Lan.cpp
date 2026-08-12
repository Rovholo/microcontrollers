#include "Lan.h"

bool Lan::connectWifi(const String& ssid, const String& password) {
  Serial.println("Wifi Connecting");
  WiFi.begin(ssid, password);
  unsigned long prevTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - prevTime >= 30000) {
      Serial.println("\nWiFi not connected " + WiFi.channel());
      WiFi.disconnect();
      return false;
    }
    delay(200);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected " + WiFi.channel());
  return true;
}

void Lan::createHotspot(const String& ssid, const String& password) {
  WiFi.softAP(ssid, password);
  Serial.println("Hotspot created");
}

void Lan::disconnectHotspot() {
  WiFi.softAPdisconnect(true);
}
