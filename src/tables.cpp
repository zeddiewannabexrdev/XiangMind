#include "tables.h"
#include <vector>

Bitboard GENERAL_ATTACKS[NSQUARES];
Bitboard ADVISOR_ATTACKS[NSQUARES];
HorseEntry HORSE_TABLE[NSQUARES];
ElephantEntry ELEPHANT_TABLE[NSQUARES];
Bitboard SOLDIER_ATTACKS[NCOLORS][NSQUARES];

Bitboard RANK_CHARIOT_ATK[NSQUARES][512];
Bitboard FILE_CHARIOT_ATK[NSQUARES][1024];
Bitboard RANK_CANNON_QUIET[NSQUARES][512];
Bitboard FILE_CANNON_QUIET[NSQUARES][1024];
Bitboard RANK_CANNON_CAPT[NSQUARES][512];
Bitboard FILE_CANNON_CAPT[NSQUARES][1024];

Bitboard SQUARES_BETWEEN_BB[NSQUARES][NSQUARES];
Bitboard LINE[NSQUARES][NSQUARES];

static bool in_board(int r, int f) { return r >= 0 && r < 10 && f >= 0 && f < 9; }
static bool in_palace(int r, int f, Color c) {
    if (f < 3 || f > 5) return false;
    return c == WHITE ? (r >= 0 && r <= 2) : (r >= 7 && r <= 9);
}
static bool own_half(int r, Color c) {
    return c == WHITE ? (r <= 4) : (r >= 5);
}

static void init_leapers() {
    for (int i = 0; i < NSQUARES; ++i) {
        GENERAL_ATTACKS[i] = Bitboard{0, 0};
        ADVISOR_ATTACKS[i] = Bitboard{0, 0};
        HORSE_TABLE[i].targets = Bitboard{0, 0};
        for(int j=0; j<8; ++j) HORSE_TABLE[i].legs[j] = NO_SQUARE;
        ELEPHANT_TABLE[i].targets = Bitboard{0, 0};
        for(int j=0; j<4; ++j) ELEPHANT_TABLE[i].eyes[j] = NO_SQUARE;
        SOLDIER_ATTACKS[WHITE][i] = Bitboard{0, 0};
        SOLDIER_ATTACKS[BLACK][i] = Bitboard{0, 0};

        int r = i / 9;
        int f = i % 9;

        // General
        int gr[4] = {r+1, r-1, r, r};
        int gf[4] = {f, f, f+1, f-1};
        for (int d = 0; d < 4; ++d) {
            if (in_palace(gr[d], gf[d], WHITE) && in_palace(r, f, WHITE)) GENERAL_ATTACKS[i] |= sq_bb(Square(gr[d]*9 + gf[d]));
            if (in_palace(gr[d], gf[d], BLACK) && in_palace(r, f, BLACK)) GENERAL_ATTACKS[i] |= sq_bb(Square(gr[d]*9 + gf[d]));
        }

        // Advisor
        int ar[4] = {r+1, r+1, r-1, r-1};
        int af[4] = {f+1, f-1, f+1, f-1};
        for (int d = 0; d < 4; ++d) {
            if (in_palace(ar[d], af[d], WHITE) && in_palace(r, f, WHITE)) ADVISOR_ATTACKS[i] |= sq_bb(Square(ar[d]*9 + af[d]));
            if (in_palace(ar[d], af[d], BLACK) && in_palace(r, f, BLACK)) ADVISOR_ATTACKS[i] |= sq_bb(Square(ar[d]*9 + af[d]));
        }

        // Horse
        int hr[8] = {r+2, r+2, r-2, r-2, r+1, r-1, r+1, r-1};
        int hf[8] = {f+1, f-1, f+1, f-1, f+2, f+2, f-2, f-2};
        int lr[8] = {r+1, r+1, r-1, r-1, r, r, r, r};
        int lf[8] = {f, f, f, f, f+1, f+1, f-1, f-1};
        int h_idx = 0;
        for (int d = 0; d < 8; ++d) {
            if (in_board(hr[d], hf[d])) {
                HORSE_TABLE[i].targets |= sq_bb(Square(hr[d]*9 + hf[d]));
                HORSE_TABLE[i].legs[h_idx++] = create_square((File)lf[d], (Rank)lr[d]);
            }
        }

        // Elephant
        int er[4] = {r+2, r+2, r-2, r-2};
        int ef[4] = {f+2, f-2, f+2, f-2};
        int e_idx = 0;
        for (int d = 0; d < 4; ++d) {
            if (in_board(er[d], ef[d])) {
                bool w_ok = own_half(r, WHITE) && own_half(er[d], WHITE);
                bool b_ok = own_half(r, BLACK) && own_half(er[d], BLACK);
                if (w_ok || b_ok) {
                    ELEPHANT_TABLE[i].targets |= sq_bb(Square(er[d]*9 + ef[d]));
                    ELEPHANT_TABLE[i].eyes[e_idx++] = create_square((File)((f + ef[d])/2), (Rank)((r + er[d])/2));
                }
            }
        }

        // Soldier (White moves up/North, Black moves down/South)
        if (in_board(r+1, f)) SOLDIER_ATTACKS[WHITE][i] |= sq_bb(Square((r+1)*9 + f));
        if (in_board(r-1, f)) SOLDIER_ATTACKS[BLACK][i] |= sq_bb(Square((r-1)*9 + f));
        if (!own_half(r, WHITE)) { // crossed river
            if (in_board(r, f+1)) SOLDIER_ATTACKS[WHITE][i] |= sq_bb(Square(r*9 + f+1));
            if (in_board(r, f-1)) SOLDIER_ATTACKS[WHITE][i] |= sq_bb(Square(r*9 + f-1));
        }
        if (!own_half(r, BLACK)) { // crossed river
            if (in_board(r, f+1)) SOLDIER_ATTACKS[BLACK][i] |= sq_bb(Square(r*9 + f+1));
            if (in_board(r, f-1)) SOLDIER_ATTACKS[BLACK][i] |= sq_bb(Square(r*9 + f-1));
        }
    }
}

