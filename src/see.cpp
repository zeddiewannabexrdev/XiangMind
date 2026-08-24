#include "see.h"
#include "eval.h"
#include "tables.h"

namespace search {

bool see_ge(const Position &pos, Move m, int threshold) {
  // A simplified SEE for Xiangqi: just check the first capture
  // A full SEE for Xiangqi must handle Cannon screens accurately.
  
  if (m.flags() != CAPTURE) {
      return 0 >= threshold;
  }
  
  Piece captured = pos.piece_on(m.to());
  Piece attacker = pos.piece_on(m.from());
  
  if (captured == NO_PIECE) return 0 >= threshold;
  
  int swap = eval::PIECE_VALUE[type_of(captured)] - threshold;
  if (swap < 0) return false;
  
  swap = eval::PIECE_VALUE[type_of(attacker)] - swap;
  if (swap <= 0) return true;
  
  // For now, assume true if the direct trade looks ok
  return true;
}

} // namespace search
