#include <Homeq.h>

const uint8_t relays[] = {14};
const uint8_t inputs[] = {4};

const Board board = {
  .led = 5,
  .button = -1,
  .buttonMode = INPUT,
  .buttonPressed = LOW,
  .relays = relays,
  .inputs = inputs,
  .channels = 1,
  .inputMode = INPUT_PULLUP,
  .inputActive = LOW,
  .defaultType = DeviceType::GARAGE_DOOR,
};

void setup() {
  Homeq::begin(board);
}

void loop() {
  Homeq::loop();
}
