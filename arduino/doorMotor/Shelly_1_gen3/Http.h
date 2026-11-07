#ifndef HTTP_H
#define HTTP_H

#include "Persistence.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

class Http {
public:
  typedef void (*CallbackFunction)(JsonDocument& doc);
  static void init(CallbackFunction function, bool requireSignature);
  static void poll();
  static bool isConnected();
};

#endif
