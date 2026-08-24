#pragma once
#include "types.h"

struct alignas(32) HorseEntry {
    Bitboard targets;
    Square legs[8]; 
};

struct alignas(32) ElephantEntry {
    Bitboard targets;
    Square eyes[4]; 
};

extern Bitboard GENERAL_ATTACKS[NSQUARES];
extern Bitboard ADVISOR_ATTACKS[NSQUARES];
extern HorseEntry HORSE_TABLE[NSQUARES];
extern ElephantEntry ELEPHANT_TABLE[NSQUARES];
extern Bitboard SOLDIER_ATTACKS[NCOLORS][NSQUARES];

extern Bitboard RANK_CHARIOT_ATK[10][512];
extern Bitboard FILE_CHARIOT_ATK[9][1024];
extern Bitboard RANK_CANNON_QUIET[10][512];
extern Bitboard FILE_CANNON_QUIET[9][1024];
extern Bitboard RANK_CANNON_CAPT[10][512];
extern Bitboard FILE_CANNON_CAPT[9][1024];

extern Bitboard SQUARES_BETWEEN_BB[NSQUARES][NSQUARES];
extern Bitboard LINE[NSQUARES][NSQUARES];

void initialise_all_databases();

inline int extract_rank(Bitboard occ, int r) {
    int shift = r * 9;
    if (shift <= 64 - 9) {
        return (occ.lo >> shift) & 0x1FF;
    } else if (shift >= 64) {
        return (occ.hi >> (shift - 64)) & 0x1FF;
    } else {
        return ((occ.lo >> shift) | (occ.hi << (64 - shift))) & 0x1FF;
    }
}

inline int extract_file(Bitboard occ, int f) {
    int res = 0;
    for (int r = 0; r < 10; ++r) {
        if (sq_bb(r * 9 + f) & occ) res |= (1 << r);
    }
    return res;
}

inline Bitboard get_chariot_attacks(Square sq, Bitboard occ) {
    int r = rank_of(sq);
    int f = file_of(sq);
    int rank_mask = extract_rank(occ, r);
    int file_mask = extract_file(occ, f);
    return RANK_CHARIOT_ATK[r][rank_mask] | FILE_CHARIOT_ATK[f][file_mask];
}

inline Bitboard get_cannon_quiet_attacks(Square sq, Bitboard occ) {
    int r = rank_of(sq);
    int f = file_of(sq);
    int rank_mask = extract_rank(occ, r);
    int file_mask = extract_file(occ, f);
    return RANK_CANNON_QUIET[r][rank_mask] | FILE_CANNON_QUIET[f][file_mask];
}

inline Bitboard get_cannon_capture_attacks(Square sq, Bitboard occ) {
    int r = rank_of(sq);
    int f = file_of(sq);
    int rank_mask = extract_rank(occ, r);
    int file_mask = extract_file(occ, f);
    return RANK_CANNON_CAPT[r][rank_mask] | FILE_CANNON_CAPT[f][file_mask];
}

inline bool flying_general(Square red_king, Square black_king, Bitboard occ) {
    if (file_of(red_king) != file_of(black_king)) return false;
    Bitboard between = SQUARES_BETWEEN_BB[red_king][black_king];
    return (between & occ) == Bitboard{0, 0};
}
