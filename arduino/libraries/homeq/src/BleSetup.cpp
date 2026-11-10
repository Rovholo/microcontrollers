#include "BleSetup.h"

#if defined(ESP32) && defined(CONFIG_BT_ENABLED)

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <mbedtls/ecdh.h>
#include <mbedtls/gcm.h>
#include <mbedtls/md.h>
#include <mbedtls/base64.h>
#include <esp_random.h>

namespace {
const char* serviceUuid = "6b9f0001-3c5e-4f8a-9d2b-7e1a4c6d8f00";
const char* aboutUuid = "6b9f0002-3c5e-4f8a-9d2b-7e1a4c6d8f00";
const char* requestUuid = "6b9f0003-3c5e-4f8a-9d2b-7e1a4c6d8f00";
const char* replyUuid = "6b9f0004-3c5e-4f8a-9d2b-7e1a4c6d8f00";
const char* keyLabel = "homeq-ble-v1";
const uint8_t firstChunk = 0x01;
const uint8_t lastChunk = 0x02;
const size_t chunkSize = 19;
const size_t maxRequest = 2048;
const size_t keySize = 32;
const size_t nonceSize = 12;
const size_t tagSize = 16;

bool started = false;
BleSetup::CallbackFunction callback = nullptr;
BLEServer* server = nullptr;
BLECharacteristic* replyCharacteristic = nullptr;
mbedtls_ecp_group group;
mbedtls_mpi privateKey;
mbedtls_ecp_point publicPoint;
uint8_t publicKey[keySize];
String incoming;
String request;
volatile bool requestReady = false;
volatile bool restartAdvertising = false;

int randomBytes(void*, unsigned char* out, size_t length) {
  esp_fill_random(out, length);
  return 0;
}

String toBase64(const uint8_t* data, size_t length) {
  size_t needed = 0;
  mbedtls_base64_encode(nullptr, 0, &needed, data, length);
  char* text = new char[needed + 1];
  size_t written = 0;
  mbedtls_base64_encode((unsigned char*)text, needed, &written, data, length);
  text[written] = 0;
  String result(text);
  delete[] text;
  return result;
}

bool fromBase64(const String& text, uint8_t* out, size_t capacity, size_t& length) {
  return mbedtls_base64_decode(out, capacity, &length, (const unsigned char*)text.c_str(), text.length()) == 0;
}

bool deriveKey(const uint8_t* peerKey, uint8_t* key) {
  mbedtls_ecp_point peer;
  mbedtls_mpi shared;
  mbedtls_ecp_point_init(&peer);
  mbedtls_mpi_init(&shared);
  uint8_t secret[keySize];
  bool ok = mbedtls_ecp_point_read_binary(&group, &peer, peerKey, keySize) == 0 &&
            mbedtls_ecdh_compute_shared(&group, &shared, &peer, &privateKey, randomBytes, nullptr) == 0 &&
            mbedtls_mpi_write_binary_le(&shared, secret, keySize) == 0;
  mbedtls_ecp_point_free(&peer);
  mbedtls_mpi_free(&shared);
  if (!ok) return false;
  const mbedtls_md_info_t* sha = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  mbedtls_md_context_t md;
  mbedtls_md_init(&md);
  ok = mbedtls_md_setup(&md, sha, 0) == 0 && mbedtls_md_starts(&md) == 0 &&
       mbedtls_md_update(&md, (const unsigned char*)keyLabel, strlen(keyLabel)) == 0 &&
       mbedtls_md_update(&md, secret, keySize) == 0 && mbedtls_md_finish(&md, key) == 0;
  mbedtls_md_free(&md);
  memset(secret, 0, sizeof(secret));
  return ok;
}

bool crypt(bool encrypt, const uint8_t* key, const uint8_t* nonce, const uint8_t* input, size_t length,
           uint8_t* output, uint8_t* tag) {
  mbedtls_gcm_context gcm;
  mbedtls_gcm_init(&gcm);
  bool ok = mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, keySize * 8) == 0;
  if (ok && encrypt) {
    ok = mbedtls_gcm_crypt_and_tag(&gcm, MBEDTLS_GCM_ENCRYPT, length, nonce, nonceSize, nullptr, 0, input, output,
                                   tagSize, tag) == 0;
  } else if (ok) {
    ok = mbedtls_gcm_auth_decrypt(&gcm, length, nonce, nonceSize, nullptr, 0, tag, tagSize, input, output) == 0;
  }
  mbedtls_gcm_free(&gcm);
  return ok;
}

void sendReply(const String& reply) {
  if (replyCharacteristic == nullptr) return;
  size_t length = reply.length();
  size_t offset = 0;
  do {
    size_t part = length - offset < chunkSize ? length - offset : chunkSize;
    uint8_t frame[chunkSize + 1];
    frame[0] = (offset == 0 ? firstChunk : 0) | (offset + part >= length ? lastChunk : 0);
    memcpy(frame + 1, reply.c_str() + offset, part);
    replyCharacteristic->setValue(frame, part + 1);
    replyCharacteristic->notify();
    offset += part;
    delay(15);
  } while (offset < length);
}

String failure(const char* reason) {
  Serial.println(String("ble setup: ") + reason);
  return String("{\"error\":\"") + reason + "\"}";
}

