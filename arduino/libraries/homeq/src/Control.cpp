#include "Control.h"

namespace {
const uint32_t pulseTime = 500;
const unsigned long debounceTime = 50;
const unsigned long holdTime = 5000;
}

void Control::init(const Board& b) {
  Serial.println("control init");
  board = &b;
  relayTickers = new Ticker[board->channels];
  relayStates = new bool[board->channels];
  pinMode(board->led, OUTPUT);
  if (board->button >= 0) {
    pinMode(board->button, board->buttonMode);
    buttonRaw = readButton();
    buttonDown = buttonRaw;
    buttonArmed = !buttonDown;
    buttonChanged = millis();
  }
  for (uint8_t i = 0; i < board->channels; i++) {
    pinMode(board->relays[i], OUTPUT);
    pinMode(board->inputs[i], board->inputMode);
    relaySet(i, false);
  }
  ledOff();
}

void Control::ledOn() {
  digitalWrite(board->led, LOW);
}

void Control::ledOff() {
  digitalWrite(board->led, HIGH);
}

void Control::ledBlink() {
  ledOn();
  ticker.once(0.5, ledOff);
}

void Control::relaySet(const uint8_t& i, bool on) {
  digitalWrite(board->relays[i], on ? HIGH : LOW);
  relayStates[i] = on;
}

bool Control::isRelayOn(const uint8_t& i) {
  return relayStates[i];
}

bool Control::relayPulse(const uint8_t& i) {
  if (relayTickers[i].active()) return false;
  relaySet(i, true);
  uint8_t relay = i;
  relayTickers[i].once_ms(pulseTime, [relay]() { relaySet(relay, false); });
  return true;
}

uint8_t Control::channelCount() {
  return board->channels;
}

bool Control::isInputActive(const uint8_t& i) {
  return digitalRead(board->inputs[i]) == board->inputActive;
}

void Control::restart() {
  Serial.println("\nrestarting...");
  delay(500);
  ESP.restart();
}

bool Control::readButton() {
  return digitalRead(board->button) == board->buttonPressed;
}

ButtonEvent Control::checkButton() {
  if (board == nullptr || board->button < 0) return ButtonEvent::NONE;
  bool raw = readButton();
  if (raw != buttonRaw) {
    buttonRaw = raw;
    buttonChanged = millis();
  }
  if (raw != buttonDown && millis() - buttonChanged >= debounceTime) {
    buttonDown = raw;
    if (buttonDown) {
      buttonDownAt = millis();
      holdFired = false;
      return ButtonEvent::NONE;
    }
    bool wasArmed = buttonArmed;
    buttonArmed = true;
    return wasArmed && !holdFired ? ButtonEvent::PRESS : ButtonEvent::NONE;
  }
  if (buttonDown && buttonArmed && !holdFired && millis() - buttonDownAt >= holdTime) {
    holdFired = true;
    ledOn();
    return ButtonEvent::HOLD;
  }
  return ButtonEvent::NONE;
}
