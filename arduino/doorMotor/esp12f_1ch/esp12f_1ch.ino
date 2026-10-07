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
const long statusTime = 5 * 1000;

unsigned long offlineTimer = millis();
unsigned long statusTimer = millis();
String* prevStatuses;

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

DynamicJsonDocument getStatus(const uint8_t& i) {
  DynamicJsonDocument doc(1024);
  doc[idx] = i;
  doc[status] = Control::isClosed(i) ? "closed" : "open";
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

void handleRecovery() {
  if (millis() - statusTimer >= statusTime) {
    statusTimer = millis();
    Control::ledBlink();
    for (uint8_t i = 0; i < Control::relayCount(); i++) {
      DynamicJsonDocument resp = getStatus(i);
      if (prevStatuses[i] != resp[status]) {
        resp["prevStatus"] = prevStatuses[i];
        prevStatuses[i] = resp[status].as<String>();
        Mqtt::send(resp);
      }
    }
  }
  if (millis() - offlineTimer >= offlineTime) {
    offlineTimer = millis();
    if (!Http::isConnected()) {
      Control::restart();
    }
  }
}

// Called on the first mqtt connection after boot, so the start reactions aren't lost if mqtt isn't connected yet.
void sendStart() {
  for (uint8_t i = 0; i < Control::relayCount(); i++) {
    DynamicJsonDocument doc = getStatus(i);
    prevStatuses[i] = doc[status].as<String>();
    doc[mode] = "start";
    Mqtt::send(doc);
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

  // Set before Mqtt::init, which may connect straight away and call sendStart.
  prevStatuses = new String[Control::relayCount()];
  for (uint8_t i = 0; i < Control::relayCount(); i++) {
    prevStatuses[i] = getStatus(i)[status].as<String>();
  }
  Mqtt::init(handleRequest, sendStart);
}

void loop() {
  Http::poll();
  Mqtt::poll();
  handleRecovery();
}
