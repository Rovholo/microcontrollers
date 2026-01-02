#include "Persistance.h"

void Persistance::init() {
  if (!LittleFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }
  if (read().isEmpty()) {
    clearJson();
  };
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
  write("{}");
}

DynamicJsonDocument Persistance::getJson() {
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc,read());
  if (error) {
    doc.clear();
  }
  return doc;
}

String Persistance::get(const String& key) {
  return getJson()[key].as<String>();
}

void Persistance::persist(const String& key, const String& value) {
  DynamicJsonDocument doc = getJson();
  doc[key] = value;
  String prettyOutput;
  serializeJsonPretty(doc, prettyOutput);
  write(prettyOutput);
}

String Persistance::getToken() {
  return base64::encode(get(HOME_ID) + "/" + get(ID) + "/user:keylock");
}
