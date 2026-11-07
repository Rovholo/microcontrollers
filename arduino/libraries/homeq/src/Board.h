#ifndef BOARD_H
#define BOARD_H

#include <Arduino.h>

enum class DeviceType : uint8_t {
  GARAGE_DOOR,
  LIGHT,
  SENSOR,
};

struct Board {
  uint8_t led;
  int8_t button;
  uint8_t buttonMode;
  uint8_t buttonPressed;
  const uint8_t* relays;
  const uint8_t* inputs;
  uint8_t channels;
  uint8_t inputMode;
  uint8_t inputActive;
  DeviceType defaultType;
};

#endif
