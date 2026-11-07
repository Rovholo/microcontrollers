#ifndef LAN_H
#define LAN_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>

class Lan {
public:
  static bool connectWifi(const String& ssid, const String& password);
  static uint8_t scanChannel();
  static void createHotspot(const String& ssid, const String& password, uint8_t channel);
  static void disconnectHotspot();
};

#endif
