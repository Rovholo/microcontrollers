#include "Mqtt.h"

Mqtt::CallbackFunction mqttCallback;
Mqtt::ConnectFunction connectCallback;
WiFiClientSecure secureClient;
BearSSL::Session tlsSession;
PubSubClient client(secureClient);

void handleRequest(char* topic, byte* payload, unsigned int length) {
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, payload, length);
  if (error) {
    Serial.print("json deserialize failed: ");
    Serial.println(error.c_str());
    return;
  }
  Mqtt::send(mqttCallback(doc));
}

void Mqtt::handleConnection() {
  if (isConnected()) return;
  if (retryDelay > 0 && millis() - retryTimer < retryDelay) return;
  retryTimer = millis();
  if (Http::isConnected() && connect(deviceId, username, password)) {
    Serial.println("MQTT connected");
    retryDelay = 0;
    subscribe(home + "/" + action + "/" + deviceId);
    connectCallback(!connectedBefore);
    connectedBefore = true;
  } else {
    retryDelay = retryDelay == 0 ? minRetryDelay : min(retryDelay * 2, maxRetryDelay);
    Serial.println("MQTT connection failed, rc=" + String(client.state()) + ", retrying in " + String(retryDelay / 1000) + "s");
  }
}

void Mqtt::init(CallbackFunction function, ConnectFunction onConnect) {
  broker = Persistance::get(Persistance::MQTT_BROKER);
  username = Persistance::get(Persistance::MQTT_USER);
  password = Persistance::get(Persistance::MQTT_PASSWORD);
  deviceId = Persistance::get(Persistance::ID);
  home = "home/" + Persistance::get(Persistance::HOME_ID);
  action = "action";
  reaction = "reaction";
  stateTopic = home + "/state/" + deviceId;

  secureClient.setInsecure();
  secureClient.setSession(&tlsSession);
  // Sends each small mqtt packet straight away instead of waiting to batch it with the next one.
  WiFiClient::setDefaultNoDelay(true);
  client.setServer(broker.c_str(), 8883);
  client.setSocketTimeout(5);
  mqttCallback = function;
  connectCallback = onConnect;
  client.setCallback(handleRequest);
  handleConnection();
}

bool Mqtt::connect(const String& id, const String& username, const String& password) {
  Serial.println("Connecting MQTT");
  DynamicJsonDocument doc(1024);
  doc["status"] =  "offline";
  String willMessage;
  serializeJson(doc, willMessage);
  return client.connect(id.c_str(), username.c_str(), password.c_str(), stateTopic.c_str(), 1, true, willMessage.c_str());
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

void Mqtt::send(DynamicJsonDocument doc) {
  String resp;
  serializeJson(doc, resp);
  send(resp);
}

bool Mqtt::sendState(DynamicJsonDocument doc) {
  String state;
  serializeJson(doc, state);
  return send(stateTopic, state, true);
}

void Mqtt::send(const String& payload) {
  send(home + "/" + reaction + "/" + deviceId, payload);
}

bool Mqtt::send(const String& topic, const String& payload, bool retained) {
  if(isConnected()) {
    Serial.println("sending " + payload);
    return client.publish(topic.c_str(), payload.c_str(), retained);
  }
  return false;
}