static void init_sliders() {
    // Rank Chariot/Cannon (9 files = 512 patterns)
    for (int r = 0; r < 10; ++r) {
        for (int mask = 0; mask < 512; ++mask) {
            for (int f = 0; f < 9; ++f) {
                Bitboard c_atk{0, 0};
                Bitboard p_quiet{0, 0};
                Bitboard p_capt{0, 0};
                
                // Right
                bool jumped = false;
                for (int x = f + 1; x < 9; ++x) {
                    if (!jumped) {
                        if ((mask & (1 << x)) == 0) {
                            c_atk |= sq_bb(Square(r*9 + x));
                            p_quiet |= sq_bb(Square(r*9 + x));
                        } else {
                            c_atk |= sq_bb(Square(r*9 + x));
                            jumped = true;
                        }
                    } else {
                        if ((mask & (1 << x)) != 0) {
                            p_capt |= sq_bb(Square(r*9 + x));
                            break;
                        }
                    }
                }
                
                // Left
                jumped = false;
                for (int x = f - 1; x >= 0; --x) {
                    if (!jumped) {
                        if ((mask & (1 << x)) == 0) {
                            c_atk |= sq_bb(Square(r*9 + x));
                            p_quiet |= sq_bb(Square(r*9 + x));
                        } else {
                            c_atk |= sq_bb(Square(r*9 + x));
                            jumped = true;
                        }
                    } else {
                        if ((mask & (1 << x)) != 0) {
                            p_capt |= sq_bb(Square(r*9 + x));
                            break;
                        }
                    }
                }
                
                int sq = r * 9 + f;
                RANK_CHARIOT_ATK[sq][mask] = c_atk;
                RANK_CANNON_QUIET[sq][mask] = p_quiet;
                RANK_CANNON_CAPT[sq][mask] = p_capt;
            }
        }
    }

    // File Chariot/Cannon (10 ranks = 1024 patterns)
    for (int f = 0; f < 9; ++f) {
        for (int mask = 0; mask < 1024; ++mask) {
            for (int r = 0; r < 10; ++r) {
                Bitboard c_atk{0, 0};
                Bitboard p_quiet{0, 0};
                Bitboard p_capt{0, 0};
                
                // Up
                bool jumped = false;
                for (int y = r + 1; y < 10; ++y) {
                    if (!jumped) {
                        if ((mask & (1 << y)) == 0) {
                            c_atk |= sq_bb(Square(y*9 + f));
                            p_quiet |= sq_bb(Square(y*9 + f));
                        } else {
                            c_atk |= sq_bb(Square(y*9 + f));
                            jumped = true;
                        }
                    } else {
                        if ((mask & (1 << y)) != 0) {
                            p_capt |= sq_bb(Square(y*9 + f));
                            break;
                        }
                    }
                }
                
                // Down
                jumped = false;
                for (int y = r - 1; y >= 0; --y) {
                    if (!jumped) {
                        if ((mask & (1 << y)) == 0) {
                            c_atk |= sq_bb(Square(y*9 + f));
                            p_quiet |= sq_bb(Square(y*9 + f));
                        } else {
                            c_atk |= sq_bb(Square(y*9 + f));
                            jumped = true;
                        }
                    } else {
                        if ((mask & (1 << y)) != 0) {
                            p_capt |= sq_bb(Square(y*9 + f));
                            break;
                        }
                    }
                }
                
                int sq = r * 9 + f;
                FILE_CHARIOT_ATK[sq][mask] = c_atk;
                FILE_CANNON_QUIET[sq][mask] = p_quiet;
                FILE_CANNON_CAPT[sq][mask] = p_capt;
            }
        }
    }
}

static void init_geometry() {
    for (int i = 0; i < NSQUARES; ++i) {
        for (int j = 0; j < NSQUARES; ++j) {
            SQUARES_BETWEEN_BB[i][j] = Bitboard{0, 0};
            LINE[i][j] = Bitboard{0, 0};
            
            if (i == j) continue;
            
            int r1 = i / 9, f1 = i % 9;
            int r2 = j / 9, f2 = j % 9;
            
            if (r1 == r2) { // Same rank
                int min_f = std::min(f1, f2);
                int max_f = std::max(f1, f2);
                for (int f = min_f + 1; f < max_f; ++f) SQUARES_BETWEEN_BB[i][j] |= sq_bb(Square(r1*9 + f));
                for (int f = 0; f < 9; ++f) LINE[i][j] |= sq_bb(Square(r1*9 + f));
            } else if (f1 == f2) { // Same file
                int min_r = std::min(r1, r2);
                int max_r = std::max(r1, r2);
                for (int r = min_r + 1; r < max_r; ++r) SQUARES_BETWEEN_BB[i][j] |= sq_bb(Square(r*9 + f1));
                for (int r = 0; r < 10; ++r) LINE[i][j] |= sq_bb(Square(r*9 + f1));
            }
        }
    }
}

void initialise_all_databases() {
    init_leapers();
    init_sliders();
    init_geometry();
}
