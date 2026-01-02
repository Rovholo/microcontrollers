#ifndef PERSISTANCE_H
#define PERSISTANCE_H

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <base64.h>

class Persistance {
private:
  inline static const String psFileName = "persistance.json";
  static String read();
  static void write(const String& text);
  static DynamicJsonDocument getJson();
public:
  inline static const String WIFI_SSID = "wifiSsid";
  inline static const String WIFI_PASSWORD = "wifiPassword";
  inline static const String ACTIVE = "active";
  inline static const String NAME = "name";
  inline static const String ID = "id";
  inline static const String HOME_ID = "homeId";
  inline static const String MQTT_BROKER = "mqttBrokerId";
  inline static const String MQTT_USER = "mqttUser";
  inline static const String MQTT_PASSWORD = "mqttPassword";
  static String read(const String& name);
  static void write(const String& name, const String& text);
  static void init();
  static void clearJson();
  static String get(const String& key);
  static void persist(const String& key, const String& value);
  static String getToken();
};

#endif