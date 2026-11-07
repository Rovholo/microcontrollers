#include <Homeq.h>

const uint8_t relays[] = {5};
const uint8_t inputs[] = {10};

const Board board = {
  .led = 0,
  .button = 4,
  .buttonMode = INPUT_PULLUP,
  .buttonPressed = LOW,
  .relays = relays,
  .inputs = inputs,
  .channels = 1,
  .inputMode = INPUT,
  .inputActive = HIGH,
  .defaultType = DeviceType::GARAGE_DOOR,
};

void setup() {
  Homeq::begin(board);
}

void loop() {
  Homeq::loop();
}
