#include "Http.h"
#ifdef ESP8266
#include <bearssl/bearssl_hmac.h>
#else
#include <mbedtls/md.h>
#include <esp_random.h>
#endif

namespace {
#ifdef ESP8266
ESP8266WebServer server(80);
#else
WebServer server(80);
#endif

Http::CallbackFunction callback;
bool signedOnly = false;

const String JSON_CONTENT_TYPE = "application/json";
const uint8_t nonceCount = 8;
const unsigned long nonceTime = 10 * 1000;
String nonces[nonceCount];
unsigned long nonceTimers[nonceCount];
uint8_t nonceIndex = 0;
const unsigned long secretWindow = 10 * 60 * 1000;
bool secretWindowClosed = false;
unsigned long secretWindowStart = 0;
bool serverStarted = false;
bool mdnsStarted = false;
String about;

String toHex(const uint8_t* bytes, size_t length) {
  const char* digits = "0123456789abcdef";
  String hex;
  hex.reserve(length * 2);
  for (size_t i = 0; i < length; i++) {
    hex += digits[bytes[i] >> 4];
    hex += digits[bytes[i] & 0x0f];
  }
  return hex;
}

String sign(const String& secret, const String& message) {
  uint8_t out[32];
#ifdef ESP8266
  br_hmac_key_context key;
  br_hmac_key_init(&key, &br_sha256_vtable, secret.c_str(), secret.length());
  br_hmac_context hmac;
  br_hmac_init(&hmac, &key, 0);
  br_hmac_update(&hmac, message.c_str(), message.length());
  br_hmac_out(&hmac, out);
#else
  mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
                  (const unsigned char*)secret.c_str(), secret.length(),
                  (const unsigned char*)message.c_str(), message.length(), out);
#endif
  return toHex(out, sizeof(out));
}

void fillRandom(uint8_t* bytes, size_t length) {
#ifdef ESP8266
  ESP.random(bytes, length);
#else
  esp_fill_random(bytes, length);
#endif
}

bool sameText(const String& a, const String& b) {
  if (a.length() != b.length()) return false;
  uint8_t diff = 0;
  for (size_t i = 0; i < a.length(); i++) {
    diff |= a[i] ^ b[i];
  }
  return diff == 0;
}

bool useNonce(const String& nonce) {
  for (uint8_t i = 0; i < nonceCount; i++) {
    if (!nonces[i].isEmpty() && nonces[i] == nonce) {
      nonces[i] = "";
      return millis() - nonceTimers[i] < nonceTime;
    }
  }
  return false;
}

bool isSecret(const String& secret) {
  if (secret.length() != 64) return false;
  for (size_t i = 0; i < secret.length(); i++) {
    if (!isDigit(secret[i]) && (secret[i] < 'a' || secret[i] > 'f')) return false;
  }
  return true;
}

bool isAuthorized(const String& body) {
  if (!server.hasHeader("Authorization")) return false;
  String auth = server.header("Authorization");
  if (signedOnly) {
    int split = auth.indexOf(':');
    if (!auth.startsWith("HMAC ") || split < 0) return false;
    String nonce = auth.substring(5, split);
    String secret = Persistence::get(Persistence::SECRET);
    return isSecret(secret) && useNonce(nonce) && sameText(sign(secret, nonce + body), auth.substring(split + 1));
  }
  return auth.startsWith("Basic ") && auth.substring(6) == Persistence::getToken();
}

String newNonce() {
  uint8_t bytes[16];
  fillRandom(bytes, sizeof(bytes));
  String nonce = toHex(bytes, sizeof(bytes));
  nonces[nonceIndex] = nonce;
  nonceTimers[nonceIndex] = millis();
  nonceIndex = (nonceIndex + 1) % nonceCount;
  return nonce;
}

void handleNonce() {
  if (!signedOnly) {
    server.send(404, JSON_CONTENT_TYPE, "{\"success\":false}");
    return;
  }
  server.send(200, JSON_CONTENT_TYPE, "{\"nonce\":\"" + newNonce() + "\"}");
}

void handleAbout() {
  if (signedOnly || about.isEmpty()) {
    server.send(404, JSON_CONTENT_TYPE, "{\"success\":false}");
    return;
  }
  server.send(200, JSON_CONTENT_TYPE, about);
}

void handleSecret() {
  if (!signedOnly) {
    server.send(404, JSON_CONTENT_TYPE, "{\"success\":false}");
    return;
  }
  if (!server.hasHeader("Authorization") || server.header("Authorization") != "Basic " + Persistence::getToken()) {
    server.send(401, JSON_CONTENT_TYPE, "{\"success\":false}");
    return;
  }
  if (isSecret(Persistence::get(Persistence::SECRET))) {
    server.send(409, JSON_CONTENT_TYPE, "{\"success\":false}");
    return;
  }
  if (secretWindowClosed) {
    server.send(403, JSON_CONTENT_TYPE, "{\"success\":false}");
    return;
  }
  StaticJsonDocument<256> doc;
  String secret = deserializeJson(doc, server.arg("plain")) ? "" : doc["secret"] | "";
  if (!isSecret(secret)) {
    server.send(400, JSON_CONTENT_TYPE, "{\"success\":false}");
    return;
  }
  Persistence::persist(Persistence::SECRET, secret);
  server.send(200, JSON_CONTENT_TYPE, "{\"success\":true}");
}

void handleRequest() {
  if (server.method() != HTTP_POST) {
    server.send(405, JSON_CONTENT_TYPE, "{\"success\":false}");
    return;
  }
  String body = server.arg("plain");
  if (!isAuthorized(body)) {
    server.send(401, JSON_CONTENT_TYPE, "{\"success\":false}");
    return;
  }
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, body);
  if (error) {
    server.send(400, JSON_CONTENT_TYPE, "{\"success\":false}");
    return;
  }
  callback(doc);
  if (signedOnly) doc["nonce"] = newNonce();
  String resp;
  serializeJson(doc, resp);
  server.send(200, JSON_CONTENT_TYPE, resp);
}
}

bool Http::isConnected() {
  IPAddress testIP;
#ifdef ESP8266
  return WiFi.hostByName("www.google.com", testIP, 2000);
#else
  return WiFi.hostByName("www.google.com", testIP);
#endif
}

void Http::init(CallbackFunction function, bool requireSignature) {
  callback = function;
  signedOnly = requireSignature;
  secretWindowStart = millis();
  secretWindowClosed = false;
  if (!serverStarted) {
    server.on("/nonce", HTTP_GET, handleNonce);
    server.on("/about", HTTP_GET, handleAbout);
    server.on("/secret", HTTP_POST, handleSecret);
    server.on("/", handleRequest);
    server.begin();
    serverStarted = true;
  }
  if (signedOnly && !mdnsStarted) mdnsStarted = MDNS.begin(Persistence::get(Persistence::ID).c_str());
}

void Http::setAbout(const String& json) {
  about = json;
}

void Http::stopMdns() {
  if (!mdnsStarted) return;
  MDNS.end();
  mdnsStarted = false;
}

void Http::poll() {
  if (!secretWindowClosed && millis() - secretWindowStart > secretWindow) secretWindowClosed = true;
  server.handleClient();
#ifdef ESP8266
  if (mdnsStarted) MDNS.update();
#endif
}
