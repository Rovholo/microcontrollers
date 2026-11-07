#include <Homeq.h>

const uint8_t relays[] = {13, 12};
const uint8_t inputs[] = {5, 18};

const Board board = {
  .led = 0,
  .button = 4,
  .buttonMode = INPUT_PULLUP,
  .buttonPressed = LOW,
  .relays = relays,
  .inputs = inputs,
  .channels = 2,
  .inputMode = INPUT,
  .inputActive = HIGH,
  .defaultType = DeviceType::LIGHT,
};

void setup() {
  Homeq::begin(board);
}

void loop() {
  Homeq::loop();
}
