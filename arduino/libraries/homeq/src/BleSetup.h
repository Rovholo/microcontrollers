#ifndef BLE_SETUP_H
#define BLE_SETUP_H

#include <Arduino.h>
#include <ArduinoJson.h>

class BleSetup {
public:
  typedef void (*CallbackFunction)(JsonDocument& doc);
  static bool available();
  static void begin(const String& name, const String& about, CallbackFunction callback);
  static void poll();
  static void end();
};

#endif
