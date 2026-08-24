#pragma once
#ifdef _MSC_VER
  #pragma warning(disable : 26812) // intentional unscoped enums
#endif

#include <array>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <string>
#include <vector>

constexpr size_t NCOLORS = 2;
enum Color : int { WHITE, BLACK };
constexpr Color operator~(Color c) { return Color(c ^ BLACK); }

constexpr size_t NDIRS = 8;
enum Direction : int {
  NORTH       = 9,
  NORTH_EAST  = 10,
  EAST        = 1,
  SOUTH_EAST  = -8,
  SOUTH       = -9,
  SOUTH_WEST  = -10,
  WEST        = -1,
  NORTH_WEST  = 8,
  NORTH_NORTH = 18,
  SOUTH_SOUTH = -18,
  // Horse jumps
  NNE = 19, NNW = 17, NEE = 11, NWW = 7,
  SSE = -17, SSW = -19, SEE = -7, SWW = -11
};

constexpr size_t NPIECE_TYPES = 7;
enum PieceType : int { SOLDIER, ADVISOR, ELEPHANT, HORSE, CANNON, CHARIOT, GENERAL };

const std::string PIECE_STR = "PAEHCRG~paehcrg.";
const std::string DEFAULT_FEN = "rheakaehr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RHEAKAEHR w - - 0 1";

constexpr size_t NPIECES = 15;
enum Piece : int {
  WHITE_SOLDIER, WHITE_ADVISOR, WHITE_ELEPHANT, WHITE_HORSE, WHITE_CANNON, WHITE_CHARIOT, WHITE_GENERAL,
  BLACK_SOLDIER = 8, BLACK_ADVISOR, BLACK_ELEPHANT, BLACK_HORSE, BLACK_CANNON, BLACK_CHARIOT, BLACK_GENERAL,
  NO_PIECE = 15
};

constexpr Piece make_piece(Color c, PieceType pt) { return Piece((c << 3) + pt); }
constexpr PieceType type_of(Piece pc) { return PieceType(pc & 0b111); }
constexpr Color color_of(Piece pc) { return Color((pc & 0b1000) >> 3); }

struct Bitboard {
  uint64_t lo = 0;
  uint64_t hi = 0;

  constexpr Bitboard() = default;
  constexpr Bitboard(uint64_t l, uint64_t h) : lo(l), hi(h) {}
  constexpr explicit operator bool() const { return lo | hi; }

  constexpr Bitboard operator&(Bitboard o) const { return {lo & o.lo, hi & o.hi}; }
  constexpr Bitboard operator|(Bitboard o) const { return {lo | o.lo, hi | o.hi}; }
  constexpr Bitboard operator^(Bitboard o) const { return {lo ^ o.lo, hi ^ o.hi}; }
  constexpr Bitboard operator~() const { return {~lo, ~hi & 0x3FFFFFFULL}; } // mask 26 bits

  constexpr Bitboard& operator&=(Bitboard o) { lo &= o.lo; hi &= o.hi; return *this; }
  constexpr Bitboard& operator|=(Bitboard o) { lo |= o.lo; hi |= o.hi; return *this; }
  constexpr Bitboard& operator^=(Bitboard o) { lo ^= o.lo; hi ^= o.hi; return *this; }

  constexpr bool operator==(Bitboard o) const { return lo == o.lo && hi == o.hi; }
  constexpr bool operator!=(Bitboard o) const { return !(*this == o); }

  constexpr Bitboard operator<<(int n) const {
    if (n == 0) return *this;
    if (n >= 64) return {0, lo << (n - 64)};
    return {lo << n, (hi << n) | (lo >> (64 - n))};
  }
  constexpr Bitboard operator>>(int n) const {
    if (n == 0) return *this;
    if (n >= 64) return {hi >> (n - 64), 0};
    return {(lo >> n) | (hi << (64 - n)), hi >> n};
  }
};

constexpr int NSQUARES = 90;
constexpr int NFILES = 9;
constexpr int NRANKS = 10;

enum Square : int {
  a0, b0, c0, d0, e0, f0, g0, h0, i0,
  a1, b1, c1, d1, e1, f1, g1, h1, i1,
  a2, b2, c2, d2, e2, f2, g2, h2, i2,
  a3, b3, c3, d3, e3, f3, g3, h3, i3,
  a4, b4, c4, d4, e4, f4, g4, h4, i4,
  a5, b5, c5, d5, e5, f5, g5, h5, i5,
  a6, b6, c6, d6, e6, f6, g6, h6, i6,
  a7, b7, c7, d7, e7, f7, g7, h7, i7,
  a8, b8, c8, d8, e8, f8, g8, h8, i8,
  a9, b9, c9, d9, e9, f9, g9, h9, i9,
  NO_SQUARE = 90
};

inline Square   &operator++(Square &s) { return s = Square(int(s) + 1); }
constexpr Square operator+(Square s, Direction d) { return Square(int(s) + int(d)); }
constexpr Square operator-(Square s, Direction d) { return Square(int(s) - int(d)); }
inline Square   &operator+=(Square &s, Direction d) { return s = s + d; }
inline Square   &operator-=(Square &s, Direction d) { return s = s - d; }

