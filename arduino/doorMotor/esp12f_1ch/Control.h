#ifndef CONTROL_H
#define CONTROL_H

#include <Arduino.h>
#include <Ticker.h>

class Control {
private:
  inline static Ticker ticker;
public:
  static void init();
  static void ledOn();
  static void ledOff();
  static void ledBlink();
  static void relayOn(const uint8_t& i);
  static void relayOff(const uint8_t& i);
  static void relayPulse(const uint8_t& i);
  static uint8_t relayCount();
  static bool isClosed(const uint8_t& i);
  static void restart();
};

#endif
