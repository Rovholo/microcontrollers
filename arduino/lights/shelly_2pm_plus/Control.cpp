#include "Control.h"

const uint8_t led = 0;
const uint8_t button = 4;
const uint8_t relays[2] = {13,12};
const uint8_t switches[2] = {5,18};

void Control::init() {
    Serial.println("control init");
    pinMode(led, OUTPUT);
    for (uint8_t i = 0; i < relayCount(); i++) {
        pinMode(relays[i], OUTPUT);
        pinMode(switches[i], INPUT);
        swichStates[i] = digitalRead(switches[i]) == HIGH;
    }
    ledOff();
}

void Control::ledOn() {
    digitalWrite(led, LOW);
}

void Control::ledOff() {
    digitalWrite(led, HIGH);
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

bool Control::isRelayOn(const uint8_t& i) {
    return digitalRead(relays[i]) == HIGH;
}

void Control::relayToggle(const uint8_t& i, bool value) {
    if(value) {
        relayOn(i);
    } else {
        relayOff(i);
    }
}

void Control::relayToggle(const uint8_t& i) {
    if(isRelayOn(i)) {
        relayOff(i);
    } else {
        relayOn(i);
    }
}

uint8_t Control::relayCount() {
    return sizeof(relays) / sizeof(relays[0]);
}

void Control::restart() {
    Serial.println("restarting...");
    ESP.restart();
}

bool Control::chechStatus(const uint8_t& i) {
    if (digitalRead(button) == HIGH) {
        delay(1000);
        restart();
    }
    return isRelayOn(i);
}

void Control::checkSwitch(const uint8_t& i) {
    bool state = digitalRead(switches[i]) == HIGH;
    if (swichStates[i] != state) {
        relayToggle(i);
        swichStates[i] = state;
    }
}
