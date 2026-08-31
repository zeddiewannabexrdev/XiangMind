#include "position.h"
#include <sstream>
#include <cmath>

namespace zobrist {
uint64_t zobrist_table[NPIECES][NSQUARES];
uint64_t zobrist_side;
void initialise_zobrist_keys() {}
}

Position::Position() {
    for (int i=0; i<NPIECES; ++i) piece_bb[i] = Bitboard{0,0};
    for (int i=0; i<NSQUARES; ++i) board[i] = NO_PIECE;
    side_to_play = WHITE;
    game_ply = 0;
    hash = 0;
    history[0] = {NO_PIECE, 0, 0};
}

void Position::put_piece(Piece pc, Square s) {
    board[s] = pc; piece_bb[pc] |= sq_bb(s);
}
void Position::remove_piece(Square s) {
    Piece pc = board[s]; board[s] = NO_PIECE; piece_bb[pc] &= ~sq_bb(s);
}
void Position::move_piece(Square from, Square to) {
    Piece pc = board[from]; board[from] = NO_PIECE; board[to] = pc;
    Bitboard mask = sq_bb(from) | sq_bb(to); piece_bb[pc] ^= mask;
}

bool Position::is_draw() const {
    if (history[game_ply].fifty >= 120) return true;
    int limit = std::max(0, game_ply - history[game_ply].fifty);
    for (int i = game_ply - 2; i >= limit; i -= 2) {
        if (history[i].hash == hash) return true;
    }
    return false;
}

Bitboard Position::attackers_to(Square sq, Bitboard occ) const {
    Bitboard attackers = Bitboard{0, 0};
    
    // Soldiers
    attackers |= SOLDIER_ATTACKS[BLACK][sq] & pieces(SOLDIER, WHITE);
    attackers |= SOLDIER_ATTACKS[WHITE][sq] & pieces(SOLDIER, BLACK);
    
    // General
    attackers |= GENERAL_ATTACKS[sq] & (pieces(GENERAL, WHITE) | pieces(GENERAL, BLACK));
    
    // Advisor
    attackers |= ADVISOR_ATTACKS[sq] & (pieces(ADVISOR, WHITE) | pieces(ADVISOR, BLACK));
    
    // Chariots
    attackers |= get_chariot_attacks(sq, occ) & (pieces(CHARIOT, WHITE) | pieces(CHARIOT, BLACK));
    
    // Cannons
    attackers |= get_cannon_capture_attacks(sq, occ) & (pieces(CANNON, WHITE) | pieces(CANNON, BLACK));
    
    // Horses (check all horses on the board for jumps targeting sq)
    Bitboard horses = (pieces(HORSE, WHITE) | pieces(HORSE, BLACK)) & HORSE_TABLE[sq].targets;
    while (horses) {
        Square h = pop_lsb(&horses);
        int dx = file_of(sq) - file_of(h);
        int dy = rank_of(sq) - rank_of(h);
        Square leg = std::abs(dx) == 2 ? create_square(File(file_of(h) + dx/2), rank_of(h)) 
                                       : create_square(file_of(h), Rank(rank_of(h) + dy/2));
        if (!(sq_bb(leg) & occ)) attackers |= sq_bb(h);
    }
    
    // Elephants (check all elephants on the board for jumps targeting sq)
    Bitboard elephants = (pieces(ELEPHANT, WHITE) | pieces(ELEPHANT, BLACK)) & ELEPHANT_TABLE[sq].targets;
    while (elephants) {
        Square el = pop_lsb(&elephants);
        Square eye = create_square(File((file_of(el) + file_of(sq)) / 2), Rank((rank_of(el) + rank_of(sq)) / 2));
        if (!(sq_bb(eye) & occ)) attackers |= sq_bb(el);
    }
    
    return attackers;
}

template<Color Us> void Position::play(Move m) {
    Square from = m.from();
    Square to = m.to();
    Piece pc = board[from];
    Piece captured = board[to];
    
    history[game_ply + 1].fifty = history[game_ply].fifty + 1;
    if (captured != NO_PIECE || type_of(pc) == SOLDIER) {
        history[game_ply + 1].fifty = 0;
    }
    history[game_ply].captured = captured;
    history[game_ply].hash = hash;
    
    if (captured != NO_PIECE) {
        remove_piece(to);
        hash ^= zobrist::zobrist_table[captured][to];
    }
    
    move_piece(from, to);
    hash ^= zobrist::zobrist_table[pc][from];
    hash ^= zobrist::zobrist_table[pc][to];
    
    side_to_play = ~side_to_play;
    hash ^= zobrist::zobrist_side;
    game_ply++;
}

