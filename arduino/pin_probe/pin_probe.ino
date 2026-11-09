#include <WiFi.h>
#include <WebServer.h>

#if CONFIG_IDF_TARGET_ESP32C3
const uint8_t pins[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 18, 19};
#else
const uint8_t pins[] = {0, 2, 4, 5, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33, 34, 35, 36, 39};
#endif
const uint8_t pinCount = sizeof(pins);
const uint32_t pulseTime = 300;

WebServer server(80);
int levels[pinCount];
unsigned long changes[pinCount];

String page() {
  String html = "<!doctype html><meta name=viewport content='width=device-width'><title>Pin probe</title>"
                "<style>body{font-family:sans-serif;margin:16px}td,th{padding:6px 10px;text-align:left}"
                ".hi{color:#0a0;font-weight:bold}button{padding:6px 12px}</style>"
                "<h3>Pin probe</h3><p>Toggle a switch or press the button and watch which pin changes. "
                "Pulse sets a pin HIGH for 300 ms, listen for the relay.</p>"
                "<table><tr><th>GPIO</th><th>Level</th><th>Changes</th><th></th></tr>";
  for (uint8_t i = 0; i < pinCount; i++) {
    html += "<tr><td>" + String(pins[i]) + "</td><td class=" + String(levels[i] ? "hi>HIGH" : "lo>LOW") +
            "</td><td>" + String(changes[i]) + "</td><td><button onclick=\"fetch('/pulse?pin=" + String(pins[i]) +
            "')\">Pulse</button></td></tr>";
  }
  html += "</table><script>setTimeout(()=>location.reload(),1000)</script>";
  return html;
}

void handlePulse() {
  int pin = server.arg("pin").toInt();
  bool known = false;
  for (uint8_t i = 0; i < pinCount; i++) {
    if (pins[i] == pin) known = true;
  }
  if (!known) {
    server.send(400, "text/plain", "unknown pin");
    return;
  }
  Serial.println("pulse GPIO " + String(pin));
  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH);
  delay(pulseTime);
  digitalWrite(pin, LOW);
  pinMode(pin, INPUT);
  server.send(200, "text/plain", "ok");
}

void setup() {
  Serial.begin(115200);
  for (uint8_t i = 0; i < pinCount; i++) {
    pinMode(pins[i], INPUT);
    levels[i] = digitalRead(pins[i]);
  }
  WiFi.mode(WIFI_AP);
  WiFi.softAP("pin-probe", "probe1234");
  server.on("/", [] { server.send(200, "text/html", page()); });
  server.on("/pulse", handlePulse);
  server.begin();
  Serial.println("join wifi pin-probe (password probe1234) and open http://192.168.4.1");
}

void loop() {
  server.handleClient();
  for (uint8_t i = 0; i < pinCount; i++) {
    int level = digitalRead(pins[i]);
    if (level != levels[i]) {
      levels[i] = level;
      changes[i]++;
      Serial.println("GPIO " + String(pins[i]) + " -> " + (level ? "HIGH" : "LOW"));
    }
  }
  delay(5);
}
