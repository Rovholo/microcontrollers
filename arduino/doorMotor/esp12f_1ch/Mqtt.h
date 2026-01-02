#ifndef MQTT_H
#define MQTT_H

#include "Persistance.h"
#include "Http.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

class Mqtt {
private:
  inline static String broker;
  inline static String username;
  inline static String password;
  inline static String deviceId;
  inline static String home;
  inline static String action;
  inline static String reaction;
  inline static String pingTopic;
  static void handleConnection();
  static bool connect(const String& id, const String& username, const String& password);
  static bool isConnected();
public:
  typedef DynamicJsonDocument (*CallbackFunction)(DynamicJsonDocument doc);
  static void init(CallbackFunction function);
  static void poll();
  static void subscribe(const String& topic);
  static void send(DynamicJsonDocument doc);
  static void send(const String& payload);
  static void send(const String& topic, const String& payload);
};

#endif
