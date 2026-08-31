#pragma once
#include "position.h"
#include <mutex>
#include <string>

namespace uci {
    extern bool use_1_indexed;
    std::string move_to_string(Move m, bool is_1_indexed);
    void loop(bool bench, Position& pos, std::mutex& pos_mutex);
}
