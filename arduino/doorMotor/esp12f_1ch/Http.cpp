#include "Http.h"

ESP8266WebServer server(80);

Http::CallbackFunction callback;

const String JSON_CONTENT_TYPE="application/json";

void handleRequest() {
  if (server.method() != HTTP_POST) {
    server.send(405, JSON_CONTENT_TYPE, "{\"success\":false}");
  } else {

    if (server.hasHeader("Authorization")) {
      String authHeader = server.header("Authorization");
      if (authHeader.startsWith("Basic ")) {
        String encoded = authHeader.substring(6);
        String token = Persistance::getToken();
        if(token == encoded) {
          DynamicJsonDocument doc(1024);
          DeserializationError error = deserializeJson(doc, server.arg("plain"));
          if (error) {
            server.send(400, JSON_CONTENT_TYPE, "{\"success\":false}");
            return;
          }
          String resp;
          serializeJson(callback(doc), resp);
          server.send(200, JSON_CONTENT_TYPE, resp);
          return;
        }
      }
    }

    server.send(401, JSON_CONTENT_TYPE, "{\"success\":false}");
  }
}

bool Http::isConnected() {
  IPAddress testIP;
  return WiFi.hostByName("www.google.com", testIP);
}

void Http::init(CallbackFunction function) {
  callback = function;
  server.on("/", handleRequest);
  server.begin();
  MDNS.begin(Persistance::get(Persistance::ID));
}

void Http::poll() {
  server.handleClient();
  MDNS.update();
}

