#pragma once

#if defined(SIMD) && defined(ARCH_AVX2)
#include <immintrin.h>
#elif defined(SIMD) && defined(ARCH_NEON)
#include <arm_neon.h>
#endif

#include <bit>

inline int pop_count(Bitboard b) {
#if defined(SIMD) && defined(ARCH_AVX2)
  return static_cast<int>(_mm_popcnt_u64(b.lo)) + static_cast<int>(_mm_popcnt_u64(b.hi));
#else
  return std::popcount(b.lo) + std::popcount(b.hi);
#endif
}

inline int sparse_pop_count(Bitboard b) {
  return pop_count(b);
}

inline Square bsf(Bitboard b) {
#if defined(SIMD) && defined(ARCH_AVX2)
  return b.lo ? Square(_tzcnt_u64(b.lo)) : Square(64 + _tzcnt_u64(b.hi));
#else
  return b.lo ? Square(std::countr_zero(b.lo)) : Square(64 + std::countr_zero(b.hi));
#endif
}

inline Square pop_lsb(Bitboard *b) {
#if defined(SIMD) && defined(ARCH_AVX2)
  if (b->lo) {
    Square s = Square(_tzcnt_u64(b->lo));
    b->lo = _blsr_u64(b->lo);
    return s;
  }
  Square s = Square(64 + _tzcnt_u64(b->hi));
  b->hi = _blsr_u64(b->hi);
  return s;
#else
  if (b->lo) {
    Square s = Square(std::countr_zero(b->lo));
    b->lo &= b->lo - 1;
    return s;
  }
  Square s = Square(64 + std::countr_zero(b->hi));
  b->hi &= b->hi - 1;
  return s;
#endif
}

inline Bitboard or_reduce7(const Bitboard *p) {
  return p[0] | p[1] | p[2] | p[3] | p[4] | p[5] | p[6];
}
