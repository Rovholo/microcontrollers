#ifndef HTTP_H
#define HTTP_H

#include "Persistance.h"
#include "Lan.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <ESPmDNS.h>

class Http {
public:
  typedef DynamicJsonDocument (*CallbackFunction)(DynamicJsonDocument doc);
  static void init(CallbackFunction function);
  static void poll();
  static bool isConnected();
};

#endif