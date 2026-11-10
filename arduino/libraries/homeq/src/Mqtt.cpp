#include "Mqtt.h"
#include "Certs.h"
#include <time.h>

namespace {
const time_t validTime = 1700000000;

Mqtt::CallbackFunction mqttCallback;
Mqtt::ConnectFunction connectCallback;
Mqtt::RetireFunction retireCallback;
WiFiClientSecure secureClient;
#ifdef ESP8266
BearSSL::Session tlsSession;
BearSSL::X509List trustAnchors;
#else
const String trustAnchors = String(ISRG_ROOT_YR) + ISRG_ROOT_X1;
#endif
PubSubClient client(secureClient);

void receive(char* topic, byte* payload, unsigned int length) {
  if (length == 0) return;
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  if (error) {
    Serial.print("json deserialize failed: ");
    Serial.println(error.c_str());
    return;
  }
  Mqtt::handleMessage(String(topic), doc);
}

int lastSslError(char* text, size_t length) {
#ifdef ESP8266
  return secureClient.getLastSSLError(text, length);
#else
  return secureClient.lastError(text, length);
#endif
}

bool lookup(const String& host, IPAddress& ip) {
#ifdef ESP8266
  return WiFi.hostByName(host.c_str(), ip, 2000);
#else
  return WiFi.hostByName(host.c_str(), ip);
#endif
}
}

String Mqtt::topic(const String& kind, uint8_t index) {
  uint8_t i = legacy || index >= channelCount ? 0 : index;
  return home + "/" + kind + "/" + channelIds[i];
}

void Mqtt::handleConnection() {
  if (retired || isConnected()) return;
  time_t now = time(nullptr);
  if (now < validTime) return;
  if (retryDelay > 0 && millis() - retryTimer < retryDelay) return;
  retryTimer = millis();
#ifdef ESP8266
  secureClient.setX509Time(now);
#endif
  IPAddress brokerIp;
  if (lookup(broker, brokerIp) && connect()) {
    Serial.println("MQTT connected, checking owner");
    retryDelay = 0;
    for (uint8_t i = 0; i < channelCount; i++) {
      client.subscribe(topic("owner", i).c_str(), 1);
    }
    ownerPending = true;
    ownerTimer = millis();
  } else {
    retryDelay = retryDelay == 0 ? minRetryDelay : min(retryDelay * 2, maxRetryDelay);
    char sslError[64] = "";
    int sslCode = lastSslError(sslError, sizeof(sslError));
    Serial.println("MQTT connection failed, rc=" + String(client.state()) + ", ssl=" + String(sslCode) + " " + String(sslError) + ", retrying in " + String(retryDelay / 1000) + "s");
  }
}

void Mqtt::finishConnect() {
  ownerPending = false;
  for (uint8_t i = 0; i < channelCount; i++) {
    client.subscribe(topic("action", i).c_str(), 1);
  }
  Serial.println("MQTT ready");
  connectCallback(!connectedBefore);
  connectedBefore = true;
}

void Mqtt::init(CallbackFunction function, ConnectFunction onConnect, RetireFunction onRetire,
                const String* ids, uint8_t count, bool legacyIds, const String& board) {
  broker = Persistence::get(Persistence::MQTT_BROKER);
  username = Persistence::get(Persistence::MQTT_USER);
  password = Persistence::get(Persistence::MQTT_PASSWORD);
  home = "home/" + Persistence::get(Persistence::HOME_ID);
  channelIds = ids;
  channelCount = count;
  legacy = legacyIds;
  boardId = board;
  clientId = board;

  configTime(0, 0, "pool.ntp.org", "time.google.com");
#ifdef ESP8266
  trustAnchors.append(ISRG_ROOT_YR);
  trustAnchors.append(ISRG_ROOT_X1);
  secureClient.setTrustAnchors(&trustAnchors);
  secureClient.setSession(&tlsSession);
  WiFiClient::setDefaultNoDelay(true);
#else
  secureClient.setCACert(trustAnchors.c_str());
#endif
  client.setServer(broker.c_str(), 8883);
  client.setSocketTimeout(5);
  mqttCallback = function;
  connectCallback = onConnect;
  retireCallback = onRetire;
  client.setCallback(receive);
  handleConnection();
}

bool Mqtt::connect() {
  Serial.println("Connecting MQTT");
  if (!client.connect(clientId.c_str(), username.c_str(), password.c_str(), topic("state", 0).c_str(), 1, true, "{\"status\":\"offline\"}")) return false;
#ifndef ESP8266
  secureClient.setNoDelay(true);
#endif
  return true;
}

bool Mqtt::isConnected() {
  return client.connected();
}

bool Mqtt::isRetired() {
  return retired;
}

bool Mqtt::announceReset(unsigned long waitTime) {
  if (retired || broker.isEmpty()) return false;
  unsigned long start = millis();
  while (!isConnected() && millis() - start < waitTime) {
    handleConnection();
    delay(10);
  }
  while (isConnected() && ownerPending && !retired && millis() - ownerTimer < ownerWait) {
    client.loop();
    delay(10);
  }
  if (retired || !isConnected()) return false;
  time_t now = time(nullptr);
  bool sent = true;
  for (uint8_t i = 0; i < channelCount; i++) {
    StaticJsonDocument<128> doc;
    doc["status"] = "offline";
    doc["mode"] = "reset";
    if (now >= validTime) doc["ts"] = (unsigned long)now;
    sent = sendState(doc, i) && sent;
  }
  client.loop();
  client.disconnect();
  delay(250);
  return sent;
}

void Mqtt::poll() {
  handleConnection();
  if (!isConnected()) return;
  client.loop();
  if (ownerPending && !retired && millis() - ownerTimer >= ownerWait) finishConnect();
}

void Mqtt::handleMessage(const String& topic, JsonDocument& doc) {
  String ownerPrefix = home + "/owner/";
  if (topic.startsWith(ownerPrefix)) {
    String owner = doc["board"] | "";
    if (owner.isEmpty() || owner == boardId || retired) return;
    Serial.println("owned by board " + owner + ", retiring");
    retired = true;
    ownerPending = false;
    client.disconnect();
    retireCallback(owner);
    return;
  }
  String actionPrefix = home + "/action/";
  if (retired || !topic.startsWith(actionPrefix)) return;
  String id = topic.substring(actionPrefix.length());
  int16_t index = -1;
  for (uint8_t i = 0; i < channelCount; i++) {
    if (channelIds[i] == id) index = i;
  }
  if (index < 0) return;
  mqttCallback(doc, legacy ? (doc["index"] | 0) : index);
  String reply;
  serializeJson(doc, reply);
  send(home + "/reaction/" + id, reply);
}

bool Mqtt::sendState(const JsonDocument& doc, uint8_t channel) {
  String state;
  serializeJson(doc, state);
  return send(topic("state", channel), state, true);
}

bool Mqtt::send(const String& topic, const String& payload, bool retained) {
  if (retired || !isConnected()) return false;
  Serial.println("sending " + payload);
  return client.publish(topic.c_str(), payload.c_str(), retained);
}
