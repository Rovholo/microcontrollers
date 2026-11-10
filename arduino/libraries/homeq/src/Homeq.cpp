#include "Homeq.h"
#include "Persistence.h"
#include "Control.h"
#include "Mqtt.h"
#include "Http.h"
#include "Lan.h"
#include "BleSetup.h"
#include <ArduinoJson.h>
#include <time.h>

namespace {
const String action = "action";
const String status = "status";
const String mode = "mode";
const String operate = "operate";
const String on = "on";
const String off = "off";
const String closedStatus = "closed";
const String openStatus = "open";
const String idx = "index";
const String garageDoorType = "garage_door";
const String lightType = "light";
const String sensorType = "sensor";
const unsigned long offlineTime = 4UL * 60 * 60 * 1000;
const unsigned long blinkTime = 5000;
const unsigned long retiredBlinkTime = 600;
const unsigned long inputStableTime = 2000;
const unsigned long switchStableTime = 50;
const unsigned long handoverTime = 5000;
const uint8_t recentIdCount = 8;
const unsigned long repeatTime = 10 * 1000;
const unsigned long remoteCauseTime = 60 * 1000;
const unsigned long resetAnnounceTime = 5000;
const time_t validTime = 1700000000;
const String causeKey = "cause";
const String timeKey = "ts";
const String remoteCause = "remote";
const String inputCause = "input";

const Board* board = nullptr;
String boardId;
String* channelIds = nullptr;
DeviceType* channelTypes = nullptr;
uint8_t channelCount = 0;
bool legacyIds = false;
bool retired = false;
unsigned long offlineTimer = 0;
unsigned long blinkTimer = 0;
String* prevStatuses;
String* readings;
unsigned long* readingTimers;
String* causes;
unsigned long* remoteTimers;
time_t* changeTimes;
bool* switchStates;
bool* switchReadings;
unsigned long* switchTimers;
String recentIds[recentIdCount];
unsigned long recentIdTimers[recentIdCount];
uint8_t recentIdIndex = 0;

bool parseType(const String& value, DeviceType& type) {
  if (value == garageDoorType) {
    type = DeviceType::GARAGE_DOOR;
  } else if (value == lightType) {
    type = DeviceType::LIGHT;
  } else if (value == sensorType) {
    type = DeviceType::SENSOR;
  } else {
    return false;
  }
  return true;
}

String typeName(DeviceType type) {
  switch (type) {
    case DeviceType::LIGHT: return lightType;
    case DeviceType::SENSOR: return sensorType;
    default: return garageDoorType;
  }
}

String readBoardId() {
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  mac.toLowerCase();
  return mac;
}

DeviceType storedType() {
  DeviceType type;
  return parseType(Persistence::get(Persistence::TYPE), type) ? type : board->defaultType;
}

void loadChannels() {
  String stored = Persistence::get(Persistence::CHANNELS);
  if (stored == "null") stored = "";
  uint8_t count = stored.isEmpty() ? 0 : 1;
  for (size_t i = 0; i < stored.length(); i++) {
    if (stored[i] == ',') count++;
  }
  if (count > board->channels) count = board->channels;
  legacyIds = count == 0;
  channelCount = legacyIds ? board->channels : count;
  channelIds = new String[channelCount];
  channelTypes = new DeviceType[channelCount];
  if (legacyIds) {
    DeviceType type = storedType();
    for (uint8_t i = 0; i < channelCount; i++) {
      channelIds[i] = Persistence::get(Persistence::ID);
      channelTypes[i] = type;
    }
    return;
  }
  int start = 0;
  for (uint8_t i = 0; i < channelCount; i++) {
    int end = stored.indexOf(',', start);
    if (end < 0) end = stored.length();
    String entry = stored.substring(start, end);
    start = end + 1;
    int split = entry.indexOf('|');
    channelIds[i] = split < 0 ? entry : entry.substring(0, split);
    if (split < 0 || !parseType(entry.substring(split + 1), channelTypes[i])) channelTypes[i] = board->defaultType;
  }
}

bool readChannels(JsonDocument& doc, String& stored, String& firstId) {
  JsonArray channels = doc[Persistence::CHANNELS].as<JsonArray>();
  if (channels.isNull()) {
    String id = doc["id"] | "";
    String type = doc[Persistence::TYPE] | "";
    DeviceType parsed;
    if (id.isEmpty() || (!type.isEmpty() && !parseType(type, parsed))) return false;
    stored = id + "|" + (type.isEmpty() ? typeName(board->defaultType) : type);
    firstId = id;
    return true;
  }
  if (channels.size() == 0 || channels.size() > board->channels) return false;
  stored = "";
  for (JsonObject channel : channels) {
    String id = channel["id"] | "";
    String type = channel["type"] | "";
    DeviceType parsed;
    if (id.isEmpty() || id.indexOf(',') >= 0 || id.indexOf('|') >= 0 || !parseType(type, parsed)) return false;
    if (stored.isEmpty()) firstId = id;
    if (!stored.isEmpty()) stored += ",";
    stored += id + "|" + type;
  }
  return true;
}

void handleCredential(JsonDocument& doc) {
  String secret = doc[Persistence::SECRET] | "";
  doc.remove(Persistence::SECRET);
  String channels;
  String firstId;
  if (!readChannels(doc, channels, firstId)) {
    Serial.println("handleCredential: unsupported channels");
    doc.clear();
    doc["success"] = false;
    doc["board"] = boardId;
    return;
  }
  Serial.println("handleCredential: joining " + doc[Persistence::WIFI_SSID].as<String>());
  bool connected = Lan::connectWifi(doc[Persistence::WIFI_SSID], doc[Persistence::WIFI_PASSWORD]);
  if (connected) {
    Persistence::set(Persistence::WIFI_SSID, doc[Persistence::WIFI_SSID]);
    Persistence::set(Persistence::WIFI_PASSWORD, doc[Persistence::WIFI_PASSWORD]);
    Persistence::set(Persistence::ID, firstId);
    Persistence::set(Persistence::HOME_ID, doc[Persistence::HOME_ID]);
    Persistence::set(Persistence::MQTT_BROKER, doc[Persistence::MQTT_BROKER]);
    Persistence::set(Persistence::MQTT_USER, doc[Persistence::MQTT_USER]);
    Persistence::set(Persistence::MQTT_PASSWORD, doc[Persistence::MQTT_PASSWORD]);
    Persistence::set(Persistence::SECRET, secret);
    Persistence::set(Persistence::CHANNELS, channels);
    Persistence::set(Persistence::ACTIVE, String(true));
    Persistence::save();
  }
  doc.clear();
  doc["success"] = connected;
  doc["board"] = boardId;
}

String readStatus(const uint8_t& i) {
  if (channelTypes[i] == DeviceType::LIGHT) return Control::isRelayOn(i) ? on : off;
  return Control::isInputActive(i) ? closedStatus : openStatus;
}

time_t clockTime() {
  time_t now = time(nullptr);
  return now >= validTime ? now : 0;
}

void markRemote(const uint8_t& i) {
  causes[i] = remoteCause;
  remoteTimers[i] = millis();
}

String changeCause(const uint8_t& i) {
  if (channelTypes[i] == DeviceType::LIGHT) return causes[i];
  return causes[i] == remoteCause && millis() - remoteTimers[i] < remoteCauseTime ? remoteCause : inputCause;
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

void runRequest(JsonDocument& doc, int16_t i) {
  Serial.print("request for channel " + String(i) + ": ");
  serializeJson(doc, Serial);
  Serial.println("");
  String requested = doc[action] | "";
  String msgId = doc["id"] | "";
  bool success = !retired && i >= 0 && i < channelCount;
  if (success) {
    DeviceType type = channelTypes[i];
    if (requested == status) {
    } else if (type == DeviceType::GARAGE_DOOR && requested == operate) {
      if (!isRepeat(msgId) && Control::relayPulse(i)) markRemote(i);
    } else if (type == DeviceType::LIGHT && (requested == on || requested == off)) {
      if (Control::isRelayOn(i) != (requested == on)) markRemote(i);
      Control::relaySet(i, requested == on);
    } else {
      success = false;
    }
  }
  doc.clear();
  if (!msgId.isEmpty()) doc["id"] = msgId;
  doc["success"] = success;
  if (success) doc[status] = readStatus(i);
}

void handleMqttRequest(JsonDocument& doc, uint8_t i) {
  runRequest(doc, i);
}

void handleHttpRequest(JsonDocument& doc) {
  int16_t channel = -1;
  if (legacyIds) {
    channel = doc[idx] | 0;
  } else {
    String device = doc["device"] | "";
    for (uint8_t i = 0; i < channelCount; i++) {
      if (channelIds[i] == device) channel = i;
    }
  }
  runRequest(doc, channel);
}

void handleSwitches() {
  for (uint8_t i = 0; i < channelCount; i++) {
    if (channelTypes[i] != DeviceType::LIGHT) continue;
    bool reading = Control::isInputActive(i);
    if (reading != switchReadings[i]) {
      switchReadings[i] = reading;
      switchTimers[i] = millis();
    } else if (reading != switchStates[i] && millis() - switchTimers[i] >= switchStableTime) {
      switchStates[i] = reading;
      causes[i] = inputCause;
      Control::relaySet(i, !Control::isRelayOn(i));
    }
  }
}

void handleStatusChanges() {
  for (uint8_t i = 0; i < channelCount; i++) {
    unsigned long stableTime = channelTypes[i] == DeviceType::LIGHT ? 0 : inputStableTime;
    String reading = readStatus(i);
    if (reading != readings[i]) {
      readings[i] = reading;
      readingTimers[i] = millis();
      changeTimes[i] = clockTime();
    } else if (reading != prevStatuses[i] && millis() - readingTimers[i] >= stableTime) {
      StaticJsonDocument<256> doc;
      doc[status] = reading;
      doc["prevStatus"] = prevStatuses[i];
      doc[causeKey] = changeCause(i);
      if (changeTimes[i] > 0) doc[timeKey] = (unsigned long)changeTimes[i];
      if (Mqtt::sendState(doc, i)) {
        prevStatuses[i] = reading;
      }
    }
  }
}

void handleButton() {
  switch (Control::checkButton()) {
    case ButtonEvent::PRESS:
      Serial.println("reset button pressed");
      Control::restart();
      break;
    case ButtonEvent::HOLD:
      Serial.println("reset button held, clearing settings");
      if (Persistence::get(Persistence::ACTIVE).toInt() != 0 && Mqtt::announceReset(resetAnnounceTime)) {
        Serial.println("told HomeQ this device was reset");
      }
      Persistence::clearJson();
      Control::restart();
      break;
    default:
      break;
  }
}

void handleRecovery() {
  handleButton();
  if (millis() - blinkTimer >= (retired ? retiredBlinkTime : blinkTime)) {
    blinkTimer = millis();
    Control::ledBlink();
  }
  if (retired) return;
  if (millis() - offlineTimer >= offlineTime) {
    offlineTimer = millis();
    if (!Http::isConnected()) {
      Control::restart();
    }
  }
}

void onMqttConnect(bool firstConnect) {
  for (uint8_t i = 0; i < channelCount; i++) {
    StaticJsonDocument<256> doc;
    doc[status] = readStatus(i);
    if (firstConnect) prevStatuses[i] = doc[status].as<String>();
    doc[mode] = "connect";
    time_t now = clockTime();
    if (now > 0) doc[timeKey] = (unsigned long)now;
    Mqtt::sendState(doc, i);
  }
}

void onRetire(const String& owner) {
  retired = true;
  for (uint8_t i = 0; i < Control::channelCount(); i++) {
    Control::relaySet(i, false);
  }
  Http::stopMdns();
  Serial.println("retired: board " + owner + " now runs this device. Hold reset for 5 seconds to set it up again");
}

String hotspotName() {
  char name[16];
#ifdef ESP8266
  snprintf(name, sizeof(name), "homeQ-%06X", ESP.getChipId());
#else
  uint64_t mac = ESP.getEfuseMac();
  snprintf(name, sizeof(name), "homeQ-%06X", (uint32_t)((mac >> 24) & 0xFFFFFF));
#endif
  return String(name);
}

void provision() {
  StaticJsonDocument<128> about;
  about["channels"] = board->channels;
  about["board"] = boardId;
  String aboutJson;
  serializeJson(about, aboutJson);
  Http::setAbout(aboutJson);
  Lan::createHotspot(hotspotName(), "abcd1234", Lan::scanChannel());
  Http::init(handleCredential, false);
  BleSetup::begin(hotspotName(), aboutJson, handleCredential);
  while (Persistence::get(Persistence::ACTIVE).toInt() == 0) {
    Http::poll();
    BleSetup::poll();
    handleButton();
    delay(1);
  }
  unsigned long handoverTimer = millis();
  while (millis() - handoverTimer < handoverTime) {
    Http::poll();
    BleSetup::poll();
    delay(1);
  }
  BleSetup::end();
  Lan::disconnectHotspot();
}
}

void Homeq::begin(const Board& b) {
  board = &b;
  Serial.begin(115200);
  Serial.println();
  Serial.println("init start");
  Persistence::init();
  Control::init(b);
  boardId = readBoardId();
  Serial.println("board " + boardId);

  if (Persistence::get(Persistence::ACTIVE).toInt() == 0) {
    provision();
    Lan::stationOnly();
  } else {
    Lan::stationOnly();
    Lan::connectWifi(Persistence::get(Persistence::WIFI_SSID), Persistence::get(Persistence::WIFI_PASSWORD));
  }
  loadChannels();
  for (uint8_t i = 0; i < channelCount; i++) {
    Serial.println("channel " + String(i) + ": " + channelIds[i] + " " + typeName(channelTypes[i]));
  }
  Http::init(handleHttpRequest, true);

  prevStatuses = new String[channelCount];
  readings = new String[channelCount];
  readingTimers = new unsigned long[channelCount];
  causes = new String[channelCount];
  remoteTimers = new unsigned long[channelCount];
  changeTimes = new time_t[channelCount];
  switchStates = new bool[channelCount];
  switchReadings = new bool[channelCount];
  switchTimers = new unsigned long[channelCount];
  for (uint8_t i = 0; i < channelCount; i++) {
    prevStatuses[i] = readStatus(i);
    readings[i] = prevStatuses[i];
    readingTimers[i] = millis();
    causes[i] = inputCause;
    remoteTimers[i] = 0;
    changeTimes[i] = 0;
    switchStates[i] = Control::isInputActive(i);
    switchReadings[i] = switchStates[i];
    switchTimers[i] = millis();
  }
  offlineTimer = millis();
  blinkTimer = millis();
  Mqtt::init(handleMqttRequest, onMqttConnect, onRetire, channelIds, legacyIds ? 1 : channelCount, legacyIds, boardId);
  Serial.println("init done");
}

void Homeq::loop() {
  Http::poll();
  if (!retired) {
    Mqtt::poll();
    handleSwitches();
    handleStatusChanges();
  }
  handleRecovery();
}
