#include "Mqtt.h"

Mqtt::CallbackFunction mqttCallback;
Mqtt::ConnectFunction firstConnectCallback;
WiFiClientSecure secureClient;
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
  if (Http::isConnected() && !isConnected()) {
    if (connect(deviceId, username, password)) {
      Serial.println("MQTT connected");
      subscribe(home + "/" + action + "/" + deviceId);
      if (connectedBefore) {
        send(willTopic, "{\"status\":\"online\"}");
      } else {
        connectedBefore = true;
        firstConnectCallback();
      }
    } else {
      Serial.println("MQTT connection failed, rc=" + String(client.state()));
      delay(5000);
    }
  }
}

void Mqtt::init(CallbackFunction function, ConnectFunction onFirstConnect) {
  broker = Persistance::get(Persistance::MQTT_BROKER);
  username = Persistance::get(Persistance::MQTT_USER);
  password = Persistance::get(Persistance::MQTT_PASSWORD);
  deviceId = Persistance::get(Persistance::ID);
  home = "home/" + Persistance::get(Persistance::HOME_ID);
  action = "action";
  reaction = "reaction";
  willTopic = home + "/will/" + deviceId;

  secureClient.setInsecure();
  client.setServer(broker.c_str(), 8883);
  mqttCallback = function;
  firstConnectCallback = onFirstConnect;
  client.setCallback(handleRequest);
  handleConnection();
}

bool Mqtt::connect(const String& id, const String& username, const String& password) {
  Serial.println("Connecting MQTT");
  DynamicJsonDocument doc(1024);
  doc["status"] =  "offline";
  String willMessage;
  serializeJson(doc, willMessage);
  return client.connect(id.c_str(), username.c_str(), password.c_str(), willTopic.c_str(), 1, false, willMessage.c_str());
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
  client.subscribe(topic.c_str());
}

void Mqtt::send(DynamicJsonDocument doc) {
  String resp;
  serializeJson(doc, resp);
  send(resp);
}

void Mqtt::send(const String& payload) {
  send(home + "/" + reaction + "/" + deviceId, payload);
}

void Mqtt::send(const String& topic, const String& payload) {
  if(isConnected()) {
    Serial.println("sending " + payload);
    client.publish(topic.c_str(), payload.c_str());
  }
}