enum File : int { AFILE, BFILE, CFILE, DFILE, EFILE, FFILE, GFILE, HFILE, IFILE };
enum Rank : int { RANK0, RANK1, RANK2, RANK3, RANK4, RANK5, RANK6, RANK7, RANK8, RANK9 };

extern const char *SQSTR[91];

constexpr Bitboard sq_bb(int sq) {
    return sq < 64 ? Bitboard{1ULL << sq, 0} : Bitboard{0, 1ULL << (sq - 64)};
}

inline constexpr auto MASK_FILE = [] {
  std::array<Bitboard, 9> t{};
  for (int f = 0; f < 9; ++f) {
    Bitboard b{0, 0};
    for (int r = 0; r < 10; ++r) {
      b |= sq_bb(r * 9 + f);
    }
    t[f] = b;
  }
  return t;
}();

inline constexpr auto MASK_RANK = [] {
  std::array<Bitboard, 10> t{};
  for (int r = 0; r < 10; ++r) {
    Bitboard b{0, 0};
    for (int f = 0; f < 9; ++f) {
      b |= sq_bb(r * 9 + f);
    }
    t[r] = b;
  }
  return t;
}();

inline constexpr auto SQUARE_BB = [] {
  std::array<Bitboard, 91> t{};
  for (int i = 0; i < 90; ++i)
    t[i] = sq_bb(i);
  t[90] = Bitboard{0, 0};
  return t;
}();

constexpr Bitboard sq_bb(Square s) { return SQUARE_BB[s]; }

extern void print_bitboard(Bitboard b);

#include "simd.h"

constexpr Rank   rank_of(Square s) { return Rank(s / 9); }
constexpr File   file_of(Square s) { return File(s % 9); }
constexpr Square create_square(File f, Rank r) { return Square(r * 9 + f); }

template<Direction D>
constexpr Bitboard shift(Bitboard b) {
  return D == NORTH           ? b << 9
         : D == SOUTH         ? b >> 9
         : D == NORTH_NORTH   ? b << 18
         : D == SOUTH_SOUTH   ? b >> 18
         : D == EAST          ? (b & ~MASK_FILE[IFILE]) << 1
         : D == WEST          ? (b & ~MASK_FILE[AFILE]) >> 1
         : D == NORTH_EAST    ? (b & ~MASK_FILE[IFILE]) << 10
         : D == NORTH_WEST    ? (b & ~MASK_FILE[AFILE]) << 8
         : D == SOUTH_EAST    ? (b & ~MASK_FILE[IFILE]) >> 8
         : D == SOUTH_WEST    ? (b & ~MASK_FILE[AFILE]) >> 10
                              : Bitboard{0, 0};
}

template<Color C>
constexpr Rank relative_rank(Rank r) {
  return C == WHITE ? r : Rank(RANK9 - r);
}

template<Color C>
constexpr Direction relative_dir(Direction d) {
  return Direction(C == WHITE ? d : -d);
}

enum MoveFlags : int {
  QUIET = 0b00,
  CAPTURE = 0b01
};

class Move {
private:
  uint16_t move;

public:
  inline Move() : move(0) {}
  inline Move(uint16_t m) { move = m; }
  inline Move(Square from, Square to) : move(0) { move = static_cast<uint16_t>((from << 7) | to); }
  inline Move(Square from, Square to, MoveFlags flags) : move(0) { move = static_cast<uint16_t>((flags << 14) | (from << 7) | to); }

  Move(const std::string &str) {
    this->move = static_cast<uint16_t>((create_square(File(str[0] - 'a'), Rank(str[1] - '0')) << 7) |
                                       create_square(File(str[2] - 'a'), Rank(str[3] - '0')));
  }

  [[nodiscard]] inline Square    to() const { return Square(move & 0x7f); }
  [[nodiscard]] inline Square    from() const { return Square((move >> 7) & 0x7f); }
  [[nodiscard]] inline int       to_from() const { return move & 0x3fff; }
  [[nodiscard]] inline MoveFlags flags() const { return MoveFlags((move >> 14) & 0x3); }
  [[nodiscard]] inline bool      is_capture() const { return flags() == CAPTURE; }

  bool operator==(Move a) const { return to_from() == a.to_from(); }
  bool operator!=(Move a) const { return to_from() != a.to_from(); }
};

template<MoveFlags F = QUIET>
inline Move *make(Square from, Bitboard to, Move *list) {
  while (to)
    *list++ = Move(from, pop_lsb(&to), F);
  return list;
}

extern std::ostream &operator<<(std::ostream &os, const Move &m);

// Palace and River Masks
constexpr Bitboard WHITE_PALACE = sq_bb(d0)|sq_bb(e0)|sq_bb(f0)|sq_bb(d1)|sq_bb(e1)|sq_bb(f1)|sq_bb(d2)|sq_bb(e2)|sq_bb(f2);
constexpr Bitboard BLACK_PALACE = sq_bb(d7)|sq_bb(e7)|sq_bb(f7)|sq_bb(d8)|sq_bb(e8)|sq_bb(f8)|sq_bb(d9)|sq_bb(e9)|sq_bb(f9);

template<Color C>
constexpr Bitboard palace_mask() { return C == WHITE ? WHITE_PALACE : BLACK_PALACE; }
