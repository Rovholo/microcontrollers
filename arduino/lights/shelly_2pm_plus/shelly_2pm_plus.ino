#include "Persistence.h"
#include "Control.h"
#include "Mqtt.h"
#include "Http.h"
#include "Lan.h"
#include <ArduinoJson.h>

const String action= "action";
const String status = "status";
const String on = "on";
const String off = "off";
const String idx = "index";
const String about = "about";
const long offlineTime = 4 * 60 * 60 * 1000; // h * min * s * ms
const long statusTime = 5 * 1000;

unsigned long offlineTimer = millis();
unsigned long statusTimer = millis();
String* prevStatuses;

DynamicJsonDocument handleCredential(DynamicJsonDocument doc) {
  Serial.print("handleCredential: ");
  serializeJson(doc, Serial);
  Serial.println("");
  if (Lan::connectWifi(doc[Persistence::WIFI_SSID], doc[Persistence::WIFI_PASSWORD])) {
    Persistence::persist(Persistence::WIFI_SSID, doc[Persistence::WIFI_SSID]);
    Persistence::persist(Persistence::WIFI_PASSWORD, doc[Persistence::WIFI_PASSWORD]);
    Persistence::persist(Persistence::NAME, doc[Persistence::NAME]);
    Persistence::persist(Persistence::ID, doc[Persistence::ID]);
    Persistence::persist(Persistence::HOME_ID, doc[Persistence::HOME_ID]);
    Persistence::persist(Persistence::MQTT_BROKER, doc[Persistence::MQTT_BROKER]);
    Persistence::persist(Persistence::MQTT_USER, doc[Persistence::MQTT_USER]);
    Persistence::persist(Persistence::MQTT_PASSWORD, doc[Persistence::MQTT_PASSWORD]);
    Persistence::persist(Persistence::ACTIVE, String(true));
    doc["success"] = true;
  } else {
    doc["success"] = false;
  }
  return doc;
}

DynamicJsonDocument startDevice(const uint8_t& i, String value) {
  Control::relayToggle(i, value == on);
  return getStatus(i);
}

DynamicJsonDocument getStatus(const uint8_t& i) {
  DynamicJsonDocument doc(1024);
  doc[idx] = i;
  doc[status] = Control::chechStatus(i) ? on : off;
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
  } else if (doc[action] == on) {
    resp = startDevice(doc[idx].as<uint8_t>(), on);
  } else if (doc[action] == off) {
    resp = startDevice(doc[idx].as<uint8_t>(), off);
  } else {
    resp["success"] = false;
  }

  for (JsonPair kv : resp.as<JsonObject>()) {
    doc[kv.key()] = kv.value();
  }
  return doc;
}

void handleRecovery() {
  if (millis() - statusTimer >= statusTime) {
    statusTimer = millis();
    Control::ledBlink();
    for (uint8_t i = 0; i < Control::relayCount(); i++) {
      DynamicJsonDocument resp = getStatus(i);
      if (prevStatuses[i] != resp[status]) {
        resp[Persistence::NAME] = Persistence::get(Persistence::NAME);
        resp["old"] = prevStatuses[i];
        prevStatuses[i] = resp[status].as<String>();
        Mqtt::send(resp);
      }
    }
  }

  for (uint8_t i = 0; i < Control::relayCount(); i++) {
    Control::checkSwitch(i);
  }
  
  if (millis() - offlineTimer >= offlineTime) {
    offlineTimer = millis();
    if (!Http::isConnected()) {
      Control::restart();
    }
  }
}

void handleSwitch() {
  for (uint8_t i = 0; i < Control::relayCount(); i++) {
    DynamicJsonDocument resp = getStatus(i);
    if (prevStatuses[i] != resp[status]) {
      resp[Persistence::NAME] = Persistence::get(Persistence::NAME);
      resp["old"] = prevStatuses[i];
      prevStatuses[i] = resp[status].as<String>();
      Mqtt::send(resp);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("homeQ");
  Persistence::init();
  Control::init();

  if (Persistence::get(Persistence::ACTIVE).toInt() == 0) {
    Lan::createHotspot("homeQ","abcd1234");
    Http::init(handleCredential);
    while (Persistence::get(Persistence::ACTIVE).toInt() == 0) {
      Http::poll();
    }
    Lan::disconnectHotspot();
    Control::restart();
  }

  Lan::connectWifi(Persistence::get(Persistence::WIFI_SSID), Persistence::get(Persistence::WIFI_PASSWORD));
  Http::init(handleRequest);
  Mqtt::init(handleRequest);

  prevStatuses = new String[Control::relayCount()];
  for (uint8_t i = 0; i < Control::relayCount(); i++) {
    DynamicJsonDocument doc = getStatus(i);
    prevStatuses[i] = doc[status].as<String>();
    Mqtt::send(doc);
  }
  Serial.println("init done");
}

void loop() {
  Http::poll();
  Mqtt::poll();
  handleRecovery();
}
