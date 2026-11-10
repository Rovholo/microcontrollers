#ifndef MQTT_H
#define MQTT_H

#include "Persistence.h"
#include <Arduino.h>
#ifdef ESP8266
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#include <WiFiClientSecure.h>
#endif
#include <ArduinoJson.h>
#include <PubSubClient.h>

class Mqtt {
private:
  inline static String broker;
  inline static String username;
  inline static String password;
  inline static String clientId;
  inline static String home;
  inline static const String* channelIds = nullptr;
  inline static uint8_t channelCount = 0;
  inline static bool legacy = false;
  inline static String boardId;
  inline static bool connectedBefore = false;
  inline static bool ownerPending = false;
  inline static bool retired = false;
  inline static unsigned long ownerTimer = 0;
  inline static const unsigned long ownerWait = 2 * 1000;
  inline static const unsigned long minRetryDelay = 5 * 1000;
  inline static const unsigned long maxRetryDelay = 60 * 1000;
  inline static unsigned long retryTimer = 0;
  inline static unsigned long retryDelay = 0;
  static void handleConnection();
  static void finishConnect();
  static bool connect();
  static bool isConnected();
  static String topic(const String& kind, uint8_t index);
public:
  typedef void (*CallbackFunction)(JsonDocument& doc, uint8_t channel);
  typedef void (*ConnectFunction)(bool firstConnect);
  typedef void (*RetireFunction)(const String& owner);
  static void init(CallbackFunction function, ConnectFunction onConnect, RetireFunction onRetire,
                   const String* ids, uint8_t count, bool legacyIds, const String& board);
  static void poll();
  static void handleMessage(const String& topic, JsonDocument& doc);
  static bool sendState(const JsonDocument& doc, uint8_t channel);
  static bool send(const String& topic, const String& payload, bool retained = false);
  static bool isRetired();
  static bool announceReset(unsigned long waitTime);
};

#endif
