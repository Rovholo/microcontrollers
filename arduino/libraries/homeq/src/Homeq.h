#ifndef HOMEQ_H
#define HOMEQ_H

#include "Board.h"

class Homeq {
public:
  static void begin(const Board& board);
  static void loop();
};

#endif
