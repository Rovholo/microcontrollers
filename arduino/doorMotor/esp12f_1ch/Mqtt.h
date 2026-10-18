#ifndef MQTT_H
#define MQTT_H

#include "Persistance.h"
#include <Arduino.h>
#include <ESP8266WiFi.h>
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
  inline static String reactionTopic;
  inline static String pingTopic;
  inline static String stateTopic;
  inline static bool connectedBefore = false;
  inline static const unsigned long minRetryDelay = 5 * 1000;
  inline static const unsigned long maxRetryDelay = 60 * 1000;
  inline static unsigned long retryTimer = 0;
  inline static unsigned long retryDelay = 0;
  static void handleConnection();
  static bool connect(const String& id, const String& username, const String& password);
  static bool isConnected();
public:
  typedef void (*CallbackFunction)(JsonDocument& doc);
  typedef void (*ConnectFunction)(bool firstConnect);
  static void init(CallbackFunction function, ConnectFunction onConnect);
  static void poll();
  static void subscribe(const String& topic);
  static void send(const JsonDocument& doc);
  static void send(const String& payload);
  static bool sendState(const JsonDocument& doc);
  static bool send(const String& topic, const String& payload, bool retained = false);
};

#endif
