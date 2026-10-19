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
const long blinkTime = 5000; // ms
const long stableTime = 1500; // ms

unsigned long offlineTimer = millis();
unsigned long blinkTimer = millis();
String* prevStatuses;
String* readings;
unsigned long* readingTimers;
const uint8_t recentIdCount = 8;
const unsigned long repeatTime = 10 * 1000;
String recentIds[recentIdCount];
unsigned long recentIdTimers[recentIdCount];
uint8_t recentIdIndex = 0;

void handleCredential(JsonDocument& doc) {
  String secret = doc[Persistance::SECRET] | "";
  doc.remove(Persistance::SECRET);
  Serial.println("handleCredential: ");
  serializeJson(doc, Serial);
  if (Lan::connectWifi(doc[Persistance::WIFI_SSID], doc[Persistance::WIFI_PASSWORD])) {
    Persistance::set(Persistance::WIFI_SSID, doc[Persistance::WIFI_SSID]);
    Persistance::set(Persistance::WIFI_PASSWORD, doc[Persistance::WIFI_PASSWORD]);
    Persistance::set(Persistance::NAME, doc[Persistance::NAME]);
    Persistance::set(Persistance::ID, doc[Persistance::ID]);
    Persistance::set(Persistance::HOME_ID, doc[Persistance::HOME_ID]);
    Persistance::set(Persistance::MQTT_BROKER, doc[Persistance::MQTT_BROKER]);
    Persistance::set(Persistance::MQTT_USER, doc[Persistance::MQTT_USER]);
    Persistance::set(Persistance::MQTT_PASSWORD, doc[Persistance::MQTT_PASSWORD]);
    Persistance::set(Persistance::SECRET, secret);
    Persistance::set(Persistance::ACTIVE, String(true));
    Persistance::save();
    doc["success"] = true;
  } else {
    doc["success"] = false;
  }
}

String readStatus(const uint8_t& i) {
  return Control::isClosed(i) ? "closed" : "open";
}

void writeStatus(JsonDocument& doc, const uint8_t& i) {
  doc[idx] = i;
  doc[status] = readStatus(i);
}

bool isRepeat(const String& id) {
  if (id.isEmpty()) return false;
  for (uint8_t i = 0; i < recentIdCount; i++) {
    if (recentIds[i] == id && millis() - recentIdTimers[i] < repeatTime) return true;
  }
  recentIds[recentIdIndex] = id;
  recentIdTimers[recentIdIndex] = millis();
  recentIdIndex = (recentIdIndex + 1) % recentIdCount;
  return false;
}

void handleRequest(JsonDocument& doc) {
  Serial.print("handleRequest: ");
  serializeJson(doc, Serial);
  Serial.println("");
  uint8_t i = doc[idx].as<uint8_t>();
  if (i >= Control::relayCount()) {
    doc["success"] = false;
    return;
  }
  if (doc[action] == operate) {
    String id = doc["id"] | "";
    if (!isRepeat(id)) Control::relayPulse(i);
  } else if (doc[action] != status) {
    doc["success"] = false;
    return;
  }
  writeStatus(doc, i);
  doc["success"] = true;
}

void handleStatusChanges() {
  for (uint8_t i = 0; i < Control::relayCount(); i++) {
    String reading = readStatus(i);
    if (reading != readings[i]) {
      readings[i] = reading;
      readingTimers[i] = millis();
    } else if (reading != prevStatuses[i] && millis() - readingTimers[i] >= stableTime) {
      StaticJsonDocument<256> doc;
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
    StaticJsonDocument<256> doc;
    writeStatus(doc, i);
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
    Http::init(handleCredential, false);
    while (Persistance::get(Persistance::ACTIVE).toInt() == 0) {
      Http::poll();
    }
    Lan::disconnectHotspot();
    Control::restart();
  }

  Lan::connectWifi(Persistance::get(Persistance::WIFI_SSID), Persistance::get(Persistance::WIFI_PASSWORD));
  Http::init(handleRequest, true);

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
