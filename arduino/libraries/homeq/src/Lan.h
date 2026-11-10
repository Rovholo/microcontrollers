#ifndef LAN_H
#define LAN_H

#include <Arduino.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

class Lan {
public:
  static bool connectWifi(const String& ssid, const String& password);
  static uint8_t scanChannel();
  static void createHotspot(const String& ssid, const String& password, uint8_t channel);
  static void disconnectHotspot();
  static void stationOnly();
};

#endif
