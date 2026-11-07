#ifndef HTTP_H
#define HTTP_H

#include "Persistence.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#else
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#endif

class Http {
public:
  typedef void (*CallbackFunction)(JsonDocument& doc);
  static void init(CallbackFunction function, bool requireSignature);
  static void poll();
  static bool isConnected();
  static void setAbout(const String& json);
  static void stopMdns();
};

#endif