template<Color Us> void Position::undo(Move m) {
    game_ply--;
    side_to_play = ~side_to_play;
    
    Square from = m.from();
    Square to = m.to();
    Piece pc = board[to];
    
    move_piece(to, from);
    
    Piece captured = history[game_ply].captured;
    if (captured != NO_PIECE) {
        put_piece(captured, to);
    }
    
    hash = history[game_ply].hash;
}

template<Color Us> bool Position::in_check() const {
    Square ksq = king_sq<Us>();
    Bitboard occ = all_pieces();
    constexpr Color Them = (Us == WHITE ? BLACK : WHITE);
    
    if (attackers_to(ksq, occ) & all_pieces(Them)) return true;
    
    Square enemy_ksq = king_sq<Them>();
    if (flying_general(ksq, enemy_ksq, occ)) return true;
    
    return false;
}

bool Position::is_legal(Move m) const {
    Square from = m.from();
    Square to = m.to();
    Color us = side_to_play;
    Color them = ~us;
    
    Square ksq = king_sq(us);
    if (board[from] == make_piece(us, GENERAL)) ksq = to;
    
    Bitboard occ = all_pieces();
    occ &= ~sq_bb(from);
    occ |= sq_bb(to);
    
    Bitboard attackers = attackers_to(ksq, occ);
    attackers &= ~sq_bb(to); // Exclude the piece that gets captured
    attackers &= all_pieces(them);
    
    if (attackers) return false;
    
    Square enemy_ksq = king_sq(them);
    if (to == enemy_ksq) return true; // Capturing enemy general is allowed/wins
    
    if (flying_general(ksq, enemy_ksq, occ)) return false;
    
    return true;
}

template<Color Us, bool CAPTURES_ONLY> Move* Position::generate_legals(Move* list) const {
    Move* start = list;
    constexpr Color Them = (Us == WHITE ? BLACK : WHITE);
    Bitboard occ = all_pieces();
    Bitboard us_pieces = all_pieces<Us>();
    Bitboard them_pieces = all_pieces<Them>();
    Bitboard target_squares = CAPTURES_ONLY ? them_pieces : ~us_pieces;
    
    auto add_moves = [&](Square from, Bitboard attacks) {
        attacks &= target_squares;
        Bitboard caps = attacks & them_pieces;
        Bitboard quiets = attacks & ~them_pieces;
        list = make<CAPTURE>(from, caps, list);
        if constexpr (!CAPTURES_ONLY) {
            list = make<QUIET>(from, quiets, list);
        }
    };

    Square ksq = king_sq<Us>();
    add_moves(ksq, GENERAL_ATTACKS[ksq]);

    Bitboard advs = pieces(ADVISOR, Us);
    while (advs) {
        Square sq = pop_lsb(&advs);
        add_moves(sq, ADVISOR_ATTACKS[sq]);
    }

    Bitboard ele = pieces(ELEPHANT, Us);
    while (ele) {
        Square sq = pop_lsb(&ele);
        Bitboard targets = ELEPHANT_TABLE[sq].targets;
        Bitboard valid_targets = Bitboard{0,0};
        while (targets) {
            Square t = pop_lsb(&targets);
            Square eye = create_square(File((file_of(sq) + file_of(t)) / 2), Rank((rank_of(sq) + rank_of(t)) / 2));
            if (!(sq_bb(eye) & occ)) valid_targets |= sq_bb(t);
        }
        add_moves(sq, valid_targets);
    }

    Bitboard hrs = pieces(HORSE, Us);
    while (hrs) {
        Square sq = pop_lsb(&hrs);
        Bitboard targets = HORSE_TABLE[sq].targets;
        Bitboard valid_targets = Bitboard{0,0};
        while (targets) {
            Square t = pop_lsb(&targets);
            int dx = file_of(t) - file_of(sq);
            int dy = rank_of(t) - rank_of(sq);
            Square leg = std::abs(dx) == 2 ? create_square(File(file_of(sq) + dx/2), rank_of(sq)) 
                                           : create_square(file_of(sq), Rank(rank_of(sq) + dy/2));
            if (!(sq_bb(leg) & occ)) valid_targets |= sq_bb(t);
        }
        add_moves(sq, valid_targets);
    }

    Bitboard cha = pieces(CHARIOT, Us);
    while (cha) {
        Square sq = pop_lsb(&cha);
        add_moves(sq, get_chariot_attacks(sq, occ));
    }

    Bitboard can = pieces(CANNON, Us);
    while (can) {
        Square sq = pop_lsb(&can);
        if constexpr (CAPTURES_ONLY) {
            add_moves(sq, get_cannon_capture_attacks(sq, occ));
        } else {
            Bitboard caps = get_cannon_capture_attacks(sq, occ) & them_pieces;
            Bitboard quiets = get_cannon_quiet_attacks(sq, occ) & ~occ;
            list = make<CAPTURE>(sq, caps, list);
            list = make<QUIET>(sq, quiets, list);
        }
    }

    Bitboard sol = pieces(SOLDIER, Us);
    while (sol) {
        Square sq = pop_lsb(&sol);
        add_moves(sq, SOLDIER_ATTACKS[Us][sq]);
    }

    Move* out = start;
    for (Move* m = start; m < list; ++m) {
        if (is_legal(*m)) {
            *out++ = *m;
        }
    }
    return out;
}

