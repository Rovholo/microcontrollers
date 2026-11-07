#include "Mqtt.h"
#include "Certs.h"
#include <time.h>

const time_t validTime = 1700000000;

Mqtt::CallbackFunction mqttCallback;
Mqtt::ConnectFunction connectCallback;
WiFiClientSecure secureClient;
const String trustAnchors = String(ISRG_ROOT_YR) + ISRG_ROOT_X1;
PubSubClient client(secureClient);

void handleRequest(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  if (error) {
    Serial.print("json deserialize failed: ");
    Serial.println(error.c_str());
    return;
  }
  mqttCallback(doc);
  Mqtt::send(doc);
}

void Mqtt::handleConnection() {
  if (isConnected()) return;
  if (time(nullptr) < validTime) return;
  if (retryDelay > 0 && millis() - retryTimer < retryDelay) return;
  retryTimer = millis();
  IPAddress brokerIp;
  if (WiFi.hostByName(broker.c_str(), brokerIp) && connect(deviceId, username, password)) {
    Serial.println("MQTT connected");
    retryDelay = 0;
    subscribe(home + "/" + action + "/" + deviceId);
    connectCallback(!connectedBefore);
    connectedBefore = true;
  } else {
    retryDelay = retryDelay == 0 ? minRetryDelay : min(retryDelay * 2, maxRetryDelay);
    char sslError[64] = "";
    int sslCode = secureClient.lastError(sslError, sizeof(sslError));
    Serial.println("MQTT connection failed, rc=" + String(client.state()) + ", ssl=" + String(sslCode) + " " + String(sslError) + ", retrying in " + String(retryDelay / 1000) + "s");
  }
}

void Mqtt::init(CallbackFunction function, ConnectFunction onConnect) {
  broker = Persistence::get(Persistence::MQTT_BROKER);
  username = Persistence::get(Persistence::MQTT_USER);
  password = Persistence::get(Persistence::MQTT_PASSWORD);
  deviceId = Persistence::get(Persistence::ID);
  home = "home/" + Persistence::get(Persistence::HOME_ID);
  action = "action";
  reaction = "reaction";
  stateTopic = home + "/state/" + deviceId;
  reactionTopic = home + "/" + reaction + "/" + deviceId;

  configTime(0, 0, "pool.ntp.org", "time.google.com");
  secureClient.setCACert(trustAnchors.c_str());
  client.setServer(broker.c_str(), 8883);
  client.setSocketTimeout(5);
  mqttCallback = function;
  connectCallback = onConnect;
  client.setCallback(handleRequest);
  handleConnection();
}

bool Mqtt::connect(const String& id, const String& username, const String& password) {
  Serial.println("Connecting MQTT");
  if (!client.connect(id.c_str(), username.c_str(), password.c_str(), stateTopic.c_str(), 1, true, "{\"status\":\"offline\"}")) return false;
  secureClient.setNoDelay(true);
  return true;
}

bool Mqtt::isConnected() {
  return client.connected();
}

void Mqtt::poll() {
  handleConnection();
  if (isConnected()) {
    client.loop();
  }
}

void Mqtt::subscribe(const String& topic) {
  client.subscribe(topic.c_str(), 1);
}

void Mqtt::send(const JsonDocument& doc) {
  String resp;
  serializeJson(doc, resp);
  send(resp);
}

bool Mqtt::sendState(const JsonDocument& doc) {
  String state;
  serializeJson(doc, state);
  return send(stateTopic, state, true);
}

void Mqtt::send(const String& payload) {
  send(reactionTopic, payload);
}

bool Mqtt::send(const String& topic, const String& payload, bool retained) {
  if (isConnected()) {
    Serial.println("sending " + payload);
    return client.publish(topic.c_str(), payload.c_str(), retained);
  }
  return false;
}
