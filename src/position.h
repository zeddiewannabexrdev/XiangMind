#pragma once
#include "tables.h"
#include <string>

namespace zobrist {
extern uint64_t zobrist_table[NPIECES][NSQUARES];
extern uint64_t zobrist_side;
void initialise_zobrist_keys();
}

struct UndoInfo {
    Piece captured;
    uint64_t hash;
    int fifty;
};

class Position {
public:
    Bitboard piece_bb[NPIECES];
    Piece board[NSQUARES];
    Color side_to_play;
    int game_ply;
    uint64_t hash;
    UndoInfo history[1024];

    Position();
    Color stm() const { return side_to_play; }
    Piece piece_on(Square s) const { return board[s]; }
    Bitboard pieces(PieceType pt, Color c) const { return piece_bb[make_piece(c, pt)]; }
    
    template<Color C> Bitboard all_pieces() const {
        return piece_bb[make_piece(C, SOLDIER)] | piece_bb[make_piece(C, ADVISOR)] |
               piece_bb[make_piece(C, ELEPHANT)] | piece_bb[make_piece(C, HORSE)] |
               piece_bb[make_piece(C, CANNON)] | piece_bb[make_piece(C, CHARIOT)] |
               piece_bb[make_piece(C, GENERAL)];
    }
    Bitboard all_pieces() const { return all_pieces<WHITE>() | all_pieces<BLACK>(); }
    Bitboard all_pieces(Color c) const { return c == WHITE ? all_pieces<WHITE>() : all_pieces<BLACK>(); }

    template<Color Us> Square king_sq() const { return bsf(pieces(GENERAL, Us)); }
    Square king_sq(Color c) const { return c == WHITE ? king_sq<WHITE>() : king_sq<BLACK>(); }
    
    bool is_draw() const;
    
    template<Color Us> void play(Move m);
    template<Color Us> void undo(Move m);
    
    template<Color Us> bool in_check() const;
    Bitboard attackers_to(Square sq, Bitboard occ) const;
    bool is_legal(Move m) const;
    
    template<Color Us, bool CAPTURES_ONLY> Move* generate_legals(Move* list) const;
    
    static void set(const std::string& fen, Position& p);
    std::string fen() const;
    uint64_t get_hash() const { return hash; }
    int get_ply() const { return game_ply; }
    int get_fifty() const { return history[game_ply].fifty; }

private:
    void put_piece(Piece pc, Square s);
    void remove_piece(Square s);
    void move_piece(Square from, Square to);
};
