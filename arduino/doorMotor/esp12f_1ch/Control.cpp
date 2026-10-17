#include "Control.h"

const uint8_t LED_PIN = 5;
const uint8_t relayPins[1] = {14};
const uint8_t reedSwitchesPins[1] = {4};
const uint32_t pulseTime = 500;
Ticker relayTickers[sizeof(relayPins) / sizeof(relayPins[0])];

void Control::init() {
    pinMode(LED_PIN, OUTPUT);
    for (uint8_t i = 0; i < relayCount(); i++) {
        pinMode(relayPins[i], OUTPUT);
        pinMode(reedSwitchesPins[i], INPUT_PULLUP);
    }
    ledOff();
}

void Control::ledOn() {
    digitalWrite(LED_PIN, LOW);
}

void Control::ledOff() {
    digitalWrite(LED_PIN, HIGH);
}

void Control::ledBlink() {
    ledOn();
    ticker.once(0.5, ledOff);
}

void Control::relayOn(const uint8_t& i) {
    digitalWrite(relayPins[i], HIGH);
}

void Control::relayOff(const uint8_t& i) {
    digitalWrite(relayPins[i], LOW);
}
bool Control::relayPulse(const uint8_t& i) {
    if (relayTickers[i].active()) return false;
    relayOn(i);
    relayTickers[i].once_ms(pulseTime, [i]() { relayOff(i); });
    return true;
}

uint8_t Control::relayCount() {
    return sizeof(relayPins) / sizeof(relayPins[0]);
}

bool Control::isClosed(const uint8_t& i) {
    return digitalRead(reedSwitchesPins[i]) == LOW;
}

void Control::restart() {
    ESP.restart();
}
