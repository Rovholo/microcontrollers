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
  if (read().isEmpty()) {
    clearJson();
  };
  Serial.println("Persistence init");
}

String Persistence::read(const String& name) {
  File file = LittleFS.open(name, "r");
  if (!file) {
    if (!LittleFS.exists(name)) {
        Serial.println("Error: File does not actually exist");
    }
    Serial.println("Failed to open file for reading: " + name);
    return "";
  }
  String s = file.readString();  // file is closed automatically when 'file' goes out of scope
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
  write("{}");
}

DynamicJsonDocument Persistence::getJson() {
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc,read());
  if (error) {
    doc.clear();
  }
  return doc;
}

String Persistence::get(const String& key) {
  return getJson()[key].as<String>();
}

void Persistence::persist(const String& key, const String& value) {
  DynamicJsonDocument doc = getJson();
  doc[key] = value;
  String prettyOutput;
  serializeJsonPretty(doc, prettyOutput);
  write(prettyOutput);
}

String Persistence::getToken() {
  Serial.print(get(HOME_ID) + "/" + get(ID) + "/user:keylock");
  return base64::encode(get(HOME_ID) + "/" + get(ID) + "/user:keylock");
}
