#include "Persistance.h"
#include "Control.h"
#include "Mqtt.h"
#include "Http.h"
#include "Lan.h"
#include <ArduinoJson.h>

const String action= "action";
const String status = "status";
const String mode = "mode";
const String operate = "operate";
const String idx = "index";
const String about = "about";
const long offlineTime = 4 * 60 * 60 * 1000; // h * min * s * ms
const long blinkTime = 5 * 1000;
const long stableTime = 3 * 1000;

unsigned long offlineTimer = millis();
unsigned long blinkTimer = millis();
String* prevStatuses;
String* readings;
unsigned long* readingTimers;

DynamicJsonDocument handleCredential(DynamicJsonDocument doc) {
  Serial.println("handleCredential: ");
  serializeJson(doc, Serial);
  if (Lan::connectWifi(doc[Persistance::WIFI_SSID], doc[Persistance::WIFI_PASSWORD])) {
    Persistance::persist(Persistance::WIFI_SSID, doc[Persistance::WIFI_SSID]);
    Persistance::persist(Persistance::WIFI_PASSWORD, doc[Persistance::WIFI_PASSWORD]);
    Persistance::persist(Persistance::NAME, doc[Persistance::NAME]);
    Persistance::persist(Persistance::ID, doc[Persistance::ID]);
    Persistance::persist(Persistance::HOME_ID, doc[Persistance::HOME_ID]);
    Persistance::persist(Persistance::MQTT_BROKER, doc[Persistance::MQTT_BROKER]);
    Persistance::persist(Persistance::MQTT_USER, doc[Persistance::MQTT_USER]);
    Persistance::persist(Persistance::MQTT_PASSWORD, doc[Persistance::MQTT_PASSWORD]);
    Persistance::persist(Persistance::ACTIVE, String(true));
    doc["success"] = true;
  } else {
    doc["success"] = false;
  }
  return doc;
}

DynamicJsonDocument startDoor(const uint8_t& i) {
  Control::relayPulse(i);
  return getStatus(i);
}

String readStatus(const uint8_t& i) {
  return Control::isClosed(i) ? "closed" : "open";
}

DynamicJsonDocument getStatus(const uint8_t& i) {
  DynamicJsonDocument doc(1024);
  doc[idx] = i;
  doc[status] = readStatus(i);
  return doc;
}

DynamicJsonDocument handleRequest(DynamicJsonDocument doc) {
  Serial.print("handleRequest: ");
  serializeJson(doc, Serial);
  Serial.println("");
  DynamicJsonDocument resp(1024);
  doc["success"] = true;
  if (doc[action] == status) {
    resp = getStatus(doc[idx].as<uint8_t>());
  } else if (doc[action] == operate) {
    resp = startDoor(doc[idx].as<uint8_t>());
  } else {
    resp["success"] = false;
  }

  for (JsonPair kv : resp.as<JsonObject>()) {
    doc[kv.key()] = kv.value();
  }

  return doc;
}

void handleStatusChanges() {
  for (uint8_t i = 0; i < Control::relayCount(); i++) {
    String reading = readStatus(i);
    if (reading != readings[i]) {
      readings[i] = reading;
      readingTimers[i] = millis();
    } else if (reading != prevStatuses[i] && millis() - readingTimers[i] >= stableTime) {
      DynamicJsonDocument doc(1024);
      doc[idx] = i;
      doc[status] = reading;
      doc["prevStatus"] = prevStatuses[i];
      if (Mqtt::sendState(doc)) {
        prevStatuses[i] = reading;
      }
    }
  }
}

void handleRecovery() {
  if (millis() - blinkTimer >= blinkTime) {
    blinkTimer = millis();
    Control::ledBlink();
  }
  if (millis() - offlineTimer >= offlineTime) {
    offlineTimer = millis();
    if (!Http::isConnected()) {
      Control::restart();
    }
  }
}

void onMqttConnect(bool firstConnect) {
  for (uint8_t i = 0; i < Control::relayCount(); i++) {
    DynamicJsonDocument doc = getStatus(i);
    if (firstConnect) prevStatuses[i] = doc[status].as<String>();
    doc[mode] = "connect";
    Mqtt::sendState(doc);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Persistance::init();
  Control::init();
  // Persistance::clearJson();

  if (Persistance::get(Persistance::ACTIVE).toInt() == 0) {
    Lan::createHotspot("homeQ","abcd1234");
    Http::init(handleCredential);
    while (Persistance::get(Persistance::ACTIVE).toInt() == 0) {
      Http::poll();
    }
    Lan::disconnectHotspot();
    Control::restart();
  }

  Lan::connectWifi(Persistance::get(Persistance::WIFI_SSID), Persistance::get(Persistance::WIFI_PASSWORD));
  Http::init(handleRequest);

  // Set before Mqtt::init, which may connect straight away and call onMqttConnect.
  prevStatuses = new String[Control::relayCount()];
  readings = new String[Control::relayCount()];
  readingTimers = new unsigned long[Control::relayCount()];
  for (uint8_t i = 0; i < Control::relayCount(); i++) {
    prevStatuses[i] = readStatus(i);
    readings[i] = prevStatuses[i];
    readingTimers[i] = millis();
  }
  Mqtt::init(handleRequest, onMqttConnect);
}

void loop() {
  Http::poll();
  Mqtt::poll();
  handleStatusChanges();
  handleRecovery();
}
