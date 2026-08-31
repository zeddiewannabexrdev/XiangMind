#include "uci.h"
#include "search.h"
#include "position.h"
#include "tables.h"
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>

namespace uci {

bool use_1_indexed = false;

std::string move_to_string(Move m, bool is_1_indexed) {
    if (!is_1_indexed) {
        return std::string(SQSTR[m.from()]) + std::string(SQSTR[m.to()]);
    }
    return std::string(1, 'a' + file_of(m.from())) +
           std::to_string(rank_of(m.from()) + 1) +
           std::string(1, 'a' + file_of(m.to())) +
           std::to_string(rank_of(m.to()) + 1);
}

static Move find_legal_move(const Position& pos, const std::string& move_str) {
    Move list[256];
    Move* end = pos.stm() == WHITE ? pos.generate_legals<WHITE, false>(list) : pos.generate_legals<BLACK, false>(list);
    
    for (Move* cur = list; cur != end; ++cur) {
        // Match 0-indexed (e.g. h2e2)
        std::string s0 = std::string(SQSTR[cur->from()]) + std::string(SQSTR[cur->to()]);
        if (s0 == move_str) {
            return *cur;
        }
        // Match 1-indexed (e.g. h3e3 or b10c8)
        std::string s1 = std::string(1, 'a' + file_of(cur->from())) +
                         std::to_string(rank_of(cur->from()) + 1) +
                         std::string(1, 'a' + file_of(cur->to())) +
                         std::to_string(rank_of(cur->to()) + 1);
        if (s1 == move_str) {
            return *cur;
        }
    }
    return Move();
}

static void apply_moves(Position& pos, const std::string& moves_str) {
    std::istringstream iss(moves_str);
    std::string token;
    while (iss >> token) {
        Move m = find_legal_move(pos, token);
        if (m != Move()) {
            if (pos.stm() == WHITE) pos.play<WHITE>(m);
            else pos.play<BLACK>(m);
        }
    }
}

void loop(bool bench, Position& pos, std::mutex& pos_mutex) {
    std::string line;
    {
        std::lock_guard<std::mutex> lock(pos_mutex);
        Position::set(DEFAULT_FEN, pos);
    }
    
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;

        if (line == "uci" || line == "ucci") {
            std::cout << "id name xiangqi-zeddieengine" << std::endl;
            std::cout << "id author askaig" << std::endl;
            std::cout << "option name Hash type spin default 16 min 1 max 1024" << std::endl;
            std::cout << "option name Threads type spin default 1 min 1 max 128" << std::endl;
            std::cout << "option name UCI_Variant type string default xiangqi" << std::endl;
            std::cout << "option name UCI_XiangqiCoordinates type combo default cyclone var cyclone var standard" << std::endl;
            std::cout << "option name Move Overhead type spin default 10 min 0 max 5000" << std::endl;
            std::cout << "option name Ponder type check default false" << std::endl;
            if (line == "uci") std::cout << "uciok" << std::endl;
            if (line == "ucci") std::cout << "ucciok" << std::endl;
        } else if (line.find("setoption") == 0) {
            if (line.find("UCI_XiangqiCoordinates") != std::string::npos) {
                if (line.find("standard") != std::string::npos || line.find("1-indexed") != std::string::npos) {
                    use_1_indexed = true;
                } else {
                    use_1_indexed = false;
                }
            }
        } else if (line == "isready") {
            std::cout << "readyok" << std::endl;
        } else if (line == "ucinewgame") {
            std::lock_guard<std::mutex> lock(pos_mutex);
            Position::set(DEFAULT_FEN, pos);
            search::new_game();
        } else if (line.find("position ") == 0) {
            std::lock_guard<std::mutex> lock(pos_mutex);
            std::string rem = line.substr(9);
            size_t moves_pos = rem.find(" moves ");
            std::string moves_str;
            if (moves_pos != std::string::npos) {
                moves_str = rem.substr(moves_pos + 7);
                rem = rem.substr(0, moves_pos);
            }
            
            if (rem.find("startpos") == 0) {
                Position::set(DEFAULT_FEN, pos);
            } else if (rem.find("fen ") == 0) {
                Position::set(rem.substr(4), pos);
            }

            if (!moves_str.empty()) {
                apply_moves(pos, moves_str);
            }
        } else if (line.find("go") == 0) {
            Position copy_pos;
            {
                std::lock_guard<std::mutex> lock(pos_mutex);
                copy_pos = pos;
            }

            int depth = 0;
            int64_t movetime = 0;
            int64_t wtime = 0, btime = 0;
            int64_t winc = 0, binc = 0;
            
            std::istringstream iss(line);
            std::string token;
            iss >> token; // skip "go"
            while (iss >> token) {
                if (token == "depth") {
                    iss >> depth;
                } else if (token == "movetime") {
                    iss >> movetime;
                } else if (token == "wtime") {
                    iss >> wtime;
                } else if (token == "btime") {
                    iss >> btime;
                } else if (token == "winc") {
                    iss >> winc;
                } else if (token == "binc") {
                    iss >> binc;
                }
            }

            int64_t soft_ms = 0;
            int64_t hard_ms = 0;
            if (movetime > 0) {
                soft_ms = (movetime * 9) / 10;
                hard_ms = movetime;
            } else {
                int64_t my_time = (copy_pos.stm() == WHITE) ? wtime : btime;
                int64_t my_inc = (copy_pos.stm() == WHITE) ? winc : binc;
                if (my_time > 0) {
                    soft_ms = my_time / 25 + (my_inc * 3) / 4;
                    hard_ms = my_time / 10 + my_inc;
                    if (hard_ms > my_time - 50) hard_ms = std::max<int64_t>(10, my_time - 50);
                    if (soft_ms > hard_ms) soft_ms = hard_ms;
                }
            }

            search::Result r = search::think(copy_pos, depth, nullptr, soft_ms, hard_ms);
            std::cout << "bestmove " << move_to_string(r.best, use_1_indexed) << std::endl;
        } else if (line == "stop") {
            search::request_stop();
        } else if (line == "quit") {
            std::exit(0);
        } else if (line == "d") {
            std::lock_guard<std::mutex> lock(pos_mutex);
            for (int r = RANK9; r >= RANK0; --r) {
                for (int f = AFILE; f <= IFILE; ++f) {
                    Piece pc = pos.piece_on(create_square((File)f, (Rank)r));
                    if (pc == NO_PIECE) std::cout << ". ";
                    else std::cout << PIECE_STR[pc] << " ";
                }
                std::cout << "\n";
            }
            std::cout << pos.fen() << "\n";
        }
    }
}

} // namespace uci