void Position::set(const std::string& fen, Position& p) {
    for (int i=0; i<NPIECES; ++i) p.piece_bb[i] = Bitboard{0,0};
    for (int i=0; i<NSQUARES; ++i) p.board[i] = NO_PIECE;
    
    std::istringstream iss(fen);
    std::string board_part, color_part, castling, ep, half, full;
    iss >> board_part >> color_part >> castling >> ep >> half >> full;
    
    int rank = RANK9;
    int file = AFILE;
    for (char c : board_part) {
        if (c == '/') {
            rank--;
            file = AFILE;
        } else if (isdigit(c)) {
            file += (c - '0');
        } else {
            Piece pc = NO_PIECE;
            switch (c) {
                case 'P': pc = WHITE_SOLDIER; break;
                case 'A': pc = WHITE_ADVISOR; break;
                case 'E': pc = WHITE_ELEPHANT; break;
                case 'H': pc = WHITE_HORSE; break;
                case 'C': pc = WHITE_CANNON; break;
                case 'R': pc = WHITE_CHARIOT; break;
                case 'G': case 'K': pc = WHITE_GENERAL; break;
                case 'p': pc = BLACK_SOLDIER; break;
                case 'a': pc = BLACK_ADVISOR; break;
                case 'e': pc = BLACK_ELEPHANT; break;
                case 'h': pc = BLACK_HORSE; break;
                case 'c': pc = BLACK_CANNON; break;
                case 'r': pc = BLACK_CHARIOT; break;
                case 'g': case 'k': pc = BLACK_GENERAL; break;
            }
            if (pc != NO_PIECE) {
                p.put_piece(pc, create_square((File)file, (Rank)rank));
                file++;
            }
        }
    }
    
    p.side_to_play = (color_part == "b" || color_part == "B") ? BLACK : WHITE;
    p.game_ply = 0;
    p.history[0].fifty = half.empty() ? 0 : std::stoi(half);
    
    p.hash = 0;
    for (int s = 0; s < NSQUARES; ++s) {
        if (p.board[s] != NO_PIECE) p.hash ^= zobrist::zobrist_table[p.board[s]][s];
    }
    if (p.side_to_play == BLACK) p.hash ^= zobrist::zobrist_side;
}

std::string Position::fen() const {
    std::string res;
    for (int r = RANK9; r >= RANK0; --r) {
        int empty = 0;
        for (int f = AFILE; f <= IFILE; ++f) {
            Piece pc = board[create_square((File)f, (Rank)r)];
            if (pc == NO_PIECE) {
                empty++;
            } else {
                if (empty > 0) { res += std::to_string(empty); empty = 0; }
                res += PIECE_STR[pc];
            }
        }
        if (empty > 0) res += std::to_string(empty);
        if (r > RANK0) res += "/";
    }
    res += (side_to_play == WHITE ? " w " : " b ");
    res += "- - ";
    res += std::to_string(history[game_ply].fifty) + " 1";
    return res;
}

// Explicit template instantiations
template void Position::play<WHITE>(Move m);
template void Position::play<BLACK>(Move m);
template void Position::undo<WHITE>(Move m);
template void Position::undo<BLACK>(Move m);
template bool Position::in_check<WHITE>() const;
template bool Position::in_check<BLACK>() const;
template Move* Position::generate_legals<WHITE, false>(Move* list) const;
template Move* Position::generate_legals<BLACK, false>(Move* list) const;
template Move* Position::generate_legals<WHITE, true>(Move* list) const;
template Move* Position::generate_legals<BLACK, true>(Move* list) const;
