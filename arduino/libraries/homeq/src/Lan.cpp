#include "Lan.h"

namespace {
int16_t findNetwork(const String& ssid) {
  int16_t found = -1;
  int16_t count = WiFi.scanComplete();
  for (int16_t i = 0; i < count; i++) {
    if (WiFi.SSID(i) == ssid && (found < 0 || WiFi.RSSI(i) > WiFi.RSSI(found))) found = i;
  }
  return found;
}
}

bool Lan::connectWifi(const String& ssid, const String& password) {
  Serial.println("Wifi Connecting");
  int16_t network = findNetwork(ssid);
  if (network >= 0) {
    WiFi.begin(ssid, password, WiFi.channel(network), WiFi.BSSID(network));
  } else {
    WiFi.begin(ssid, password);
  }
  unsigned long prevTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - prevTime >= 10 * 1000) {
      Serial.println("\nWiFi not connected");
      if (WiFi.getMode() & WIFI_AP) WiFi.disconnect();
      return false;
    }
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  return true;
}

uint8_t Lan::scanChannel() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  int16_t count = WiFi.scanNetworks();
  uint8_t channel = 1;
  int32_t strongest = INT32_MIN;
  for (int16_t i = 0; i < count; i++) {
    if (WiFi.RSSI(i) > strongest) {
      strongest = WiFi.RSSI(i);
      channel = WiFi.channel(i);
    }
  }
  Serial.println("Hotspot channel " + String(channel));
  return channel;
}

void Lan::createHotspot(const String& ssid, const String& password, uint8_t channel) {
  Serial.println("Creating hotspot...");
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid, password, channel);
  Serial.println("Hotspot created");
}

void Lan::disconnectHotspot() {
  WiFi.softAPdisconnect(true);
  WiFi.scanDelete();
}
