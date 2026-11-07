#include "Persistence.h"

void Persistence::init() {
  uint8_t attempts = 0;
  while (!LittleFS.begin(true)) {
    Serial.println("Failed to mount file system");
    if (++attempts >= mountAttempts) {
      Serial.println("restarting...");
      delay(restartDelay);
      ESP.restart();
    }
    delay(retryDelay);
  }
  if (deserializeJson(settings, read())) {
    clearJson();
  }
  Serial.println("Persistence init");
}

String Persistence::read(const String& name) {
  File file = LittleFS.open(name, "r");
  if (!file) {
    Serial.println("Failed to open file for reading: " + name);
    return "";
  }
  String s = file.readString();
  file.close();
  return s;
}

void Persistence::write(const String& name, const String& text) {
  File file = LittleFS.open(name, "w");
  if (!file) {
    Serial.println("Failed to open file for writing: " + name);
    return;
  }
  file.println(text);
  file.close();
}

String Persistence::read() {
  return read(psFileName);
}

void Persistence::write(const String& text) {
  write(psFileName, text);
}

void Persistence::clearJson() {
  settings.clear();
  token = "";
  write("{}");
}

String Persistence::get(const String& key) {
  return settings[key].as<String>();
}

void Persistence::set(const String& key, const String& value) {
  settings[key] = value;
  if (key == ID || key == HOME_ID) token = "";
}

void Persistence::save() {
  String text;
  serializeJsonPretty(settings, text);
  write(text);
  deserializeJson(settings, text);
}

void Persistence::persist(const String& key, const String& value) {
  set(key, value);
  save();
}

String Persistence::getToken() {
  if (token.isEmpty()) token = base64::encode(get(HOME_ID) + "/" + get(ID) + "/user:keylock");
  return token;
}
