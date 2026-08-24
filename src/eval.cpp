#include "eval.h"

namespace eval {

// Simple piece-square tables could be added here later.
// For now, we do a material + basic PST evaluation.

int evaluate(const Position& pos) {
    int score = 0;
    
    // Material
    score += pop_count(pos.pieces(SOLDIER, WHITE)) * PIECE_VALUE[SOLDIER];
    score += pop_count(pos.pieces(ADVISOR, WHITE)) * PIECE_VALUE[ADVISOR];
    score += pop_count(pos.pieces(ELEPHANT, WHITE)) * PIECE_VALUE[ELEPHANT];
    score += pop_count(pos.pieces(HORSE, WHITE)) * PIECE_VALUE[HORSE];
    score += pop_count(pos.pieces(CANNON, WHITE)) * PIECE_VALUE[CANNON];
    score += pop_count(pos.pieces(CHARIOT, WHITE)) * PIECE_VALUE[CHARIOT];

    score -= pop_count(pos.pieces(SOLDIER, BLACK)) * PIECE_VALUE[SOLDIER];
    score -= pop_count(pos.pieces(ADVISOR, BLACK)) * PIECE_VALUE[ADVISOR];
    score -= pop_count(pos.pieces(ELEPHANT, BLACK)) * PIECE_VALUE[ELEPHANT];
    score -= pop_count(pos.pieces(HORSE, BLACK)) * PIECE_VALUE[HORSE];
    score -= pop_count(pos.pieces(CANNON, BLACK)) * PIECE_VALUE[CANNON];
    score -= pop_count(pos.pieces(CHARIOT, BLACK)) * PIECE_VALUE[CHARIOT];
    
    // Basic positional rules:
    // 1. Soldiers crossing river are worth more (+100)
    Bitboard white_soldiers = pos.pieces(SOLDIER, WHITE);
    Bitboard black_soldiers = pos.pieces(SOLDIER, BLACK);
    
    // Rank >= 5 is crossed for white, Rank <= 4 is crossed for black
    // But evaluating bitboards is fast if we have masks
    // For now, loop over pieces for simplicity
    while(white_soldiers) {
        Square sq = pop_lsb(&white_soldiers);
        if (rank_of(sq) >= 5) score += 100; // crossed river
    }
    while(black_soldiers) {
        Square sq = pop_lsb(&black_soldiers);
        if (rank_of(sq) <= 4) score -= 100; // crossed river
    }

    return pos.stm() == WHITE ? score : -score;
}

} // namespace eval
