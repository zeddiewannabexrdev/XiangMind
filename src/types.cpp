#include "types.h"

const char *SQSTR[91] = {
  "a0", "b0", "c0", "d0", "e0", "f0", "g0", "h0", "i0",
  "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1", "i1",
  "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2", "i2",
  "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3", "i3",
  "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4", "i4",
  "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5", "i5",
  "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6", "i6",
  "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7", "i7",
  "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8", "i8",
  "a9", "b9", "c9", "d9", "e9", "f9", "g9", "h9", "i9",
  "None"
};

void print_bitboard(Bitboard b) {
  for (int r = 9; r >= 0; --r) {
    for (int f = 0; f < 9; ++f) {
      int sq = r * 9 + f;
      bool set = sq < 64 ? (b.lo & (1ULL << sq)) : (b.hi & (1ULL << (sq - 64)));
      std::cout << (set ? "1 " : ". ");
    }
    std::cout << "\n";
  }
}

std::ostream &operator<<(std::ostream &os, const Move &m) {
  os << SQSTR[m.from()] << SQSTR[m.to()];
  return os;
}
