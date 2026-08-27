#include "uci.h"
#include "search.h"
#include "position.h"
#include "tables.h"
#include <iostream>
#include <string>
#include <sstream>

namespace uci {

void loop(bool bench, Position& pos, std::mutex& pos_mutex) {
    std::string line;
    {
        std::lock_guard<std::mutex> lock(pos_mutex);
        Position::set(DEFAULT_FEN, pos);
    }
    
    while (std::getline(std::cin, line)) {
        if (line == "uci" || line == "ucci") {
            std::cout << "id name xiangqi-zeddieengine\n";
            std::cout << "id author askaig\n";
            if (line == "uci") std::cout << "uciok\n";
            if (line == "ucci") std::cout << "ucciok\n";
        } else if (line == "isready") {
            std::cout << "readyok\n";
        } else if (line == "ucinewgame") {
            std::lock_guard<std::mutex> lock(pos_mutex);
            Position::set(DEFAULT_FEN, pos);
        } else if (line.find("position fen ") == 0) {
            std::string fen = line.substr(13);
            std::lock_guard<std::mutex> lock(pos_mutex);
            Position::set(fen, pos);
        } else if (line.find("position startpos") == 0) {
            std::lock_guard<std::mutex> lock(pos_mutex);
            Position::set(DEFAULT_FEN, pos);
            size_t moves_pos = line.find(" moves ");
            if (moves_pos != std::string::npos) {
                std::istringstream iss(line.substr(moves_pos + 7));
                std::string move_str;
                while (iss >> move_str) {
                    Move list[256];
                    Move* end = pos.stm() == WHITE ? pos.generate_legals<WHITE, false>(list) : pos.generate_legals<BLACK, false>(list);
                    Move m(move_str);
                    bool legal = false;
                    for (Move* cur = list; cur != end; ++cur) {
                        if (cur->from() == m.from() && cur->to() == m.to()) {
                            m = *cur;
                            legal = true;
                            break;
                        }
                    }
                    if (legal) {
                        if (pos.stm() == WHITE) pos.play<WHITE>(m);
                        else pos.play<BLACK>(m);
                    }
                }
            }
        } else if (line.find("go") == 0) {
            Position copy_pos;
            {
                std::lock_guard<std::mutex> lock(pos_mutex);
                copy_pos = pos;
            }
            search::Result r = search::think(copy_pos, 1, nullptr, 1000, 0);
            std::cout << "bestmove " << r.best << "\n";
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
