#include "Control.h"

const uint8_t Led = 0;
const uint8_t button = 4;
const uint8_t relays[1] = {5};
const uint8_t reedSwitches[1] = {6};

void Control::init() {
    Serial.println("control init");
    pinMode(Led, OUTPUT);
    for (uint8_t i = 0; i < relayCount(); i++) {
        pinMode(relays[i], OUTPUT);
        pinMode(reedSwitches[i], INPUT_PULLUP);
        pinMode(button, INPUT_PULLUP);
    }
    ledOff();
}

void Control::ledOn() {
    digitalWrite(Led, LOW);
}

void Control::ledOff() {
    digitalWrite(Led, HIGH);
}

void Control::ledBlink() {
    ledOn();
    ticker.once(0.5, ledOff);
}

void Control::relayOn(const uint8_t& i) {
    digitalWrite(relays[i], HIGH);
}

void Control::relayOff(const uint8_t& i) {
    digitalWrite(relays[i], LOW);
}
void Control::relayPulse(const uint8_t& i) {
    relayOn(i);
    delay(500);
    relayOff(i);
}

uint8_t Control::relayCount() {
    return sizeof(relays) / sizeof(relays[0]);
}

bool Control::isClosed(const uint8_t& i) {
    return digitalRead(reedSwitches[i]) == LOW;
}

void Control::restart() {
    Serial.println("\nrestarting...");
    delay(500);
    ESP.restart();
}

void Control::chechButton() {
    if (digitalRead(button) == HIGH) {
        Serial.println("reset button clicked...");
        restart();
    }
}
