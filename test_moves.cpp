#include "src/position.h"
#include "src/tables.h"
#include <iostream>

int main() {
    initialise_all_databases();
    Position pos;
    Position::set(DEFAULT_FEN, pos);
    
    Move list[256];
    Move* end = pos.generate_legals<WHITE, false>(list);
    std::cout << "Legal moves for startpos: " << (end - list) << "\n";
    for (Move* cur = list; cur != end; ++cur) {
        std::cout << *cur << " ";
    }
    std::cout << "\n";
    return 0;
}
