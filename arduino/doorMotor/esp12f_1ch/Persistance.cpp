#include "Persistance.h"

void Persistance::init() {
  if (!LittleFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }
  if (deserializeJson(settings, read())) {
    clearJson();
  }
}

String Persistance::read(const String& name) {
  File file = LittleFS.open(name, "r");
  if (!file) {
    Serial.println("Failed to open file for reading " + name);
    return "";
  }
  return file.readString();  // file is closed automatically when 'file' goes out of scope
}

void Persistance::write(const String& name, const String& text) {
  File file = LittleFS.open(name, "w");
  if (!file) {
    Serial.println("Failed to open file for writing " + name);
    return;
  }
  file.println(text);
  file.close();
}

String Persistance::read() {
  return read(psFileName);
}

void Persistance::write(const String& text) {
  write(psFileName, text);
}

void Persistance::clearJson() {
  settings.clear();
  token = "";
  write("{}");
}

String Persistance::get(const String& key) {
  return settings[key].as<String>();
}

void Persistance::set(const String& key, const String& value) {
  settings[key] = value;
  if (key == ID || key == HOME_ID) token = "";
}

void Persistance::save() {
  String text;
  serializeJsonPretty(settings, text);
  write(text);
  // Reparsing compacts the document, since replaced values otherwise keep their old space.
  deserializeJson(settings, text);
}

void Persistance::persist(const String& key, const String& value) {
  set(key, value);
  save();
}

String Persistance::getToken() {
  if (token.isEmpty()) token = base64::encode(get(HOME_ID) + "/" + get(ID) + "/user:keylock");
  return token;
}
