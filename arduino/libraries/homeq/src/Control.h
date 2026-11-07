#ifndef CONTROL_H
#define CONTROL_H

#include "Board.h"
#include <Arduino.h>
#include <Ticker.h>

enum class ButtonEvent : uint8_t {
  NONE,
  PRESS,
  HOLD,
};

class Control {
private:
  inline static const Board* board = nullptr;
  inline static Ticker ticker;
  inline static Ticker* relayTickers = nullptr;
  inline static bool* relayStates = nullptr;
  inline static bool buttonRaw = false;
  inline static bool buttonDown = false;
  inline static bool buttonArmed = false;
  inline static bool holdFired = false;
  inline static unsigned long buttonChanged = 0;
  inline static unsigned long buttonDownAt = 0;
  static bool readButton();
public:
  static void init(const Board& b);
  static void ledOn();
  static void ledOff();
  static void ledBlink();
  static void relaySet(const uint8_t& i, bool on);
  static bool isRelayOn(const uint8_t& i);
  static bool relayPulse(const uint8_t& i);
  static uint8_t channelCount();
  static bool isInputActive(const uint8_t& i);
  static void restart();
  static ButtonEvent checkButton();
};

#endif