String handle(const String& text) {
  StaticJsonDocument<maxRequest + 256> envelope;
  if (deserializeJson(envelope, text)) return failure("bad request");
  uint8_t peerKey[keySize + 4];
  uint8_t nonce[nonceSize + 4];
  size_t peerLength = 0;
  size_t nonceLength = 0;
  if (!fromBase64(envelope["key"] | "", peerKey, sizeof(peerKey), peerLength) || peerLength != keySize ||
      !fromBase64(envelope["nonce"] | "", nonce, sizeof(nonce), nonceLength) || nonceLength != nonceSize) {
    return failure("bad key");
  }
  String data = envelope["data"] | "";
  size_t capacity = data.length();
  uint8_t* sealed = new uint8_t[capacity];
  size_t sealedLength = 0;
  if (!fromBase64(data, sealed, capacity, sealedLength) || sealedLength <= tagSize) {
    delete[] sealed;
    return failure("bad data");
  }
  uint8_t key[keySize];
  size_t plainLength = sealedLength - tagSize;
  uint8_t* plain = new uint8_t[plainLength + 1];
  bool opened = deriveKey(peerKey, key) &&
                crypt(false, key, nonce, sealed, plainLength, plain, sealed + plainLength);
  delete[] sealed;
  if (!opened) {
    delete[] plain;
    memset(key, 0, sizeof(key));
    return failure("cannot decrypt");
  }
  plain[plainLength] = 0;
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, (const char*)plain, plainLength);
  memset(plain, 0, plainLength);
  delete[] plain;
  if (error) {
    memset(key, 0, sizeof(key));
    return failure("bad setup");
  }
  callback(doc);
  String reply;
  serializeJson(doc, reply);
  uint8_t replyNonce[nonceSize];
  esp_fill_random(replyNonce, nonceSize);
  uint8_t* out = new uint8_t[reply.length() + tagSize];
  bool sealedReply = crypt(true, key, replyNonce, (const uint8_t*)reply.c_str(), reply.length(), out,
                           out + reply.length());
  memset(key, 0, sizeof(key));
  String result = sealedReply
      ? String("{\"nonce\":\"") + toBase64(replyNonce, nonceSize) + "\",\"data\":\"" +
        toBase64(out, reply.length() + tagSize) + "\"}"
      : failure("cannot encrypt");
  delete[] out;
  return result;
}

class RequestCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* characteristic) override {
    if (requestReady) return;
    String value = characteristic->getValue();
    if (value.length() == 0) return;
    uint8_t flags = (uint8_t)value[0];
    if (flags & firstChunk) incoming = "";
    if (incoming.length() + value.length() > maxRequest) {
      incoming = "";
      return;
    }
    incoming += value.substring(1);
    if (flags & lastChunk) {
      request = incoming;
      incoming = "";
      requestReady = true;
    }
  }
};

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer*) override {
    Serial.println("ble setup: phone connected");
  }

  void onDisconnect(BLEServer*) override {
    incoming = "";
    restartAdvertising = true;
  }
};
}

bool BleSetup::available() {
  return true;
}

void BleSetup::begin(const String& name, const String& about, CallbackFunction function) {
  if (started) return;
  callback = function;
  mbedtls_ecp_group_init(&group);
  mbedtls_mpi_init(&privateKey);
  mbedtls_ecp_point_init(&publicPoint);
  size_t written = 0;
  if (mbedtls_ecp_group_load(&group, MBEDTLS_ECP_DP_CURVE25519) != 0 ||
      mbedtls_ecdh_gen_public(&group, &privateKey, &publicPoint, randomBytes, nullptr) != 0 ||
      mbedtls_ecp_point_write_binary(&group, &publicPoint, MBEDTLS_ECP_PF_UNCOMPRESSED, &written, publicKey,
                                     keySize) != 0 ||
      written != keySize) {
    Serial.println("ble setup: could not create a key");
    return;
  }

  StaticJsonDocument<256> info;
  deserializeJson(info, about);
  info["key"] = toBase64(publicKey, keySize);
  String aboutJson;
  serializeJson(info, aboutJson);

  BLEDevice::init(name.c_str());
  server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());
  BLEService* service = server->createService(serviceUuid);
  BLECharacteristic* aboutCharacteristic = service->createCharacteristic(aboutUuid, BLECharacteristic::PROPERTY_READ);
  aboutCharacteristic->setValue(aboutJson);
  BLECharacteristic* requestCharacteristic = service->createCharacteristic(
      requestUuid, BLECharacteristic::PROPERTY_WRITE);
  requestCharacteristic->setCallbacks(new RequestCallbacks());
  replyCharacteristic = service->createCharacteristic(replyUuid, BLECharacteristic::PROPERTY_NOTIFY);
  replyCharacteristic->addDescriptor(new BLE2902());
  service->start();
  BLEAdvertising* advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(serviceUuid);
  advertising->setScanResponse(true);
  BLEDevice::startAdvertising();
  started = true;
  Serial.println("ble setup: advertising as " + name);
}

void BleSetup::poll() {
  if (!started) return;
  if (restartAdvertising) {
    restartAdvertising = false;
    BLEDevice::startAdvertising();
  }
  if (!requestReady) return;
  String text = request;
  request = "";
  sendReply(handle(text));
  requestReady = false;
}

void BleSetup::end() {
  if (!started) return;
  started = false;
  BLEDevice::deinit(true);
  server = nullptr;
  replyCharacteristic = nullptr;
  mbedtls_mpi_free(&privateKey);
  mbedtls_ecp_point_free(&publicPoint);
  mbedtls_ecp_group_free(&group);
  Serial.println("ble setup: bluetooth off");
}

#else

bool BleSetup::available() {
  return false;
}

void BleSetup::begin(const String&, const String&, CallbackFunction) {}

void BleSetup::poll() {}

void BleSetup::end() {}

#endif
