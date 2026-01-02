#ifndef HTTP_H
#define HTTP_H

#include "Persistance.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

class Http {
public:
  typedef DynamicJsonDocument (*CallbackFunction)(DynamicJsonDocument doc);
  static void init(CallbackFunction function);
  static void poll();
  static bool isConnected();
};

#endif