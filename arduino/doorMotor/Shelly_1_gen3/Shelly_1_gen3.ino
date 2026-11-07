#include "Persistence.h"
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
const long offlineTime = 4 * 60 * 60 * 1000;
const long blinkTime = 5000;
const long stableTime = 2000;
const unsigned long handoverTime = 5000;

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
  String secret = doc[Persistence::SECRET] | "";
  doc.remove(Persistence::SECRET);
  Serial.println("handleCredential: " + doc[Persistence::NAME].as<String>() + " on " + doc[Persistence::WIFI_SSID].as<String>());
  bool connected = Lan::connectWifi(doc[Persistence::WIFI_SSID], doc[Persistence::WIFI_PASSWORD]);
  if (connected) {
    Persistence::set(Persistence::WIFI_SSID, doc[Persistence::WIFI_SSID]);
    Persistence::set(Persistence::WIFI_PASSWORD, doc[Persistence::WIFI_PASSWORD]);
    Persistence::set(Persistence::NAME, doc[Persistence::NAME]);
    Persistence::set(Persistence::ID, doc[Persistence::ID]);
    Persistence::set(Persistence::HOME_ID, doc[Persistence::HOME_ID]);
    Persistence::set(Persistence::MQTT_BROKER, doc[Persistence::MQTT_BROKER]);
    Persistence::set(Persistence::MQTT_USER, doc[Persistence::MQTT_USER]);
    Persistence::set(Persistence::MQTT_PASSWORD, doc[Persistence::MQTT_PASSWORD]);
    Persistence::set(Persistence::SECRET, secret);
    Persistence::set(Persistence::ACTIVE, String(true));
    Persistence::save();
  }
  doc.clear();
  doc["success"] = connected;
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
  Control::chechButton();
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

String hotspotName() {
  uint64_t mac = ESP.getEfuseMac();
  char name[16];
  snprintf(name, sizeof(name), "homeQ-%06X", (uint32_t)((mac >> 24) & 0xFFFFFF));
  return String(name);
}

void provision() {
  Lan::createHotspot(hotspotName(), "abcd1234", Lan::scanChannel());
  Http::init(handleCredential, false);
  while (Persistence::get(Persistence::ACTIVE).toInt() == 0) {
    Http::poll();
    Control::chechButton();
    delay(1);
  }
  unsigned long handoverTimer = millis();
  while (millis() - handoverTimer < handoverTime) {
    Http::poll();
    delay(1);
  }
  Lan::disconnectHotspot();
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("init start");
  Persistence::init();
  Control::init();

  if (Persistence::get(Persistence::ACTIVE).toInt() == 0) {
    provision();
  } else {
    Lan::connectWifi(Persistence::get(Persistence::WIFI_SSID), Persistence::get(Persistence::WIFI_PASSWORD));
  }
  Http::init(handleRequest, true);

  prevStatuses = new String[Control::relayCount()];
  readings = new String[Control::relayCount()];
  readingTimers = new unsigned long[Control::relayCount()];
  for (uint8_t i = 0; i < Control::relayCount(); i++) {
    prevStatuses[i] = readStatus(i);
    readings[i] = prevStatuses[i];
    readingTimers[i] = millis();
  }
  Mqtt::init(handleRequest, onMqttConnect);
  Serial.println("init done");
}

void loop() {
  Http::poll();
  Mqtt::poll();
  handleStatusChanges();
  handleRecovery();
}
